# Local development (Windows x64)

Repository: https://github.com/0xcd03/TaskMaster (branch `main`).

## Toolchain

The existing installation was sufficient; no additional packages were needed:

- Git for Windows.
- Visual Studio Build Tools 2022, C++ x86/x64 tools, MSVC/MASM 14.44.35207.
- Windows SDK 10.0.26100.0 (10.0.19041.0 is also installed).
- Windows PowerShell 5.1, PowerShell 7.6.9 and VS Code.

Build scripts locate Visual Studio with `vswhere` and call `vcvars64.bat` themselves.
Do not add a specific compiler version to the global PATH. No Python, Node.js,
CMake, MASM32 package, or third-party runtime is required.

## Build and run

Run in the repository directory:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-taskmanager.ps1 -Preview -OutputPath .\work\TaskManagerX64-standard.exe
.\work\TaskManagerX64-standard.exe
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-taskmanager.ps1 -OutputPath .\work\TaskManagerX64.exe
```

The standard executable runs without elevation. The administrator build requests UAC.
Local builds go into ignored `work/`; the downloaded release files in `outputs/`
are preserved. Resource generation can change the tracked ICO/PNG on different
machines; review these generated changes before committing.

In VS Code, Ctrl+Shift+B builds the standard executable. Terminal > Run Task
also provides the administrator build, model tests, UI regression tests, and
`TaskMaster: verify development` for the full development validation.

## Verify

Rebuild both local configurations and run all current development checks:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\verify-development.ps1
```

This runs PE verification, the native-runner fixtures, model/action/Scheduler,
UI, integration, service-pagination, thread-CPU, new-task, native/optional process-details, process-columns, process-metadata and isolated packaging
checks sequentially. It stops
on the first failure and saves individual logs plus `summary.json` under
`work/validation/`. Skipped subtests are recorded as `PARTIAL` with their reason
and make `summary.json`'s `Complete` false, even when the script exits zero.
A completed green run validates these implemented behaviors;
it does not establish full product parity. See [the product roadmap](docs/ROADMAP.md).

