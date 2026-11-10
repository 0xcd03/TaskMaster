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
powershell -NoProfile -ExecutionPolicy Bypass -File .\test-ui-regression.ps1
```

`build-resources.ps1` generates UTF-16 Windows resources and the MASM string buffers from `res/ui-strings.json`. The generator checks RU/EN format arguments and length limits. English and Russian menus, process/thread/service/user/performance/startup/history labels, warning text and error text are covered.

## Implemented completion work

- Process topology: parent PID plus creation time, validated ancestry, expandable/collapsible process tree, ancestor-preserving search, aggregate commit/working-set columns with a shared-page warning, tree actions for terminate/pause/resume/trim.
- Direct2D/DirectWrite: five rolling series (CPU, RAM, GPU, disk, network), D2D fallback text, device-loss target recreation, unavailable counter gaps, high-DPI layout and font rebuilding, minimized-window sampling reduction.
- Startup: HKCU/HKLM Run and RunOnce in 32/64-bit views, user/common Startup folders, recursive Task Scheduler enumeration with enabled state and access errors. Disable/restore moves registry values into a private reversible key and files into a private sibling folder after identity/hash validation. Task enablement uses the scheduler COM API.
- History: observed process identity (PID + birth), CPU, peak commit, I/O read/write and sample counts are persisted in `%LOCALAPPDATA%\TaskManagerX64\history.bin` through a temporary file, flush, atomic replacement and a schema/checksum check. Corrupt or truncated files are rejected. History is explicitly limited to observations made by this program.
- Reliability: background collection, wake-on-command switching, process and TID identity checks, foreign suspend-count preservation, fail-closed protected-process guards, formatted Win32/HRESULT messages, PE/map verification, bounded strings, and large-row limits.

## Validation

The model test suite passed: stale PID identity; pause/resume/trim/priority/terminate on disposable target processes; RU/EN resource loading; 5000-row topology; collapse and ancestor search; PID reuse; persistent history; checksum corruption; registry and Startup-folder round trips; chart ring buffer; high-DPI scale calculation; Task Scheduler enumeration; and an independent COM-verified Task Scheduler disable/enable round trip. The final PE verifier passed for both binaries: AMD64, PE32+ GUI, the linker-map-confirmed `SimpleEntry`, ASLR/NX flags, and a size limit of 256 KiB (see the current verification report for exact size).

Computer-use verification observed the final localized process screen, Direct2D performance screen, startup sources (Run/RunOnce/folder/task entries), and persistent history screen. A final relaunch after the last non-visual hardening changes was stopped by the physical Escape key, so that exact post-hardening relaunch is not claimed as visually verified. Windows 11 build 22631 was used for the observed run; Windows 10 remains to be tested separately.

Memory trim reduces resident working set; it does not reduce private commit and can cause later page faults. Per-process network attribution is intentionally not claimed: the performance panel reports interface totals. GPU and PDH values may be unavailable on a specific machine and are shown as `N/A`/`нет данных`.

The collector prioritizes the requested panel and cancels obsolete navigation generations. Process history follows the refresh interval on Processes; on other panels, it takes a background observation at most once every five seconds after publishing the visible panel. Navigation can cancel that observation, so recorded history can contain gaps and is not a continuous trace. Task Scheduler and shell icon discovery are not prerequisites for service, user, performance, or thread snapshots.

Process icons load on one separate worker through a 32-request queue and a bounded PID-plus-creation-time cache. Rows use the generic EXE icon until a result is ready; queue overflow is retried later, and executable paths over 1023 characters keep the generic icon. A slow shell icon handler cannot hold the collector or the UI, and application exit never waits for that handler.
## Application icon and signing

`make-icon.ps1` draws the TaskMaster icon and writes a native ICO container with 16, 20, 24, 32, 40, 48, 64, 128 and 256 px PNG frames. Resource 103 embeds it in the executable; the window class loads the same resource for its large/small icons. The new icon is a stylized circular shield with a light rim, graphite interior and orange T crest, rendered from code at all supported sizes.

The application is currently UNSIGNED: no publisher Code Signing certificate is available. A self-signed certificate is not a trusted publisher signature and would not establish antivirus reputation. Even a trusted signature cannot guarantee that antivirus software will never flag process/thread control operations.

To sign after obtaining a Code Signing certificate with an accessible private key in Windows My store:

```powershell
.\sign-release.ps1 -CertificateThumbprint <40-hex-thumbprint> -Files .\outputs\TaskManagerX64.exe,.\outputs\TaskManagerX64-standard.exe
```

Use `-LocalMachine` for the machine certificate store. The script uses Windows SDK SignTool, SHA-256 and an RFC 3161 timestamp, then verifies with Authenticode policy. It does not export private keys or change Windows trust settings. Repackage release ZIP files after signing. Token/cloud certificates may require provider-specific setup.

## Interface and responsiveness update

- A DPI-scaled icon sidebar highlights the selected section immediately. Switching is independent of the data collector and remains available while actions run. Mode plus generation identifiers reject stale snapshots. `WM_GETMINMAXINFO` now writes the minimum tracking size at the correct offsets, leaving maximize tracking unrestricted.
- Process columns are Name, Status, CPU, resident Working Set in MiB, read+write I/O in KiB/s, Network and PID. Values have numeric sorting, executable icons and resource heat coloring. I/O includes process file/device operations and is not a physical-disk measurement. Network is explicitly unavailable until a supported per-process attribution collector exists. Working-set sums count shared pages again.
- Menu bar and popups, search edit, list header, sidebar, list cells and the list's native scrollbars follow light/dark colors. The scrollbar track, thumb, arrows and corner are repainted through documented Win32 scrollbar geometry after native processing; hit testing, dragging, keyboard input and accessibility remain native.
- The PC boost card measures total physical RAM use and the latest before/after percentage. Its button runs guarded `EmptyWorkingSet` operations on a worker and reports successes, failures, skips, measured working sets and private commit. It is a working-set trim, not forced Windows memory compression.
- Full-list selection resets and duplicate invalidations on every snapshot were removed. Shell icon extraction runs on a separate bounded worker. Requested tabs are collected first; stale enumerations cancel at entry boundaries.

`test-ui-regression.ps1` builds and runs a hidden test window using the actual controls. It covers immediate navigation while busy, delayed stale snapshots, 5000 displayed fixture rows across multiple accepted updates, message-response latency and light/dark color paths. It never modifies user settings or trims user processes. The final interactive visual pass was stopped with Escape; the latest complete build is verified by automated tests rather than claimed as visually inspected.

After building and validating both configurations, `package-release.ps1` creates release and source ZIPs from an explicit source set. It includes the current `outputs/verification.txt`, icon and SHA-256 checksums in the release, and excludes local history, settings, Git internals and scratch files.

The current UI revision also extends the table to the client edge, paints the complete header/background area with the active theme, and keeps the result panel inside the application layout. Processes, services and startup entries expose their actions through a row context menu. Startup rows use native checkboxes whose checked state is tied to the actual enabled/disabled source; changes still pass through the existing identity and hash validation. Settings menu entries show a check mark for the active language, theme, refresh interval and tree mode. Memory optimization reports its result in the themed bottom panel and exposes the full diagnostic text through `Details`.

The process tree uses the ListView's native owner-data rows and shell application icons, with vertical hierarchy guides and expand/collapse branch glyphs. Every child remains an individual actionable process. A startup crash in v0.2.1 was traced to an incorrect HWND passed where the row model pointer was required during `LVN_GETDISPINFO`; the release build now passes the model row and rejects null row mappings before dereferencing them.
