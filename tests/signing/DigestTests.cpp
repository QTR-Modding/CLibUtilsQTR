#include <CLibUtilsQTR/Signing.hpp>
#include "TestKey.hpp"
#include <iostream>
#include <stdexcept>

namespace S = clib_utilsQTR::Signing;
void Require(bool passed, const char* message) {
    if (!passed) throw std::runtime_error(message);
}

void CompareWindows(const S::SignedFile& file) {
    GUID subjectType{};
    Require(CryptSIPRetrieveSubjectGuid(file.path.c_str(), file.handle, &subjectType), "Windows subject");
    SIP_SUBJECTINFO subject{};
    subject.cbSize = sizeof(subject);
    subject.pgSubjectType = &subjectType;
    subject.hFile = file.handle;
    subject.pwsFileName = file.path.c_str();
    subject.dwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    subject.DigestAlgorithm.pszObjId = const_cast<char*>(szOID_NIST_sha256);
    DWORD size{};
    Require(CryptSIPCreateIndirectData(&subject, &size, nullptr), "Windows digest size");
    std::vector<BYTE> buffer(size);
    auto* native = reinterpret_cast<SIP_INDIRECT_DATA*>(buffer.data());
    Require(CryptSIPCreateIndirectData(&subject, &size, native), "Windows digest");
    std::array<BYTE, 32> digest{};
    Require(S::ComputePeDigest(file.bytes, digest), "Local digest");
    Require(native->Digest.cbData == digest.size() &&
        std::memcmp(native->Digest.pbData, digest.data(), digest.size()) == 0, "Windows digest mismatch");
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 1;
    const bool oracle = std::wstring_view(argv[2]) == L"oracle";
    const auto ntdll = GetModuleHandleW(L"ntdll.dll");
    if (oracle && ntdll && GetProcAddress(ntdll, "wine_get_version")) {
        std::cout << "Skipping the native Windows SIP oracle under Wine\n";
        return 77;
    }
    S::SignedFile original;
    Require(original.Open(argv[1]), "Read signed fixture");
    if (oracle) CompareWindows(original);
    S::SignatureDiagnostic diagnostic;
    Require(S::VerifySignature(original, testKey, &diagnostic), "Signed fixture rejected");
    auto wrongKey = testKey;
    wrongKey[0] ^= 1;
    Require(!S::VerifySignature(original, wrongKey, &diagnostic) &&
        std::wstring_view(diagnostic.stage) == L"trusted signing key", "Wrong key diagnostic");
    std::vector<std::byte> modified(original.bytes.begin(), original.bytes.end());
    S::SignedFile candidate;
    candidate.bytes = modified;
    // Borrowed memory is not a mapping: detach it before SignedFile destruction.
    struct Detach { S::SignedFile& file; ~Detach() { file.bytes = {}; } } detach{candidate};
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(modified.data());
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(modified.data() + dos->e_lfanew);
    const auto savedChecksum = nt->OptionalHeader.CheckSum;
    nt->OptionalHeader.CheckSum ^= 0x1234;
    Require(S::VerifySignature(candidate, testKey), "Checksum must be excluded");
    nt->OptionalHeader.CheckSum = savedChecksum;
    auto* sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(nt + 1);
    const auto raw = sections[0].PointerToRawData;
    modified[raw] ^= std::byte{1};
    Require(!S::VerifySignature(candidate, testKey, &diagnostic) &&
        std::wstring_view(diagnostic.stage) == L"file digest mismatch", "Changed code accepted");
    modified[raw] ^= std::byte{1};
    modified[2] ^= std::byte{1};
    Require(!S::VerifySignature(candidate, testKey), "Changed header accepted");
    modified[2] ^= std::byte{1};
    std::array<BYTE, 32> digest{};
    sections[0].PointerToRawData = 0;
    Require(!S::ComputePeDigest(modified, digest), "Overlapping section accepted");
    sections[0].PointerToRawData = raw + 8;
    Require(!S::ComputePeDigest(modified, digest), "Gapped section accepted");
    sections[0].PointerToRawData = raw;
    const auto certificate = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = raw;
    Require(!S::ComputePeDigest(modified, digest), "Overlapping certificate accepted");
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] = certificate;
    Require(!S::ComputePeDigest(std::span{modified}.first(modified.size() - 1), digest), "Truncation accepted");
    Require(nt->FileHeader.NumberOfSections > 1, "Fixture needs multiple sections");
    std::swap(sections[0], sections[1]);
    Require(S::ComputePeDigest(modified, digest), "Out-of-order section headers rejected");
    Require(!S::VerifySignature(candidate, testKey), "Changed section table accepted");
    std::swap(sections[0], sections[1]);
    const auto savedSize = sections[0].SizeOfRawData;
    sections[0].SizeOfRawData = MAXDWORD;
    Require(!S::ComputePeDigest(modified, digest), "Oversized section accepted");
    sections[0].SizeOfRawData = savedSize;
    const auto certByte = certificate.VirtualAddress + offsetof(WIN_CERTIFICATE, bCertificate);
    modified[certByte] ^= std::byte{1};
    Require(!S::VerifySignature(candidate, testKey), "Damaged signature accepted");
    modified[certByte] ^= std::byte{1};
    Require(S::VerifySignature(candidate, testKey, &diagnostic) &&
        std::wstring_view(diagnostic.stage) == L"none" && diagnostic.error == 0, "Recovery retained stale error");
    std::cout << "PE digest and signature regression tests passed\n";
}
