$ErrorActionPreference = 'Stop'
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = & $locator -latest -products '*' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -find 'MSBuild\**\Bin\MSBuild.exe' |
    Select-Object -First 1

if (-not $msbuild) {
    throw 'Outils C++ Visual Studio introuvables.'
}

$buildArguments = @(
    (Join-Path $PSScriptRoot 'StreamDiscord.vcxproj')
    '/p:Configuration=Release'
    '/p:Platform=x64'
    '/nologo'
    '/verbosity:minimal'
)

& $msbuild @buildArguments

if ($LASTEXITCODE -ne 0) {
    throw 'La compilation a échoué.'
}
