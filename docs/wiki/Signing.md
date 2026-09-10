# DLL signing

The `signing` module verifies Windows x64 DLLs signed with RSA/SHA-256 and resolves exports from the verified loaded image. It is header-only C++23 and uses Windows Crypt32, with no game or CommonLib dependency.

## Add it to a client

Select only this feature in your vcpkg manifest:

```json
{ "name": "clib-utils-qtr", "default-features": false, "features": ["signing"] }
```

Follow [Getting Started](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started) to install the overlay and add its include directory. Without vcpkg, add this repository's `include` directory to your project. MSVC links Crypt32 through the header.

If another library depends on this module, keep the same explicit dependency in the application's top-level manifest too. Vcpkg can otherwise expand the transitive dependency's default features and request Skyrim packages.

```cpp
#include <CLibUtilsQTR/Signing.hpp>
#include "ProviderSigningKey.hpp" // Your fixed SigningKeyHash, not a private key.

namespace Signing = clib_utilsQTR::Signing;

// Keep the binding alive while any returned function pointer can be used.
Signing::ProviderBinding provider{L"ExampleProvider.dll", ProviderSigningKey};

bool TryCallProvider()
{
    Signing::BindingError error{};
    auto address = provider.Resolve("ExampleValue", error);
    if (error != Signing::BindingError::None || !address) {
        return false; // Log or report the error using your own policy.
    }
    using ExampleValue = int (*)(); // Must match the provider's exported ABI.
    return reinterpret_cast<ExampleValue>(address)() == 42;
}
```

Call this outside `DllMain`. The binding locates an already-loaded DLL by basename; it does not load it. `Missing` can be retried after the provider loads. A missing optional export returns null with `None`. Other errors distinguish inspection, signature, image and export failures. The utility never displays a dialog or terminates the process.

Successful binding locks the signed file against replacement and pins the DLL for the rest of the process. Exports are checked on first lookup and then cached. Keep the binding alive through any client teardown that still calls the provider.

## Sign a provider

The provider author signs the DLL; client mod authors need only its public-key fingerprint. Build the verifier from a source checkout:

```powershell
cmake -S tools/signing -B build/signing -A x64
cmake --build build/signing --config Release
```

Run the scripts in Windows PowerShell with the Windows SDK installed. For a new project, create a key once:

```powershell
./tools/signing/Initialize-Signing.ps1 -Subject 'CN=Your project'
```

For an existing project, reuse its certificate with `-CertificateThumbprint` instead. Initialization prints the certificate thumbprint and the SHA-256 hash of its DER RSA public key. These are different values. Put the 32 public-hash bytes into a `SigningKeyHash` constant in the client SDK. Never include the private key.

```powershell
./tools/signing/Sign-Build.ps1 `
    -DllPath ./build/ExampleProvider.dll `
    -VerifyTool ./build/signing/Release/qtr_verify_signature.exe `
    -CertificateThumbprint '<certificate thumbprint>' `
    -ExpectedPublicKeyHash '<64-character public-key SHA-256>'
```

`Export-PublicKey.ps1 -CertificateThumbprint ... -OutputPath ...` saves public information as JSON. The signing script checks the selected key before signing and verifies the signed DLL afterward. Call it after linking, before packaging.

New keys created by the helper are non-exportable and tied to the Windows user/key store. Plan key continuity before distributing clients: losing or changing that key requires updating their pinned fingerprint. These scripts do not provide a private-key backup or migration system.

## What it checks

Verification checks the Authenticode digest and pinned RSA key, then compares the loaded headers and executable sections with the signed file, accounting for relocations. Export resolution rejects forwarded, redirected and non-executable addresses. It does not require trusting a self-signed certificate as a Windows root.

This is not a sandbox against arbitrary native code in the same process. It does not continuously monitor code after binding, certify DLL behavior, or prevent antivirus warnings. Choose the trusted key deliberately; each provider keeps its own key and client failure policy.
