#include <windows.h>
#include <wincrypt.h>
#include <stdexcept>

namespace {
enum class Mode { Empty, Oversized, QueryFailure, ReadFailure, Valid };
Mode mode{};
BOOL WINAPI TestGetParam(HCRYPTMSG, DWORD, DWORD, void* data, DWORD* size) {
    if (mode == Mode::QueryFailure || (data && mode == Mode::ReadFailure)) {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    *size = mode == Mode::Empty ? 0 : mode == Mode::Oversized ? 1024 * 1024 + 1 : 1;
    if (data) *static_cast<BYTE*>(data) = 42;
    return TRUE;
}
}

#define CryptMsgGetParam TestGetParam
#include <CLibUtilsQTR/Signing/Authenticode.hpp>
#undef CryptMsgGetParam

int main() {
    clib_utilsQTR::Signing::SignatureMessage message;
    std::vector<BYTE> output;
    for (const auto current : {Mode::Empty, Mode::Oversized, Mode::QueryFailure, Mode::ReadFailure, Mode::Valid}) {
        mode = current;
        SetLastError(ERROR_ACCESS_DENIED);
        const bool result = message.Parameter(CMSG_CONTENT_PARAM, output);
        const auto error = GetLastError();
        const bool valid = current == Mode::Valid;
        const bool local = current == Mode::Empty || current == Mode::Oversized;
        if (result != valid || (!valid && error != (local ? ERROR_SUCCESS : ERROR_INVALID_DATA)) ||
            (valid && (output.size() != 1 || output[0] != 42))) {
            throw std::runtime_error("Signature parameter diagnostic mismatch");
        }
    }
}
