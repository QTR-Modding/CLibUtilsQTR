param(
	[Parameter(Mandatory)][string] $CertificateThumbprint,

	[string] $OutputPath
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Signing.psm1') -Force
if (-not $CertificateThumbprint) {
	throw 'Supply -CertificateThumbprint.'
}

$certificate = Get-SigningCertificate -Thumbprint $CertificateThumbprint
try {
	$info = Get-SigningPublicInfo -Certificate $certificate
} finally {
	$certificate.Dispose()
}

if ($OutputPath) {
	$fullPath = [System.IO.Path]::GetFullPath($OutputPath)
	if ([System.IO.Path]::GetExtension($fullPath) -ne '.json') {
		throw 'The public verification output must be a .json file.'
	}
	$json = ($info | ConvertTo-Json) + [Environment]::NewLine
	if (Test-Path -LiteralPath $fullPath) {
		if ([System.IO.File]::ReadAllText($fullPath) -cne $json) {
			throw 'The output already exists with different contents. Choose a new output path.'
		}
	} else {
		[System.IO.File]::WriteAllText($fullPath, $json, [System.Text.UTF8Encoding]::new($false))
	}
}

$info
