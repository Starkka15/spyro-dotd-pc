# Runs the game, saves a picture of its window at each given second, then stops only that process.
# usage: powershell -Command "& H:\spyro-dotd\tools\shot.ps1 -Name x -At 10,25"
param([string]$Name = 'shot', [double[]]$At = @(15, 30, 45), [string]$Config = 'win-amd64-relwithdebinfo', [string[]]$Extra = @())

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
$logs = 'H:\spyro-dotd\logs'
New-Item -ItemType Directory -Force $logs | Out-Null
Remove-Item "$logs\$Name.log" -ErrorAction SilentlyContinue
$args = @('--game_data_root', 'H:\spyro-dotd\game\files', '--log_file', "$logs\$Name.log", '--log_flush_interval', '1') + $Extra
$p = Start-Process -FilePath "$dir\spyro_dotd.exe" -ArgumentList $args -WorkingDirectory $dir `
    -RedirectStandardOutput "$logs\$Name.out.log" -RedirectStandardError "$logs\$Name.err.log" -PassThru
$start = Get-Date
foreach ($s in $At) {
    while (((Get-Date) - $start).TotalSeconds -lt $s) {
        if ($p.HasExited) { "exited with code $($p.ExitCode) before $s s"; return }
        Start-Sleep -Milliseconds 200
    }
    $p.Refresh()
    $r = New-Object Rect
    [void][Win]::GetWindowRect($p.MainWindowHandle, [ref]$r)
    $w = $r.R - $r.L; $h = $r.B - $r.T
    if ($w -le 0 -or $h -le 0) { "no window at $s s"; continue }
    $b = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($b)
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $b.Size)
    $b.Save(("{0}\{1}-{2:000.0}.png" -f $logs,$Name,$s))
    
}
Stop-Process -Id $p.Id -Force -Confirm:$false

