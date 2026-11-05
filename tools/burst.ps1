# Runs the game and captures its window as fast as possible (quarter size) from -From for -Secs seconds,
# then stops only that process. Frames go to logs\<Name>\NNNN.png with the time in ms in the name.
# usage: powershell -NoProfile -Command "& H:\spyro-dotd\tools\burst.ps1 -Name b1 -From 24 -Secs 4"
param([string]$Name = 'burst', [double]$From = 24, [double]$Secs = 4, [string]$Config = 'win-amd64-relwithdebinfo', [string[]]$Extra = @())

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public struct Rect { public int L, T, R, B; }
public static class Win {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out Rect r);
}
'@

$dir = "H:\spyro-dotd\out\build\$Config"
$out = "H:\spyro-dotd\logs\$Name"
Remove-Item -Recurse -Force $out -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $out | Out-Null
$args = @('--game_data_root', 'H:\spyro-dotd\game\files', '--log_file', "$out\game.log", '--log_flush_interval', '1') + $Extra
$p = Start-Process -FilePath "$dir\spyro_dotd.exe" -ArgumentList $args -WorkingDirectory $dir -PassThru
$start = Get-Date
while (((Get-Date) - $start).TotalSeconds -lt $From) {
    if ($p.HasExited) { "exited with code $($p.ExitCode) early"; return }
    Start-Sleep -Milliseconds 100
}
$p.Refresh()
$r = New-Object Rect
[void][Win]::GetWindowRect($p.MainWindowHandle, [ref]$r)
$w = $r.R - $r.L; $h = $r.B - $r.T
$full = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($full)
$frames = New-Object System.Collections.ArrayList
$t0 = Get-Date
while (((Get-Date) - $t0).TotalSeconds -lt $Secs) {
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $full.Size)
    $small = New-Object System.Drawing.Bitmap $full, ([int]($w / 4)), ([int]($h / 4))
    [void]$frames.Add(@([int]((Get-Date) - $t0).TotalMilliseconds, $small))
}
Stop-Process -Id $p.Id -Force -Confirm:$false
$i = 0
foreach ($f in $frames) { $f[1].Save(('{0}\{1:0000}-{2:00000}.png' -f $out, $i, $f[0])); $i++ }
"$i frames in $Secs s"

