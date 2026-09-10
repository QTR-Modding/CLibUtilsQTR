#include <CLibUtilsQTR/Signing/CommandLine.hpp>
int wmain(int argc, wchar_t** argv) {
    return clib_utilsQTR::Signing::VerifyFileCommandLine(argc, argv);
}
