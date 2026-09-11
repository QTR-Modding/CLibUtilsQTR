param(
    [string] $BuildDirectory = "$PSScriptRoot/../../build/signing-tests",
    [string] $Generator
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot/../../tools/signing/Signing.psm1" -Force
$build = [IO.Path]::GetFullPath($BuildDirectory)
New-Item -ItemType Directory -Path $build -Force | Out-Null
$signTool = Find-SignTool
$certificates = @()
function Invoke-Checked([string] $Command, [string[]] $Arguments) {
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Command failed: $Command ($LASTEXITCODE)" }
}
try {
    foreach ($role in @('primary', 'other')) {
        $certificates += New-SelfSignedCertificate -Type CodeSigningCert -Subject ("CN=QTR signing test $role " + [guid]::NewGuid()) `
            -CertStoreLocation 'Cert:\CurrentUser\My' -Provider 'Microsoft Software Key Storage Provider' `
            -KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256 -KeyExportPolicy NonExportable `
            -KeyUsage DigitalSignature -NotBefore (Get-Date).AddMinutes(-5) -NotAfter (Get-Date).AddDays(1)
    }
    $primary = Get-SigningPublicInfo $certificates[0]
    $initializer = ([regex]::Matches($primary.PublicKeySha256, '..') | ForEach-Object { '0x' + $_.Value }) -join ', '
    [IO.File]::WriteAllText((Join-Path $build 'TestKey.hpp'), "#pragma once`ninline constexpr clib_utilsQTR::Signing::SigningKeyHash testKey{ $initializer };`n")
    $configure = @('-S', $PSScriptRoot, '-B', $build, '-A', 'x64')
    if ($Generator) { $configure += @('-G', $Generator) }
    Invoke-Checked cmake $configure
    Invoke-Checked cmake @('--build', $build, '--config', 'Release')
    $artifacts = Get-Content (Join-Path $build 'artifacts-Release.json') -Raw | ConvertFrom-Json
    for ($index = 0; $index -lt 2; ++$index) {
        $dll = if ($index -eq 0) { $artifacts.signed } else { $artifacts.wrong }
        $info = Get-SigningPublicInfo $certificates[$index]
        & "$PSScriptRoot/../../tools/signing/Sign-Build.ps1" -DllPath $dll -VerifyTool $artifacts.verifier `
            -CertificateThumbprint $info.CertificateThumbprint -ExpectedPublicKeyHash $info.PublicKeySha256 -SignTool $signTool
    }
    Invoke-Checked ctest @('--test-dir', $build, '-C', 'Release', '--output-on-failure')
} finally {
    $cleanupErrors = @()
    foreach ($certificate in $certificates) {
      try {
        $path = 'Cert:\CurrentUser\My\' + $certificate.Thumbprint
        $rsa = [Security.Cryptography.X509Certificates.RSACertificateExtensions]::GetRSAPrivateKey($certificate)
        try { $name = $rsa.Key.KeyName; $provider = $rsa.Key.Provider } finally { $rsa.Dispose() }
        $stored = Get-Item -LiteralPath $path
        if ($stored.Subject -cne $certificate.Subject) { throw 'Temporary certificate identity changed.' }
        $stored.Dispose()
        $certificate.Dispose()
        Remove-Item -LiteralPath $path -DeleteKey -Force
        if ((Test-Path -LiteralPath $path) -or [Security.Cryptography.CngKey]::Exists($name, $provider)) {
            throw 'Temporary signing key cleanup failed.'
        }
      } catch { $cleanupErrors += $_.Exception.Message }
    }
    if ($cleanupErrors.Count) { throw ($cleanupErrors -join "`n") }
}
