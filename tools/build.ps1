param([Parameter(Mandatory=$true)][string]$Input,[Parameter(Mandatory=$true)][string]$Out)
python zhcc.py $Input -o $Out
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "完成：$Out"
