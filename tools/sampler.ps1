# Samples one thread of a running process from outside: suspend, read RIP + top of stack, resume.
# Writes "count<TAB>leaf<TAB>caller" lines, where leaf is module+RVA of RIP and caller is the first
# address on the stack that falls inside the main exe (so samples in DLLs/ntdll get blamed on game code).
# usage: powershell -NoProfile -File sampler.ps1 -ProcessId 1234 [-ThreadId 0 (busiest)] -Secs 20 -Out prof.tsv
param([int]$ProcessId, [int]$ThreadId = 0, [double]$Secs = 20, [string]$Out = 'prof.tsv')

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
public static class Sampler {
  [DllImport("kernel32.dll")] static extern IntPtr OpenThread(int access, bool inherit, int id);
  [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(int access, bool inherit, int id);
  [DllImport("kernel32.dll")] static extern int SuspendThread(IntPtr h);
  [DllImport("kernel32.dll")] static extern int ResumeThread(IntPtr h);
  [DllImport("kernel32.dll")] static extern bool GetThreadContext(IntPtr h, IntPtr ctx);
  [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr p, IntPtr a, byte[] b, IntPtr n, out IntPtr r);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  [DllImport("winmm.dll")] static extern int timeBeginPeriod(int ms);

  public static string Run(int pid, int tid, double secs, string[] modNames, long[] modBase, long[] modSize, int exeIndex) {
    timeBeginPeriod(1);
    IntPtr th = OpenThread(0x0002 | 0x0008 | 0x0040, false, tid);
    IntPtr ph = OpenProcess(0x0010 | 0x0400, false, pid);
    if (th == IntPtr.Zero || ph == IntPtr.Zero) return "open failed";
    IntPtr raw = Marshal.AllocHGlobal(1232 + 16);
    IntPtr ctx = new IntPtr((raw.ToInt64() + 15) & ~15L);
    var counts = new Dictionary<string, int>();
    byte[] stack = new byte[1024];
    var sw = Stopwatch.StartNew();
    int samples = 0;
    Func<long, string> name = a => {
      for (int i = 0; i < modBase.Length; i++)
        if (a >= modBase[i] && a < modBase[i] + modSize[i]) return modNames[i] + "+" + (a - modBase[i]).ToString("X");
      return "?" + a.ToString("X");
    };
    long eb = modBase[exeIndex], ee = eb + modSize[exeIndex];
    while (sw.Elapsed.TotalSeconds < secs) {
      if (SuspendThread(th) == -1) break;
      for (int i = 0; i < 1232; i += 8) Marshal.WriteInt64(ctx, i, 0);
      Marshal.WriteInt32(ctx, 0x30, 0x100003);  // CONTEXT_CONTROL | CONTEXT_INTEGER
      bool ok = GetThreadContext(th, ctx);
      long rip = Marshal.ReadInt64(ctx, 0xF8), rsp = Marshal.ReadInt64(ctx, 0x98);
      IntPtr got;
      bool sok = ok && ReadProcessMemory(ph, new IntPtr(rsp), stack, new IntPtr(stack.Length), out got);
      ResumeThread(th);
      if (ok) {
        string caller = "-";
        if (sok) {
          for (int i = 0; i + 8 <= stack.Length; i += 8) {
            long v = BitConverter.ToInt64(stack, i);
            if (v >= eb && v < ee) { caller = name(v); break; }
          }
        }
        string key = name(rip) + "\t" + caller;
        int c; counts.TryGetValue(key, out c); counts[key] = c + 1;
        samples++;
      }
      System.Threading.Thread.Sleep(1);
    }
    CloseHandle(th); CloseHandle(ph); Marshal.FreeHGlobal(raw);
    var sb = new System.Text.StringBuilder();
    var list = new List<KeyValuePair<string, int>>(counts);
    list.Sort((a, b) => b.Value.CompareTo(a.Value));
    foreach (var kv in list) sb.Append(kv.Value).Append('\t').Append(kv.Key).Append('\n');
    return "samples " + samples + "\n" + sb.ToString();
  }
}
'@

$p = Get-Process -Id $ProcessId
if ($ThreadId -eq 0) {
  # Busiest thread over one second.
  $t1 = @{}; $p.Threads | ForEach-Object { $t1[$_.Id] = $_.TotalProcessorTime.TotalMilliseconds }
  Start-Sleep -Milliseconds 1000; $p.Refresh()
  $ThreadId = ($p.Threads | Sort-Object { $_.TotalProcessorTime.TotalMilliseconds - $t1[$_.Id] } -Descending | Select-Object -First 1).Id
}
$mods = @($p.Modules)
$exeIndex = [array]::IndexOf(($mods | ForEach-Object { $_.FileName }), $p.MainModule.FileName)
$r = [Sampler]::Run($ProcessId, $ThreadId, $Secs, [string[]]($mods | ForEach-Object { $_.ModuleName }),
  [long[]]($mods | ForEach-Object { $_.BaseAddress.ToInt64() }), [long[]]($mods | ForEach-Object { [long]$_.ModuleMemorySize }), $exeIndex)
"thread $ThreadId" | Set-Content -Encoding utf8 $Out
$r | Add-Content -Encoding utf8 $Out
"thread $ThreadId, " + ($r -split "`n")[0]
