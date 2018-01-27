$ErrorActionPreference = 'Stop'

# Path type and installation directory package parameters can be re-added
# when https://github.com/chocolatey/choco/issues/1479 is answered.

$installDir = "$Env:ProgramFiles\Elevate"
$toolsPath = Split-Path -Parent $MyInvocation.MyCommand.Definition
$platform = if (Get-OSArchitectureWidth 64) { 'x64' } else { 'x86' }

New-Item $installDir -ItemType Directory -Force | Out-Null

Get-ChildItem "$toolsPath/$platform" |
    Move-Item -Destination $installDir -Force

# Save disk space and prevent shimming. Shimming messes up Elevate.exe's
# ability to detect the shell that started it.
Remove-Item "$toolsPath/*" -Recurse -Exclude *.ps1

Install-ChocolateyPath $installDir -PathType Machine
