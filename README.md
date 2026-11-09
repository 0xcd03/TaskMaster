# TaskManagerX64

Native Win32 process manager written in MASM x64. It uses Visual Studio Build Tools (`ml64.exe`, `link.exe`, `rc.exe`) and the Windows SDK; no IDE or external runtime is required.

## Binaries

- `TaskManagerX64.exe` — release configuration with the embedded `requireAdministrator` manifest.
- `TaskManagerX64-standard.exe` — same code and resources with the `asInvoker` preview manifest for non-elevated inspection.
- `TaskManagerX64-admin.exe` — copy of the administrator build for explicit naming.

The binaries are unsigned. Run the supplied `TaskManagerX64.exe` when process and service control is needed; UAC is expected.

## Build

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-taskmanager.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-taskmanager.ps1 -Preview -OutputPath .\outputs\TaskManagerX64-standard.exe
powershell -NoProfile -ExecutionPolicy Bypass -File .\verify-build.ps1 -Path .\outputs\TaskManagerX64.exe
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-tests.ps1
```

`build-resources.ps1` generates UTF-16 Windows resources and the MASM string buffers from `res/ui-strings.json`. The generator checks RU/EN format arguments and length limits. English and Russian menus, process/thread/service/user/performance/startup/history labels, warning text and error text are covered.

## Implemented completion work

- Process topology: parent PID plus creation time, validated ancestry, expandable/collapsible process tree, ancestor-preserving search, aggregate commit/working-set columns with a shared-page warning, tree actions for terminate/pause/resume/trim.
- Direct2D/DirectWrite: five rolling series (CPU, RAM, GPU, disk, network), D2D fallback text, device-loss target recreation, unavailable counter gaps, high-DPI layout and font rebuilding, minimized-window sampling reduction.
- Startup: HKCU/HKLM Run and RunOnce in 32/64-bit views, user/common Startup folders, recursive Task Scheduler enumeration with enabled state and access errors. Disable/restore moves registry values into a private reversible key and files into a private sibling folder after identity/hash validation. Task enablement uses the scheduler COM API.
- History: observed process identity (PID + birth), CPU, peak commit, I/O read/write and sample counts are persisted in `%LOCALAPPDATA%\TaskManagerX64\history.bin` through a temporary file, flush, atomic replacement and a schema/checksum check. Corrupt or truncated files are rejected. History is explicitly limited to observations made by this program.
- Reliability: background collection, wake-on-command switching, process and TID identity checks, foreign suspend-count preservation, fail-closed protected-process guards, formatted Win32/HRESULT messages, PE/map verification, bounded strings, and large-row limits.

## Validation

The model test suite passed: stale PID identity; pause/resume/trim/priority/terminate on disposable target processes; RU/EN resource loading; 5000-row topology; collapse and ancestor search; PID reuse; persistent history; checksum corruption; registry and Startup-folder round trips; chart ring buffer; high-DPI scale calculation; Task Scheduler enumeration; and an independent COM-verified Task Scheduler disable/enable round trip. The final PE verifier passed for both binaries: AMD64, PE32+ GUI, `SimpleEntry` RVA `0x131B`, ASLR/NX flags, and 77,312-byte files.

Computer-use verification observed the final localized process screen, Direct2D performance screen, startup sources (Run/RunOnce/folder/task entries), and persistent history screen. A final relaunch after the last non-visual hardening changes was stopped by the physical Escape key, so that exact post-hardening relaunch is not claimed as visually verified. Windows 11 build 22631 was used for the observed run; Windows 10 remains to be tested separately.

Memory trim reduces resident working set; it does not reduce private commit and can cause later page faults. Per-process network attribution is intentionally not claimed: the performance panel reports interface totals. GPU and PDH values may be unavailable on a specific machine and are shown as `N/A`/`нет данных`.