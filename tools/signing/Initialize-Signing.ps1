[CmdletBinding(DefaultParameterSetName = 'Create')]
param(
	[Parameter(Mandatory, ParameterSetName = 'Create')]
	[ValidateNotNullOrEmpty()]
	[string] $Subject,

	[Parameter(ParameterSetName = 'Create')]
	[ValidateRange(1, 10)]
	[int] $ValidityYears = 10,

	[Parameter(Mandatory, ParameterSetName = 'Reuse')]
	[string] $CertificateThumbprint
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Signing.psm1') -Force

if ($PSCmdlet.ParameterSetName -eq 'Reuse') {
	$certificate = Get-SigningCertificate -Thumbprint $CertificateThumbprint -RequirePrivateKey
} else {
	$existing = @(Get-ChildItem -LiteralPath 'Cert:\CurrentUser\My' | Where-Object Subject -EQ $Subject)
	if ($existing.Count -ne 0) {
		throw 'A certificate with this subject already exists. Reuse the intended one with -CertificateThumbprint; no new key was created.'
	}
	$certificate = New-SelfSignedCertificate -Type CodeSigningCert `
		-Subject $Subject -FriendlyName 'DLL signing' `
		-CertStoreLocation 'Cert:\CurrentUser\My' `
		-Provider 'Microsoft Software Key Storage Provider' `
		-KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256 `
		-KeyExportPolicy NonExportable -KeyUsage DigitalSignature `
		-NotBefore (Get-Date).AddMinutes(-5) -NotAfter (Get-Date).AddYears($ValidityYears)
}

try {
	Get-SigningPublicInfo -Certificate $certificate
} finally {
	$certificate.Dispose()
}
