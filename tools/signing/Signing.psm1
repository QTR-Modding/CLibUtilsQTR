$ErrorActionPreference = 'Stop'

function Get-SigningCertificate {
	param(
		[Parameter(Mandatory)]
		[string] $Thumbprint,

		[switch] $RequirePrivateKey
	)

	$normalizedThumbprint = $Thumbprint -replace '\s', ''
	if ($normalizedThumbprint -notmatch '\A[0-9a-fA-F]{40}\z') {
		throw 'Provide an exact certificate thumbprint (40 hexadecimal characters).'
	}
	$store = [System.Security.Cryptography.X509Certificates.X509Store]::new('My', 'CurrentUser')
	try {
		$store.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadOnly)
		$matches = $store.Certificates.Find('FindByThumbprint', $normalizedThumbprint, $false)
		if ($matches.Count -ne 1) { throw 'The configured signing certificate was not found.' }
		$certificate = $matches[0]
	} finally {
		$store.Close()
	}
	if ($certificate.PublicKey.Oid.Value -ne '1.2.840.113549.1.1.1') {
		throw 'The signing certificate must use an RSA key.'
	}
	$rsa = [System.Security.Cryptography.X509Certificates.RSACertificateExtensions]::GetRSAPublicKey($certificate)
	try {
		if ($rsa.KeySize -lt 3072) {
			throw 'The signing certificate must use an RSA key of at least 3072 bits.'
		}
	} finally {
		$rsa.Dispose()
	}
	$codeSigning = @($certificate.Extensions | Where-Object {
		$_ -is [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension]
	} | ForEach-Object { $_.EnhancedKeyUsages } | Where-Object {
		$_.Value -eq '1.3.6.1.5.5.7.3.3'
	})
	if ($codeSigning.Count -eq 0) {
		throw 'The certificate does not allow code signing.'
	}
	if ($RequirePrivateKey) {
		$now = Get-Date
		if (-not $certificate.HasPrivateKey) {
			throw 'The selected certificate has no private signing key on this computer.'
		}
		if ($now -lt $certificate.NotBefore -or $now -gt $certificate.NotAfter) {
			throw 'The selected signing certificate is not currently valid.'
		}
	}
	return $certificate
}

function Get-SigningPublicInfo {
	param(
		[Parameter(Mandatory)]
		[System.Security.Cryptography.X509Certificates.X509Certificate2] $Certificate
	)

	$publicKey = $Certificate.GetPublicKey()
	$sha256 = [System.Security.Cryptography.SHA256]::Create()
	try {
		$publicKeyHash = -join ($sha256.ComputeHash($publicKey) | ForEach-Object { $_.ToString('X2') })
	} finally {
		$sha256.Dispose()
	}
	[pscustomobject][ordered]@{
		Subject = $Certificate.Subject
		CertificateThumbprint = $Certificate.Thumbprint
		PublicKeyAlgorithm = 'RSA'
		PublicKeyAlgorithmOid = $Certificate.PublicKey.Oid.Value
		PublicKeyEncoding = 'DER RSAPublicKey'
		PublicKeyHashAlgorithm = 'SHA256'
		PublicKeySha256 = $publicKeyHash
		PublicKeyDerBase64 = [Convert]::ToBase64String($publicKey)
		NotAfterUtc = $Certificate.NotAfter.ToUniversalTime().ToString('o')
	}
}

function Find-SignTool {
	param([string] $Path)

	if ($Path) {
		return (Resolve-Path -LiteralPath $Path).Path
	}
	$command = Get-Command signtool.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
	if ($command) {
		return $command.Source
	}
	$kits = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' `
		-Name KitsRoot10 -ErrorAction SilentlyContinue
	if ($kits) {
		$versions = Get-ChildItem -LiteralPath (Join-Path $kits.KitsRoot10 'bin') -Directory |
			Where-Object Name -Match '^10\.\d+\.\d+\.\d+$' |
			Sort-Object { [version] $_.Name } -Descending
		foreach ($version in $versions) {
			$candidate = Join-Path $version.FullName 'x64\signtool.exe'
			if (Test-Path -LiteralPath $candidate -PathType Leaf) {
				return $candidate
			}
		}
	}
	throw 'Windows SDK signtool.exe was not found. Supply -SignTool with its full path.'
}

Export-ModuleMember -Function Get-SigningCertificate, Get-SigningPublicInfo, Find-SignTool
