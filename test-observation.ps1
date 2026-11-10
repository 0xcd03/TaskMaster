# Read-only process metrics coverage; optional privilege belongs only to this probe.
param([switch]$EnableDebugPrivilege)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
public static class TaskMasterObservationProbe {
  [StructLayout(LayoutKind.Sequential)] struct Luid { public uint low; public int high; }
  [StructLayout(LayoutKind.Sequential)] struct Privilege { public uint count; public Luid luid; public uint attributes; }
  [StructLayout(LayoutKind.Sequential)] struct Memory {
    public uint cb, faults;
    public ulong peakWS, WS, peakPaged, paged, peakNonpaged, nonpaged, pagefile, peakPagefile, privateBytes;
  }
  [StructLayout(LayoutKind.Sequential)] struct Io { public ulong a,b,c,d,e,f; }
  [DllImport("kernel32.dll", SetLastError=true)] static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  [DllImport("kernel32.dll", SetLastError=true)] static extern bool GetProcessTimes(IntPtr h,out long create,out long exit,out long kernel,out long user);
  [DllImport("kernel32.dll", SetLastError=true)] static extern bool GetProcessIoCounters(IntPtr h,out Io io);
  [DllImport("psapi.dll", SetLastError=true)] static extern bool GetProcessMemoryInfo(IntPtr h,ref Memory memory,uint cb);
  [DllImport("advapi32.dll", SetLastError=true)] static extern bool OpenProcessToken(IntPtr h,uint access,out IntPtr token);
  [DllImport("advapi32.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern bool LookupPrivilegeValue(string machine,string name,out Luid luid);
  [DllImport("advapi32.dll", SetLastError=true)] static extern bool AdjustTokenPrivileges(IntPtr token,bool disableAll,ref Privilege state,uint cb,IntPtr previous,IntPtr returned);
  [DllImport("advapi32.dll", SetLastError=true)] static extern bool GetTokenInformation(IntPtr token,int kind,out int value,int cb,out int required);
  [DllImport("kernel32.dll")] static extern IntPtr GetCurrentProcess();
  public static object Run(bool enableDebug) {
    bool elevated=false,debugEnabled=false;int debugError=0;
    IntPtr token;
    if(OpenProcessToken(GetCurrentProcess(),0x28,out token)) {
      try {
        int value,size; if(GetTokenInformation(token,20,out value,4,out size))elevated=value!=0;
        if(enableDebug) {
          Luid id;
          if(LookupPrivilegeValue(null,"SeDebugPrivilege",out id)) {
            var privilege=new Privilege{count=1,luid=id,attributes=2};
            bool success=AdjustTokenPrivileges(token,false,ref privilege,0,IntPtr.Zero,IntPtr.Zero);
            debugError=Marshal.GetLastWin32Error();debugEnabled=success&&debugError==0;
          } else debugError=Marshal.GetLastWin32Error();
        }
      } finally {CloseHandle(token);}
    }
    int total=0, oldOpen=0, limitedOpen=0, cpu=0, io=0, memory=0, self=Process.GetCurrentProcess().Id;
    bool selfGood=false;
    foreach(var p in Process.GetProcesses()) {
      using(p) {
        int id; try{id=p.Id;}catch{continue;}
        total++;
        var old=OpenProcess(0x410,false,id);
        if(old!=IntPtr.Zero){oldOpen++;CloseHandle(old);}
        var h=OpenProcess(0x1000,false,id);
        if(h==IntPtr.Zero)continue;
        try {
          limitedOpen++;
          long c,e,k,u;
          bool times=GetProcessTimes(h,out c,out e,out k,out u);
          if(times)cpu++;
          Io data;bool ioGood=GetProcessIoCounters(h,out data);if(ioGood)io++;
          var m=new Memory{cb=80}; bool mem=GetProcessMemoryInfo(h,ref m,80);
          if(!mem){var full=OpenProcess(0x410,false,id);if(full!=IntPtr.Zero){try{mem=GetProcessMemoryInfo(full,ref m,80);}finally{CloseHandle(full);}}}
          if(mem)memory++;
          if(id==self)selfGood=times&&ioGood&&mem&&m.WS>0;
        } finally {CloseHandle(h);}
      }
    }
    if(!selfGood)throw new Exception("Least-privilege self metrics failed.");
    return new {Elevated=elevated,DebugPrivilegeRequested=enableDebug,DebugPrivilegeEnabled=debugEnabled,DebugPrivilegeError=debugError,SnapshotProcesses=total,OldPolicyOpened=oldOpen,LimitedQueryOpened=limitedOpen,CpuTimeAvailable=cpu,IoAvailable=io,MemoryAvailable=memory,SelfMetricsPassed=selfGood};
  }
}
'@
[TaskMasterObservationProbe]::Run($EnableDebugPrivilege.IsPresent)
