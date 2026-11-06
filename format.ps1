param(
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
$formatterCommand = Get-Command clang-format -ErrorAction SilentlyContinue

if ($formatterCommand) {
    $formatter = $formatterCommand.Source
}
else {
    $locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $installation = & $locator -latest -products '*' -property installationPath
    $formatter = Join-Path $installation 'VC\Tools\Llvm\bin\clang-format.exe'
}

if (-not (Test-Path -LiteralPath $formatter)) {
    throw 'clang-format est introuvable. Installer les outils LLVM de Visual Studio ou ajouter clang-format au PATH.'
}

$sourceDirectory = Join-Path $PSScriptRoot 'src'
$files = Get-ChildItem -LiteralPath $sourceDirectory -File |
    Where-Object { $_.Extension -in '.cpp', '.h' } |
    Select-Object -ExpandProperty FullName

if ($Check) {
    & $formatter --dry-run --Werror @files
}
else {
    & $formatter -i @files
}

if ($LASTEXITCODE -ne 0) {
    throw 'La vérification du formatage a échoué.'
}
