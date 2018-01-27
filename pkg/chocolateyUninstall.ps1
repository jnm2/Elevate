$ErrorActionPreference = 'Stop'

$installDir = "$Env:ProgramFiles\Elevate"

Remove-Item $installDir -Force

# Does not yet exist – https://github.com/chocolatey/choco/pull/1019#issuecomment-360958196
Uninstall-ChocolateyPath $installDir -PathType Machine
