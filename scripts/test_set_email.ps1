<#
.SYNOPSIS
  Test setting device email via /api/set-email and validate /api/status.

.EXAMPLE
  .\scripts\test_set_email.ps1 -Device 'http://jacob-f4e9d4.local' -Token 's3cr3t' -Email 'you@example.com'
#>
param(
  [string]$Device = 'http://localhost',
  [string]$Token = '',
  [string]$Email = 'you@example.com'
)

$headers = @{}
$headers['Content-Type'] = 'application/json'
if ($Token -ne '') { $headers['Authorization'] = "Bearer $Token" }

$body = @{ email = $Email } | ConvertTo-Json

try {
  Write-Host "POSTing email to $Device (email=$Email)"
  $resp = Invoke-RestMethod -Uri "$Device/api/set-email" -Method Post -Headers $headers -Body $body -ContentType 'application/json' -UseBasicParsing
  Write-Host "Response:`n" ($resp | ConvertTo-Json -Depth 4)
} catch {
  Write-Error "POST failed: $_"
  exit 2
}

try {
  $status = Invoke-RestMethod -Uri "$Device/api/status" -Method Get -UseBasicParsing
  Write-Host "Status:`n" ($status | ConvertTo-Json -Depth 4)
} catch {
  Write-Warning "Failed to fetch /api/status: $_"
}

Write-Host "Done."