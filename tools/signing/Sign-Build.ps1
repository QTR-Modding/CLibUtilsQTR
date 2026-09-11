param(
	[Parameter(Mandatory)]
	[string] $DllPath,

	[Parameter(Mandatory)]
	[string] $VerifyTool,

	[Parameter(Mandatory)][string] $CertificateThumbprint,

	[Parameter(Mandatory)][ValidatePattern('\A[0-9a-fA-F]{64}\z')][string] $ExpectedPublicKeyHash,

	[string] $SignTool
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Signing.psm1') -Force
$dll = (Get-Item -LiteralPath $DllPath).FullName
if ([System.IO.Path]::GetExtension($dll) -ne '.dll') {
	throw 'The signing input must be a DLL.'
}
$verifier = (Get-Item -LiteralPath $VerifyTool).FullName
if ([System.IO.Path]::GetExtension($verifier) -ne '.exe') {
	throw 'Supply the standalone signature verifier executable with -VerifyTool.'
}
$signer = Find-SignTool -Path $SignTool
$certificate = Get-SigningCertificate -Thumbprint $CertificateThumbprint -RequirePrivateKey
try {
	$info = Get-SigningPublicInfo -Certificate $certificate
	if ($info.PublicKeySha256 -ine $ExpectedPublicKeyHash) { throw 'Certificate does not match the expected signing key.' }
	& $signer sign /q /fd SHA256 /sha1 $certificate.Thumbprint /s My $dll
	if ($LASTEXITCODE -ne 0) {
		throw "SignTool failed with exit code $LASTEXITCODE."
	}
	# The verifier checks the PKCS#7 signer and signed PE digest, without requiring root trust.
	& $verifier $dll $info.PublicKeySha256
	if ($LASTEXITCODE -ne 0) {
		throw "Signature verification failed with exit code $LASTEXITCODE. Do not deploy this DLL."
	}
} finally {
	$certificate.Dispose()
}

Write-Host "Signed and verified: $dll"
