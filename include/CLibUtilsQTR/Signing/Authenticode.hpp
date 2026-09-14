#pragma once

// Copyright (c) 2026 Quantumyilmaz. MIT License.

#include "PeDigest.hpp"

#include <wincrypt.h>
#include <mssip.h>
#include <wintrust.h>
#include <softpub.h>

#include <array>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#pragma comment(lib, "crypt32.lib")
#endif

namespace clib_utilsQTR::Signing {

using SigningKeyHash = std::array<BYTE, 32>;

struct SignedFile {
    HANDLE handle{INVALID_HANDLE_VALUE};
    HANDLE mapping{};
    std::span<const std::byte> bytes;
    std::wstring path;

    SignedFile() = default;
    SignedFile(const SignedFile&) = delete;
    SignedFile& operator=(const SignedFile&) = delete;
    ~SignedFile() {
        if (!bytes.empty()) UnmapViewOfFile(bytes.data());
        if (mapping) CloseHandle(mapping);
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
    }

    bool Open(const wchar_t* name) {
        path = name;
        if (handle != INVALID_HANDLE_VALUE) return false;
        // Deny writers/deletion while the same file is inspected and hashed.
        handle = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(handle, &size) || size.QuadPart < sizeof(IMAGE_DOS_HEADER) ||
            size.QuadPart > 512 * 1024 * 1024) return false;
        mapping = CreateFileMappingW(handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping) return false;
        const auto* view = static_cast<const std::byte*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
        if (!view) return false;
        bytes = {view, static_cast<std::size_t>(size.QuadPart)};
        return true;
    }
};

struct SignatureMessage {
    SignatureMessage() = default;
    SignatureMessage(const SignatureMessage&) = delete;
    SignatureMessage& operator=(const SignatureMessage&) = delete;
    HCERTSTORE store{};
    HCRYPTMSG message{};
    PCCERT_CONTEXT signer{};
    SIP_INDIRECT_DATA* signedDigest{};

    ~SignatureMessage() {
        if (signedDigest) LocalFree(signedDigest);
        if (signer) CertFreeCertificateContext(signer);
        if (message) CryptMsgClose(message);
        if (store) CertCloseStore(store, 0);
    }

    bool Parameter(DWORD param, std::vector<BYTE>& output) const {
        DWORD size{};
        if (!CryptMsgGetParam(message, param, 0, nullptr, &size)) return false;
        if (!size || size > 1024 * 1024) {
            SetLastError(ERROR_SUCCESS);
            return false;
        }
        output.resize(size);
        return CryptMsgGetParam(message, param, 0, output.data(), &size) != FALSE;
    }
};

