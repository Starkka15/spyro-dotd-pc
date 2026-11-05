# Regenerates the recompiled C++ and rebuilds spyro_dotd.exe.
# usage: powershell -NoProfile -File H:\spyro-dotd\tools\regen.ps1 [-Scan] [-Config win-amd64-relwithdebinfo]
#   -Scan  re-runs tools\find_funcs.py first (needs game\image.bin from SPYRO_DUMP_IMAGE): a codegen
#          with only config\manual.toml gives the analyzer's own function list (config\baseline_init.cpp),
#          and the code pointers it missed go to config\found_functions.toml.
param([switch]$Scan, [string]$Config = 'win-amd64-relwithdebinfo')
$ErrorActionPreference = 'Stop'
$root = 'H:\spyro-dotd'
$rex = 'H:\rexglue-sdk\out\install\win-amd64\bin\rexglue.exe'
$py = 'G:\Python310\python.exe'
Set-Location $root
$manifest = "$root\spyro_dotd_manifest.toml"
if ($Scan) {
    $orig = [IO.File]::ReadAllText($manifest)
    try {
        [IO.File]::WriteAllText($manifest, $orig.Replace('"config/found_functions.toml", ', ''))
        cmd /c "`"$rex`" codegen spyro_dotd_manifest.toml > codegen-base.log 2>&1"
        if ($LASTEXITCODE) { throw "baseline codegen failed ($LASTEXITCODE), see codegen-base.log" }
        Copy-Item generated\default\spyro_dotd_init.cpp config\baseline_init.cpp -Force
    } finally {
        [IO.File]::WriteAllText($manifest, $orig)
    }
    & $py -I tools\find_funcs.py game\image.bin config\baseline_init.cpp --toml config\found_functions.toml > config\find_funcs.txt
    Select-String -Path config\find_funcs.txt -Pattern 'registered' | ForEach-Object Line
}
cmd /c "`"$rex`" codegen spyro_dotd_manifest.toml > codegen.log 2>&1"
if ($LASTEXITCODE) { throw "codegen failed ($LASTEXITCODE), see codegen.log" }
$unresolved = (Select-String -Path generated\default\*.cpp -Pattern 'REX_FATAL\("Unresolved').Count
"codegen OK, $unresolved unresolved branches"
cmd /c "$root\tools\rexenv.cmd cmake --build --preset $Config -j 8 > build.log 2>&1"
if ($LASTEXITCODE) { Select-String -Path build.log -Pattern ' error' | Select-Object -First 10 | ForEach-Object Line; throw "build failed, see build.log" }
"build OK: $root\out\build\$Config\spyro_dotd.exe"