Individual checks:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\verify-build.ps1 -Path .\work\TaskManagerX64-standard.exe
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\verify-build.ps1 -Path .\work\TaskManagerX64.exe
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-tests.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-ui-regression.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-integration.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-services.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-thread-metrics.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-new-task.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-process-details.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-process-details-optional.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-process-columns.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-process-metadata.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-package-release.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-native-test-process.ps1
```

Build before running model tests: they reuse `taskmanager.obj` and generated
resources. Model/action tests use disposable process, registry, folder and task
fixtures. Scheduler mutation tests can report SKIP when task creation is denied.
Integration tests use isolated fixtures, not the production Task Manager redirect.
Authenticode signing needs a separate publisher certificate and is not needed
for development.

## Initial setup validation (2026-11-10, upstream commit 4a4f755)

- Administrator and standard builds: PASS, AMD64 / PE32+, 187904 bytes each.
- PE entry point and ASLR / high-entropy VA / NX validation: PASS for both.
- Self-test, model tests and independent Scheduler round trip: PASS.
- Isolated integration tests: PASS.
- UI regression: FAIL at stage 135 (native header offscreen theme rendering,
  `src/ui_theme_regression.inc`). The log reports `exit=135`, 2 theme assertions
  and 13 control assertions. Later UI test stages were not reached.
  The wrapper returned success despite this log; inspect the log, not only the
  PowerShell exit code. Log: `work/ui-regression.log`.

These are historical baseline results before the development changes. The UI
test fixture and the Windows PowerShell 5.1 false-success runner have since
been repaired; use a fresh `verify-development.ps1` report for the current tree.

## Verified monitoring checkpoint (2026-11-10, before new-task feature)

The complete ten-step `verify-development.ps1` run passed on Windows 11 Pro
build 26300 with Windows PowerShell 5.1.26100.9711. No coverage was skipped.
Both local executables were 197632 bytes at that checkpoint. The generated
`work/validation/` logs are replaced by subsequent runs.

The service test compared 344 entries across 13 small pages. Thread metrics
passed 44 deterministic assertions plus live busy/idle sampling. UI tests
passed 13 theme and 25 control assertions, including actual sidebar button
notifications, 24 navigation checks and 5000 rows. The independent fixture
tests preserve meaningful failure handling; none of this proves all product
features or all Windows versions are complete.

Thread CPU is a share of the machine's total active logical-processor capacity,
displayed in tenths of a percent. Windows time counters are quantized: small
one-core overruns within two measured clock increments (capped at 32 ms) are
clamped to the elapsed one-core interval. Larger impossible deltas are unknown.
The bounded 8192-slot cache starts fresh on identity/navigation/topology changes,
read failures and gaps over 30 seconds; collisions can cause one N/A sample.

The ordinary GUI was opened for inspection, but Computer Use was stopped with
physical Escape. Its smoke process was closed before the complete rebuild.
No completed desktop interaction/visual verification is claimed for this checkpoint.

## Verified new-task checkpoint (2026-11-10)

The complete twelve-step verification passed without SKIP at
`2026-11-10T20:10:24.2190322Z`: `work/validation/summary.json` has
`ExecutedAllSteps=true`, `Complete=true`. Both AMD64/PE32+ GUI executables are
210944 bytes; entry, ASLR/high-entropy/NX and the 256 KiB size gate passed.

Run-new-task coverage includes 26 native launch cases, 17 independently read
child-report assertions and 62 native dialog assertions across six EN/RU dialogs.
Normal launch validates exact Unicode/quoted arguments, cwd, token inheritance,
excluded inheritable handle and the full command-line boundary. Elevation tests
check separate API fields, cancellation/errors, closed handles and balanced COM
without displaying UAC. The UI suite exercises command 2500 while idle/busy and
all earlier navigation/theme/control regressions. Independent code review found
no remaining actionable issues after fixes. No interactive UAC or desktop visual
verification is claimed.

Packaging checks use isolated copies of these development binaries and current
sources. Required-input failures preserve existing artifacts; portable ZIP names,
contents and SHA-256 manifests pass. Downloaded `outputs/` files remain unchanged.
The overall Task Manager/PC Manager roadmap remains in progress.

## Verified native process-details checkpoint (2026-11-11)

All thirteen aggregate steps passed without SKIP at
`2026-11-11T09:27:03.3214524Z`. Both executables were 227328 bytes and passed
the PE/security/256 KiB gates. The details suite passed in PowerShell 5.1 and 7:
155 native collector assertions, 269 worker assertions, 62 rate cases / 744
assertions, 161 hidden UI assertions, and the selected-row command route in the
full UI regression. Tests cover identity, bounded mailboxes, rates, read failures,
EN/RU, theme, per-window DPI and keyboard routing. Work-only mutations that
bypassed birth/cookie checks or corrupted an I/O counter were detected.

This checkpoint covers the native details window. Account lookup and WMI
command-line collection are the next implementation phase. Manual desktop,
accessibility, Windows 10, forced path-overflow and live process-rate load
transition coverage are not established by this checkpoint.

## Verified process-details optional checkpoint (2026-11-11)

All fourteen aggregate steps passed without SKIP at
`2026-11-11T10:06:10.0079077Z`; `ExecutedAllSteps=true`, `Complete=true`.
Both executables are 237056 bytes and pass the AMD64/PE32+, entry-point,
ASLR/high-entropy VA/NX and 256 KiB gates. The standard executable is
`work/TaskManagerX64-standard.exe`; the administrator build is
`work/TaskManagerX64.exe`.

The window now includes account name and command line. A separate global worker
uses token lookup and supported Windows WMI, verifies PID/creation identity
before and after reading, and accepts optional results only within the current
three-second epoch. Slow provider calls can outlast that deadline; UI timeout
does not terminate those calls or create replacement threads. Native monitoring
continues independently. Refresh replaces the optional epoch without resetting
valid native rates. Confirmed exit/mismatch is terminal until an explicit retarget.

The final suite passed in Windows PowerShell 5.1 and PowerShell 7.6.9: optional
collector 263 assertions, worker 306, UI 173; live WMI and account checks passed
on an owned child with Unicode/spaces and an 8192-character argument tail.
The command line matched the child's independent `GetCommandLineW` value.
A real 2.5-second gated provider test kept the message loop responsive while
accepting at least five native samples. Native UI now passes 195 assertions,
including rejection of late data/errors after confirmed exit. Independent
reviews found no remaining material issue in the scoped native/optional window.

The aggregate also passed every earlier monitoring, launch, UI, integration and
packaging check. Downloaded `outputs/` artifacts are unchanged. Full configurable
tables, additional process/hardware telemetry and PC Manager features remain on
the roadmap. This checkpoint does not add interactive desktop/accessibility,
Windows 10, live process-rate load-transition or forced path-overflow evidence.

## Verified configurable-columns checkpoint (2026-11-11)

All fifteen aggregate steps passed without SKIP at
`2026-11-11T11:01:15.7839486Z`; `ExecutedAllSteps=true`, `Complete=true`.
Both executables are 246272 bytes and pass the AMD64/PE32+, entry-point,
ASLR/high-entropy VA/NX and 256 KiB gates. The new full feature gate has six
actual-main fixture entries, including the complete hidden UI regression.

The picker passed 421 assertions across twenty EN/RU light/dark dialogs;
main routing passed 25 assertions and three openings, including actionBusy,
header context menu, semantic clicks beyond physical index6, actual display
callbacks, selected PID/birth, coalesced capture, owned-file persistence and
stale-mode messages. Native tests cover real ListView/header subclasses,
96/144/192-DPI widths, numeric QWORD sorting, unknown values and identity ties.
The store has an independent exact-128-byte oracle, second-process load,
nineteen malformed records plus missing-file rejection and failed-replacement
preservation/cleanup. The native and optional details suites also passed.

The aggregate can be launched from PowerShell7 while its children use Windows
PowerShell5.1: child-only environment overrides remove the inherited Core
PSModulePath so Windows PowerShell rebuilds its Desktop defaults. The shared
runner now has nine cases, including exact environment inheritance/override/
removal and independent SHA256(abc) through Windows PowerShell's own module.
The caller environment is preserved; individual scripts keep their existing
environment unless they explicitly supply overrides.

Independent model/native and UI/integration reviews found no remaining P1/P2
issue in scope. Actual mouse dragging, desktop appearance, accessibility,
forced exact temporary-name collisions and Windows10 remain unverified.
Account/path/command-line/handle columns across the full table require a
separate bounded metadata cache. This milestone leaves the broader roadmap active.

## Verified table-metadata checkpoint (2026-11-11)

All sixteen aggregate steps passed without SKIP at
`2026-11-11T11:54:50.4189551Z`; `ExecutedAllSteps=true`, `Complete=true` in
`work/validation/summary.json`. Both production executables are 254976 bytes,
with AMD64/PE32+, `SimpleEntry`, ASLR/high-entropy VA/NX and the unchanged
256 KiB gate passing. SHA256:

- admin: `6A10801229D19DF7D1B933BF2C8046D5EE8A4D9768AAD61F5A2C38E03BB26DAB`
- standard: `A6D23C6D3C8A32B0931218C3CE12E71A4F2A50D94AFCAAB703FD602AA7BB24C6`

The process picker now has16 stable IDs. V2 is exactly160 bytes; independent
default/nondefault V1 conversion oracles,37 malformed records, second-process
load and17 startup/path cases pass. Only an absent V2 permits legacy fallback;
bad/locked/inaccessible V2 keeps diagnostics/defaults. Migration does not write
V2 or alter V1. The real picker passed469 assertions across20 dialogs; routing
passed25 assertions. Existing96/144/192-DPI and5000-row UI regressions also pass.

`test-process-metadata.ps1` builds the actual main and runs six mandatory test
entries plus an owned child fixture. Cache tests cover5000 identities,8192 slots,
the32 MiB string budget, TTL/retry, valid-empty text, stale cookies/epochs/serials,
late results and allocation failure. Collector37 assertions include real child
path/account/command→cache roundtrips and immutable text after scratch erasure.
They also cover an unchanged held identity without SYNCHRONIZE permission.
Handle-count tests use an independent owned-process observation, valid zero and
poisoned failure output. Presentation covers all three text keys, both sort
directions, unknown-last, empty command and freshness boundaries.

The hidden metadata UI passed73 assertions using native16-column controls,
selection, worker-ready messages and actual250 ms timers. It verifies separate
path/account progress while command is gated, alternating viewport/fair admission,
TTL-driven resort with selected PID/birth retained, demand/mode changes, stale
completion rejection, timeout without replacement and asynchronous stop. Thread
counts remain1/1/1; selected-details workers are independent. Synthetic identities
stay in cache-only phases; provider calls require the independently observed
owned identity, and the final foreign-call count is zero. Caption assertions
check new IDs12–15 for nonempty text/alignment in EN/RU; exact text for all16
captions is not asserted by that fixture.

The full suite uses Windows PowerShell5.1 children from the PowerShell7 aggregate;
the complete metadata suite also passed directly in PowerShell7
(`work/process-metadata-e9f79e8e22614cdfa1ca8b537c23d4ea`). Aggregate metadata
evidence is `work/process-metadata-4518add5c8b74c47b7e1b334596fc65b`.
Independent reviews found and fixed the PD processor-capacity ABI mismatch,
permitted UNKNOWN identity handling for path/account and lost provider flags.
Runtime mutation proofs detect those regressions, cookie/deadline bypasses,
numeric substitution for text sorting, unfair scheduling and omitted timer sort.
Some RED proofs were recorded after implementation and are mutation evidence,
rather than a claim of complete test-first chronology.

Metadata has three fixed background lanes, a32 MiB payload budget and a60-second
freshness interval. The three-second deadline limits result acceptance; it cannot
terminate a blocked Windows provider. Metadata is not persisted. Interactive
desktop/accessibility, Windows10 and real UAC were not added to this checkpoint.
Downloaded `outputs/` files are unchanged. Hardware/per-process GPU monitoring
and the wider Task Manager/PC Manager roadmap remain active.