// Verify an embedded SHA-256 Authenticode signature against our pinned RSA
// public key, not Windows' machine-wide publisher trust. No root-store edits,
// network lookup, expired test-certificate exception, or subject-name trust.
inline bool VerifySignature(const SignedFile& file, const SigningKeyHash& expectedKey,
    SignatureDiagnostic* diagnostic = nullptr) {
    if (diagnostic) *diagnostic = {};
    const PeImage image(file.bytes);
    const auto* nt = image.Headers();
    if (!nt) return SignatureFailure(diagnostic, L"PE headers");
    const auto& certificate = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
    const auto* entry = image.At<WIN_CERTIFICATE>(certificate.VirtualAddress);
    constexpr std::size_t prefix = offsetof(WIN_CERTIFICATE, bCertificate);
    if (!certificate.VirtualAddress || !entry || certificate.Size < prefix ||
        entry->dwLength <= prefix || entry->dwLength > certificate.Size ||
        entry->wRevision != WIN_CERT_REVISION_2_0 || entry->wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA ||
        !image.At<BYTE>(certificate.VirtualAddress, certificate.Size))
        return SignatureFailure(diagnostic, L"embedded signature layout");

    CRYPT_DATA_BLOB blob{entry->dwLength - static_cast<DWORD>(prefix), const_cast<BYTE*>(entry->bCertificate)};
    SignatureMessage signature;
    if (!CryptQueryObject(CERT_QUERY_OBJECT_BLOB, &blob, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
            CERT_QUERY_FORMAT_FLAG_BINARY, 0, nullptr, nullptr, nullptr, &signature.store,
            &signature.message, nullptr)) return SignatureFailure(diagnostic, L"CryptQueryObject", GetLastError());
    DWORD signerCount{}, countSize = sizeof(signerCount);
    if (!CryptMsgGetParam(signature.message, CMSG_SIGNER_COUNT_PARAM, 0, &signerCount, &countSize))
        return SignatureFailure(diagnostic, L"signer count", GetLastError());
    if (signerCount != 1) return SignatureFailure(diagnostic, L"single signer required");
    std::vector<BYTE> signerInfo;
    if (!signature.Parameter(CMSG_SIGNER_CERT_INFO_PARAM, signerInfo))
        return SignatureFailure(diagnostic, L"signer info", GetLastError());
    signature.signer = CertFindCertificateInStore(signature.store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0, CERT_FIND_SUBJECT_CERT, signerInfo.data(), nullptr);
    if (!signature.signer) return SignatureFailure(diagnostic, L"signer certificate", GetLastError());
    const auto& publicKey = signature.signer->pCertInfo->SubjectPublicKeyInfo;
    SigningKeyHash keyHash{};
    DWORD hashSize = static_cast<DWORD>(keyHash.size());
    if (std::strcmp(publicKey.Algorithm.pszObjId, szOID_RSA_RSA) != 0 || publicKey.PublicKey.cUnusedBits != 0)
        return SignatureFailure(diagnostic, L"RSA key format");
    if (!CryptHashCertificate2(L"SHA256", 0, nullptr, publicKey.PublicKey.pbData, publicKey.PublicKey.cbData,
            keyHash.data(), &hashSize)) return SignatureFailure(diagnostic, L"public-key hash", GetLastError());
    if (hashSize != keyHash.size() || keyHash != expectedKey)
        return SignatureFailure(diagnostic, L"trusted signing key");
    if (!CryptMsgControl(signature.message, 0, CMSG_CTRL_VERIFY_SIGNATURE,
            signature.signer->pCertInfo)) return SignatureFailure(diagnostic, L"RSA signature", GetLastError());

    std::vector<BYTE> contentType, content, algorithm;
    if (!signature.Parameter(CMSG_INNER_CONTENT_TYPE_PARAM, contentType))
        return SignatureFailure(diagnostic, L"signed content type", GetLastError());
    if (contentType.back() != 0 || std::strcmp(reinterpret_cast<const char*>(contentType.data()),
            SPC_INDIRECT_DATA_OBJID) != 0) return SignatureFailure(diagnostic, L"Authenticode content required");
    if (!signature.Parameter(CMSG_SIGNER_HASH_ALGORITHM_PARAM, algorithm))
        return SignatureFailure(diagnostic, L"signer hash algorithm", GetLastError());
    if (algorithm.size() < sizeof(CRYPT_ALGORITHM_IDENTIFIER) ||
        std::strcmp(reinterpret_cast<CRYPT_ALGORITHM_IDENTIFIER*>(algorithm.data())->pszObjId,
            szOID_NIST_sha256) != 0) return SignatureFailure(diagnostic, L"SHA-256 signature required");
    if (!signature.Parameter(CMSG_CONTENT_PARAM, content))
        return SignatureFailure(diagnostic, L"signed content", GetLastError());
    DWORD decodedSize{};
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, SPC_INDIRECT_DATA_OBJID,
            content.data(), static_cast<DWORD>(content.size()), CRYPT_DECODE_ALLOC_FLAG, nullptr,
            &signature.signedDigest, &decodedSize)) return SignatureFailure(diagnostic, L"decode signed digest", GetLastError());
    const auto* signedDigest = signature.signedDigest;
    if (std::strcmp(signedDigest->Data.pszObjId, SPC_PE_IMAGE_DATA_OBJID) != 0 ||
        std::strcmp(signedDigest->DigestAlgorithm.pszObjId, szOID_NIST_sha256) != 0 ||
        signedDigest->Digest.cbData != 32) return SignatureFailure(diagnostic, L"SHA-256 PE digest required");

    std::array<BYTE, 32> computed{};
    if (!ComputePeDigest(file.bytes, computed, diagnostic)) return false;
    return std::memcmp(computed.data(), signedDigest->Digest.pbData, computed.size()) == 0 ||
        SignatureFailure(diagnostic, L"file digest mismatch");
}

}
