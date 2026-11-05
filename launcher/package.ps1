# Builds the launcher, stages the install folder and makes Setup.
# usage: launcher\package.ps1 [-NoSetup] [-Sdk <rexglue-sdk checkout>]
# Needs: a built spyro_dotd.exe (out\build\win-amd64-relwithdebinfo, with the
# SDK DLLs beside it), Visual Studio 2022 (CRT redistributables) and Inno Setup 6
# (ISCC.exe on PATH, or -Iscc).
# Output: release\stage (the install folder; it never holds game files) and
#         release\spyro_dotd-PC-<version>-Setup.exe
param([switch]$NoSetup, [string]$Sdk = '..\rexglue-sdk', [string]$Iscc = 'ISCC.exe')
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$repo = Split-Path $here
$Game = 'spyro_dotd'
$g = @{ Guid = '3B7E2F41-6A0C-4D8B-9E15-7C2D4F8A6B93'; Title = 'The Legend of Spyro - Dawn of the Dragon';
        Disc = 'The Legend of Spyro: Dawn of the Dragon for the Xbox 360 (USA/Europe)';
        DiscText = 'about 6.7 GB'; Bytes = 7200000000 }
if (-not [IO.Path]::IsPathRooted($Sdk)) { $Sdk = Join-Path $repo $Sdk }
$version = ([regex]'GAME_VERSION\s+"([^"]+)"').Match((Get-Content "$here\games\$Game.h" -Raw)).Groups[1].Value

# 1. Launcher
cmd /c "`"$here\build.cmd`" $Game"
if ($LASTEXITCODE) { throw 'launcher build failed' }

# 2. Stage: the game exe and its SDK DLLs (no PDBs), the VC++ runtime
#    app-local, the launcher, README and licenses. No game files.
$bin = Join-Path $repo 'out\build\win-amd64-relwithdebinfo'
$stage = Join-Path $repo 'release\stage'
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item "$bin\$Game.exe" $stage
foreach ($dll in 'rexruntimerd.dll', 'rexgpu-xenosrd.dll', 'TracyClientrd.dll') { Copy-Item "$bin\$dll" $stage }
$crt = Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\2022\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT' -Directory |
    Sort-Object FullName | Select-Object -Last 1
foreach ($dll in 'msvcp140.dll', 'msvcp140_atomic_wait.dll', 'vcruntime140.dll', 'vcruntime140_1.dll') {
    Copy-Item (Join-Path $crt.FullName $dll) $stage
}
Copy-Item "$here\build\$Game\${Game}_launcher.exe" $stage

$readme = (Get-Content "$here\installer\README.template.txt" -Raw).
    Replace('{TITLE}', $g.Title).Replace('{ID}', $Game).Replace('{DISC}', $g.Disc).Replace('{VERSION}', $version)
$underline = '=' * ($g.Title.Length + ' -- PC port '.Length + $version.Length)
$readme = ($readme -split "`r?`n", 3)
$readme = @($readme[0], $underline, $readme[2]) -join "`r`n"
[IO.File]::WriteAllText("$stage\README.txt", $readme.Replace("`r`n", "`n").Replace("`n", "`r`n"))

# LICENSES.txt: this port's, the SDK's, then each bundled library's own license file.
$sb = [Text.StringBuilder]::new()
[void]$sb.AppendLine("Licenses of the software in this port").AppendLine()
[void]$sb.AppendLine('=' * 72).AppendLine('This port (GPL-3.0)').AppendLine('=' * 72)
[void]$sb.AppendLine((Get-Content "$repo\LICENSE" -Raw))
[void]$sb.AppendLine('=' * 72).AppendLine('ReXGlue SDK').AppendLine('=' * 72)
[void]$sb.AppendLine((Get-Content "$Sdk\LICENSE" -Raw))
foreach ($d in Get-ChildItem "$Sdk\thirdparty" -Directory | Where-Object Name -notin 'catch2') {
    $lic = Get-ChildItem $d.FullName -File | Where-Object Name -match '^(LICENSE|LICENCE|COPYING|COPYRIGHT)' |
        Sort-Object Name | Select-Object -First 1
    if (-not $lic) { continue }
    [void]$sb.AppendLine('=' * 72).AppendLine("$($d.Name) ($($lic.Name))").AppendLine('=' * 72)
    [void]$sb.AppendLine((Get-Content $lic.FullName -Raw))
}
[void]$sb.AppendLine('=' * 72).AppendLine('Microsoft Visual C++ runtime').AppendLine('=' * 72)
[void]$sb.AppendLine('msvcp140*.dll and vcruntime140*.dll are redistributed under the Microsoft Visual Studio license terms.')
[IO.File]::WriteAllText("$stage\LICENSES.txt", $sb.ToString().Replace("`r`n", "`n").Replace("`n", "`r`n"))

"staged $stage"
if ($NoSetup) { return }

# 3. Setup
& $Iscc /Q "/DGameId=$Game" "/DAppTitle=$($g.Title)" "/DAppVersion=$version" "/DAppGuid=$($g.Guid)" `
    "/DStageDir=$stage" "/DOutDir=$(Join-Path $repo 'release')" "/DIconFile=$repo\res\$Game.ico" `
    "/DDiscText=$($g.DiscText)" "/DGameBytes=$($g.Bytes)" "$here\installer\rexgame.iss"
if ($LASTEXITCODE) { throw 'ISCC failed' }
Get-ChildItem (Join-Path $repo 'release') -Filter '*Setup.exe' | Select-Object Name, Length, LastWriteTime
