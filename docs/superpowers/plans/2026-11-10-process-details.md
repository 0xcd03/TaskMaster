# Process details implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** show a selected process in an independent native modeless window, with verified PID/birth identity, responsive monitoring and explicit unavailable states.

**Architecture:** a stateless native collector produces a fixed immutable snapshot. An application-global worker owns a replaceable request/result mailbox; the UI accepts snapshots by window cookie, target generation and sample sequence. First deliver the independently testable native path, then add the separate bounded optional account/WMI lane; the complete design remains binding throughout.

**Tech Stack:** MASM x64, supported Win32 APIs, native EN/RU resources, Windows SDK COM/WMI, PowerShell 5.1 and 7 runners.

**Spec:** `docs/superpowers/specs/2026-11-10-process-details-design.md`.

**Execution checkpoint, 2026-11-11:** native window and optional account/WMI
lane are implemented. The final fourteen-step aggregate passed without SKIP at
`2026-11-11T10:06:10.0079077Z`; both 237056-byte builds passed PE gates.
Optional collector263 / worker306 / UI173 and native UI195 assertions passed;
both PowerShell shells passed the optional suite, including live WMI/account
and a real 2.5-second delayed-provider UI test. The original checklists below
remain the design/acceptance work record rather than a claim of every planned
manual/platform/load/overflow test. See DEVELOPMENT.md and the SDD ledger for
actual evidence and remaining gaps; configurable tables and the full product
roadmap remain in progress.

## Global constraints

- Preserve the completed Run new task checkpoint and all existing monitoring repairs. Do not overwrite downloaded `outputs/` artifacts.
- One process-details window, owned by the main window; selection/navigation never silently retarget it.
- Copy PID/birth/name during the command handler. Never retain `rowMap`, front/back row pointers, collector scratch buffers or mutable action globals.
- `windowCookie` changes for each opening; `targetGeneration` changes only for target identity changes or close. Poll and explicit Refresh do not change that generation.
- Native sequence and optional request epoch are independent. A one-second poll cannot invalidate a three-second optional request.
- Application-global worker limits survive close/reopen: one native worker and, in the optional phase, one optional worker. Each lane permits one active request, one replaceable pending request and one replaceable result.
- Requests contain copied scalar values. No context references or control/font/brush pointers are needed; `PD_R_CONTEXT` and `PD_S_CONTEXT` stay zero. Posted notifications never transfer snapshot ownership.
- No privilege changes, `GuardProcess` mutation guard, UAC, PEB reads, target suspension, `TerminateThread` or user-process actions.
- Every identifier added by this feature uses `PD_`, `Pd` or `pd`; check the actual symbol table before adding externs/equates.
- Hidden native tests are automated coverage, not claims of manual desktop/accessibility verification. Optional provider absence is an explicit coverage skip, never a successful read.

## Ownership and execution order

| Owner | Files | Independently reviewable result |
| --- | --- | --- |
| Collector worker | `src/process_details_collect.inc`, `src/process_details_collect_tests.inc`, `test-process-details.ps1` | Stateless verified native sample plus owned-process fixtures |
| Service worker | `src/process_details_worker.inc`, `src/process_details_worker_tests.inc`, `test-process-details-worker.ps1`; optional phase also `src/process_details_optional.inc`, `src/process_details_optional_tests.inc`, `test-process-details-optional.ps1` | Bounded native mailbox first; separate optional global lane second |
| Parent/UI integrator | `src/process_details_contract.inc`, `src/process_details_ui.inc`, `src/process_details_ui_tests.inc`, `res/process-details-dialogs.rc`, `test-process-details-ui.ps1`, `src/taskmanager.asm`, `src/settings.inc`, `res/menus.rc`, `res/ui-strings.json`, `build-resources.ps1`, generated `taskmanager.rc`/`src/ui_strings.inc`, `verify-development.ps1`, documentation | Real menu route, modeless window, integration and combined verification |

Tasks 1 and 2 may proceed concurrently against the approved contract; a gated test backend makes task 2 independently testable. Parent owns shared include/resource/routing edits. Complete and review native phase tasks 1–3 before implementing optional task 4. Do not describe phase 1 as completion of the full spec.

## Binding ABI

`src/process_details_contract.inc` is the shared source of truth. Version is 1; native lane is 1 and optional lane is 2. All offsets below are bytes, all structures are eight-byte aligned, and every reserved byte is initialized to zero.

### Copied request: `PD_REQUEST_SIZE = 96`

| Offset | Type | Constant / meaning |
| --- | --- | --- |
| 0 / 4 | DWORD / DWORD | `PD_R_SIZE=96` / `PD_R_VERSION=1` |
| 8 / 12 | DWORD / DWORD | `PD_R_LANE` / `PD_R_FLAGS=0` |
| 16 / 24 | QWORD / QWORD | `PD_R_COOKIE` / `PD_R_GENERATION` |
| 32 / 40 | QWORD / QWORD | `PD_R_SEQUENCE` / `PD_R_EPOCH` |
| 48 / 52 | DWORD / DWORD | `PD_R_PID` / `PD_R_RESERVED=0` |
| 56 / 64 | QWORD / QWORD | `PD_R_BIRTH` UTC FILETIME / `PD_R_DEADLINE` GetTickCount64 deadline |
| 72 / 80 | QWORD / HANDLE | `PD_R_CONTEXT=0` / `PD_R_CANCEL` optional cancellation event |
| 88 | HWND | `PD_R_NOTIFY`, notification destination only; not required to be zero |

Production UI uses cancel handle zero: cookie/generation invalidation cancels presentation without per-window event lifetime. A test that supplies an event owns it until the request has completed. Native deadline is zero; optional deadline is epoch creation tick plus 3000 ms. Native sequence starts at 1 and increases per target. Optional epoch changes on open, target change and explicit Refresh only. Never reuse a wrapped cookie/generation/sequence/epoch: fail the current window cleanly instead.

### Immutable snapshot: `PD_SNAPSHOT_SIZE = 134080`

| Offset | Type | Constant / meaning |
| --- | --- | --- |
| 0 / 4 / 8 / 12 | DWORD each | `PD_S_SIZE`, `PD_S_VERSION`, `PD_S_LANE`, `PD_S_TARGET` |
| 16 / 24 / 32 / 40 | QWORD each | `PD_S_COOKIE`, `PD_S_GENERATION`, `PD_S_SEQUENCE`, `PD_S_EPOCH` |
| 48 / 52 | DWORD each | `PD_S_PID`, `PD_S_ERROR` immediately captured target Win32 error |
| 56 / 64 | QWORD each | `PD_S_BIRTH` expected / `PD_S_OBSERVED_BIRTH` read from held process handle |
| 72 / 80 / 88 | QWORD each | `PD_S_TICK` GetTickCount64 / `PD_S_QPC` / `PD_S_FREQUENCY` |
| 96 / 100 | DWORD each | `PD_S_CAPACITY` total active logical processors / reserved zero |
| 104 / 112 / 116 / 120 | QWORD / DWORD / DWORD / QWORD | `PD_S_CONTEXT=0`, `PD_S_FLAGS=0`, reserved zero, reserved zero |
| 128 | 12 × 16 bytes | `PD_S_FIELDS`, per-field metadata described below |
| 320 / 328 / 336 | QWORD each | `PD_S_HANDLES`, `PD_S_PRIVATE`, `PD_S_WORKING_SET` |
| 344 / 352 | QWORD each | `PD_S_KERNEL`, `PD_S_USER` cumulative 100 ns units |
| 360 / 368 / 376 | QWORD each | `PD_S_READ_OPS`, `PD_S_WRITE_OPS`, `PD_S_OTHER_OPS` |
| 384 / 392 / 400 | QWORD each | `PD_S_READ_BYTES`, `PD_S_WRITE_BYTES`, `PD_S_OTHER_BYTES` |
| 408 / 412 | DWORD each | `PD_S_CPU_BASIS_POINTS` / reserved zero |
| 416 / 424 / 432 / 440 | QWORD each | `PD_S_READ_BPS`, `PD_S_WRITE_BPS`, `PD_S_OTHER_BPS`, reserved zero |
| 448 | WCHAR[32767] | `PD_S_PATH`, `PD_PATH_CAPACITY` |
| 65984 | WCHAR[256] | `PD_S_SID`, `PD_SID_CAPACITY` |
| 66496 | WCHAR[1024] | `PD_S_ACCOUNT`, `PD_ACCOUNT_CAPACITY` |
| 68544 | WCHAR[32767] | `PD_S_COMMAND`, `PD_COMMAND_CAPACITY` |

Capacities include NUL. The two-byte padding before SID and final padding are zero. The fixed block is below the 192 KiB design limit. Metadata for field index `i` starts at `PD_S_FIELDS + i * PD_FIELD_SIZE` and contains DWORD `PD_FIELD_STATUS=0`, `PD_FIELD_ERROR=4`, `PD_FIELD_LENGTH=8` (WCHAR count excluding NUL; numeric fields zero), `PD_FIELD_FLAGS=12` (bit 0 means HRESULT, bit 1 means value exceeded capacity). No truncation may become VALID.

| Field index / constant | Source and ownership |
| --- | --- |
| 0 `PD_F_IDENTITY` | Native expected/observed birth |
| 1 `PD_F_PATH` | Native full image path |
| 2 `PD_F_SID` | Native token SID |
| 3 `PD_F_ACCOUNT` | Optional account-name lookup |
| 4 `PD_F_HANDLES` | Native handle count |
| 5 `PD_F_MEMORY` | Native PrivateUsage and WorkingSetSize |
| 6 `PD_F_CPU_TIMES` | Native cumulative kernel/user times |
| 7 `PD_F_IO` | Native six cumulative I/O counters |
| 8 `PD_F_CPU_RATE` | Presentation baseline; collector/worker return NOT_REQUESTED |
| 9 `PD_F_IO_RATE` | Presentation baseline; collector/worker return NOT_REQUESTED |
| 10 `PD_F_COMMAND` | Optional WMI command line |
| 11 `PD_F_LIVENESS` | Native exit/ambiguous state |

Field states: `PD_STATUS_LOADING=0`, `VALID=1`, `UNAVAILABLE=2`, `TIMEOUT=3`, `NOT_REQUESTED=4`. Valid zero or valid empty string is distinct from all other states. Target states: `PD_TARGET_LOADING=0`, `VERIFIED=1`, `EXITED=2`, `DENIED=3`, `MISMATCH=4`, `UNKNOWN=5`, `CANCELLED=6`. Target/field failures use the snapshot; structural API failures use EAX.

### Public procedures and ownership

```asm
; Collector: caller owns a writable heap block, no allocations escape.
; RCX request*, RDX snapshot*, R8D output byte capacity -> EAX Win32 error.
PdCollectNative PROC
; 87: malformed request; 122: too-small output; 50: unsupported lane.
; Rejected structural requests write no output.
; 0: complete initialized snapshot, including any per-field/target errors.
PdCollectNative ENDP

; Application-global native service. UI thread calls these procedures.
PdWorkerEnsure PROC       ; no args -> EAX Win32; start at most one thread
PdWorkerEnsure ENDP
PdWorkerSubmit PROC       ; RCX request* -> EAX Win32; copies all 96 bytes
PdWorkerSubmit ENDP
PdWorkerTake PROC         ; no args -> RAX snapshot* or 0; ownership to caller
PdWorkerTake ENDP
PdWorkerCancel PROC       ; RCX cookie; discard matching pending/result
PdWorkerCancel ENDP
PdWorkerFree PROC         ; RCX snapshot*; free once, null accepted
PdWorkerFree ENDP
PdWorkerStop PROC         ; no args; signal stop, no UI-thread wait
PdWorkerStop ENDP
```

`PD_READY_MESSAGE=8060h` and `PD_THEME_MESSAGE=8061h` are reserved feature messages; existing 8001h/8002h remain unchanged. READY carries no heap pointer: use zero WPARAM/LPARAM and drain `PdWorkerTake`. If PostMessage fails, publication/cleanup still has a bounded owner. Closing a window calls Cancel then destroys it; a late native completion is discarded or rejected by cookie. UI frees every taken block, including rejected blocks.

## Task 1 — Native collector and independent fixture evidence

**Files:** collector owner files above; parent inserts the contract before consumer modules and includes the test module.

**Consumes:** request/snapshot ABI. **Produces:** `PdCollectNative`, `ProcessDetailsNativeTest`, `ProcessDetailsFixtureEntry`.

- [ ] Add a fixture child controlled only by named events/temp files in its unique test directory. Report its own birth, path, SID, handle count, memory and six I/O counters through independent direct APIs; expose allocation/I/O and busy/idle phases. Keep independent expected observations outside `PdCollectNative`.
- [ ] Write assertions for success against an owned child, exact identity/path/SID, counter monotonicity and controlled memory/I/O growth. Add canaries around output and request immutability checks. Start with a 120-returning stub; run the success case and record the first expected/actual assertion as RED.
- [ ] Add structural RED cases for size/version/flags/context/reserved fields, output capacity `134079`, lane 2 returning 50, birth mismatch, and signaled cancellation event. A 134079-byte capacity must return 122 without changing a sentinel byte.
- [ ] Implement with private scratch storage and checked lengths: limited query plus SYNCHRONIZE first, limited-only fallback, held-handle birth check before values; token query/SID, handle count, full Unicode path, process times, memory and I/O. A separately opened `410h` memory handle must match birth too. Do not overwrite available fields when another source fails.
- [ ] Test deterministic denied/fallback/mismatch boundaries by replacing only the narrow OpenProcess/API boundary, preserving request validation and result initialization. Confirm accessible self-reading is allowed; do not call GuardProcess. For denied/mismatch, retain expected identity but publish no foreign values as valid.
- [ ] Check exit with zero wait when SYNCHRONIZE is available. Without it, non-259 confirms exit; 259 alone yields ambiguous liveness rather than an alive claim. Keep real valid zero distinct from unavailable and capture each GetLastError before cleanup calls.
- [ ] Leave account, command and rates NOT_REQUESTED in phase 1. All successful returned strings are terminated with exact metadata length; over-cap errors set unavailable/cap flag. Verify handles/tokens/heap are balanced over repeated reads.
- [ ] GREEN: run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-process-details.ps1`, then `pwsh -NoProfile -File .\test-process-details.ps1`. Runner builds unique `work/process-details-native.obj`, `.res` if needed, `process-details-native-tests.exe` and fixture executable; use existing `Invoke-NativeTestProcess` and its nonzero/timeout checks. Fixture processes are hidden and only owned children are cleaned up.

Example structural assertion in the native test (the actual harness also verifies both canaries):

```asm
lea rcx,pdTestRequest
mov rdx,pdTestSnapshot
mov r8d,PD_SNAPSHOT_SIZE-1
call PdCollectNative
cmp eax,122
jne pdTestFailure
mov rdx,pdTestSnapshot
cmp byte ptr [rdx],0A5h
jne pdTestFailure
```

## Task 2 — Application-global native worker and mailbox

**Files:** service owner native files above. **Consumes:** copied request and `PdCollectNative`. **Produces:** six worker APIs above, `ProcessDetailsWorkerTest`; test-only `pdWorkerCollector` callback defaults to the real collector.

- [ ] Add a gated collector backend recording calls and copied requests. RED: block active sequence 1; submit 2 then 3; release 1; require the backend to see 1 and 3 only, with one global thread and no request-pointer retention. Modify the caller's input after Submit to prove copying.
- [ ] Implement one application-global native thread/event, one inline active request, one inline pending request, one heap active sample and one heap result pointer. Publish/take/cancel under one short lock; never hold it across native APIs, heap destruction or PostMessage. Replacement releases the previous result exactly once.
- [ ] Ensure is idempotent across 100 open/close cycles. Stop is application shutdown only; ordinary Close never stops/recreates the global worker. Reject Submit after service stop; never start a replacement while an older service thread exists.
- [ ] Test result replacement before Take; Cancel discarding matching pending/result; cancel during an active call; target/cookie changes; PostMessage failure; collector structural/allocation failure; and shutdown while gated. Allocation failures must surface to UI, not silently leave permanent loading. No worker touches controls or dereferences notify HWND as an object.
- [ ] UI is the final acceptance authority: an older completed sample remains acceptable if newer than last applied, even with a newer request pending. Worker must not discard it merely because its sequence differs from latest requested. Worker does not compute rates; the accepted UI baseline owns those derived fields.
- [ ] Prove bounds with backend gates/counters and process handle measurements: at most one worker creation, one active/pending/result per native lane, no accumulating request contexts across 100 cycles, full cleanup after release. Count steady-state handles after warmup rather than assuming unrelated runtime caches are leaks.
- [ ] GREEN: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-process-details-worker.ps1` and PowerShell 7 counterpart. Use unique `work/process-details-worker.obj`, `.res` if used, `process-details-worker-tests.exe`, separate logs. Event-based synchronization, bounded waits, no sleeps as the correctness oracle.

Deterministic mailbox scenario:

```text
Gate backend(1); Submit(1); wait entered(1)
Submit(2); Submit(3); release(1); wait entered(3)
assert call sequences == [1, 3]
Take completed snapshot; assert payload belongs to its copied request
Cancel(cookie); release all gates; assert outstanding snapshots == 0
```

## Task 3 — Modeless native window and parent integration

**Files:** parent/UI files above. **Consumes:** worker APIs and snapshot ABI. **Produces:** `PdShowSelectedProcess`, `PdDialogProc`, `PdRouteModeless`, `PdApplyTheme`, `PdCloseWindow`, `PdShutdown`, `ProcessDetailsUiTest`.

- [ ] Write hidden-control RED cases using the actual dialog procedure and a recording worker boundary. Cover real main WM_COMMAND 2501 routing while `actionBusy` is both 0 and 1. Require `frontMode=0`, selected row within `visibleCount`, immediate copy of row name, PID `+3584`, birth `+3600`. No selection/non-process panel must not open or submit. Birth zero opens an identity-unconfirmed explanation without accepting a new birth by PID.
- [ ] Add command 2501 to EN/RU Process menu and existing process context menu. Route it before `SettingsCommand` and outside QueueAction, preserving command 2500 and current action report Details button. Disable invalid context commands and still guard the handler itself.
- [ ] Add native modeless resources 305 EN / 306 RU in `res/process-details-dialogs.rc`; generator includes this file. Use applied `appLanguage`, not pending `selectedLanguage`. Read-only labeled controls cover name/PID/creation, path, SID/account, handle count, private commit/working set, CPU, six I/O counters/rates, command line, target/field statuses. Include Refresh, Close and explicit Copy; long path/command controls scroll. Phase 1 account/command say optional data is not yet requested, not empty/successful.
- [ ] Create with CreateDialogParamW, keep owner enabled, and store a private window state. Same identity reactivates; explicit other identity creates a new generation and replaces pending work. Keep copied display name and scalar identity in private state. Close invalidates identity, kills timer, Cancels cookie, frees accepted blocks/fonts/brushes and destroys HWND without waiting for the worker.
- [ ] Integrate keyboard routing before normal translation in `simple_loop`. `PdRouteModeless(RCX=MSG*) -> EAX BOOL` calls IsDialogMessageW only for the live dialog. If true, continue GetMessage; otherwise retain TranslateMessage/DispatchMessage. Tab/Shift+Tab/Escape and native edit Copy must work without making the dialog modal.
- [ ] On open submit native sequence 1. One-second timer replaces pending requests; hidden/minimized windows pause sampling and restoration submits fresh work. Explicit Refresh increments optional epoch only, requests a native sample and preserves the current native baseline. Sort/filter/panel changes leave the pinned target unchanged.
- [ ] READY drains Take. Reject wrong size/version/lane/cookie/generation/PID/birth and sequences `<= lastApplied`; always free rejected results. Accept sequence 5 when sequence 6 is pending if lastApplied is 4. Apply per-field states, retain final successful values with last-sample timestamp on exit, and avoid claiming exit from access denial. No snapshot or sensitive path/SID/command is logged automatically.
- [ ] Derive CPU/I/O only from two accepted valid native samples of the same identity and unchanged positive capacity. Use GetTickCount64 monotonic milliseconds, matching existing process/thread telemetry: CPU basis points = delta(kernel+user) × 10000 / (elapsed-ms × 10000 × CPU-capacity); I/O B/s = delta-bytes × 1000 / elapsed-ms. QPC/frequency remain diagnostic fields and do not gate this millisecond calculation. Reject zero/backwards/>30s intervals, counter rollback and overflowing quotients; reset on identity/access/source failures. Valid idle deltas display zero. Native raw fields remain immutable. This implementation ruling replaces the earlier QPC formula; it avoids multiplying by QPC frequency and is covered by deterministic integer boundaries.
- [ ] Own per-window DPI/font/brush/layout; WM_DPICHANGED uses suggested rect, local scale and minimum size without changing `uiDpi/appFont` for main. `PdApplyTheme` updates this window's controls and DWM attributes. Parent ApplyTheme explicitly notifies via 8061h. Delete only owned GDI objects after detaching controls.
- [ ] GREEN hidden actual-control tests: EN/RU × light/dark × 96/144/192 DPI; exact labels/status distinctions, long values, theme changes, main DPI unchanged, repeated open/close, same/different identities, table snapshot replacement, navigation, delayed completion, and HWND reuse. Assert no command executes a user action and main stays responsive to bounded WM_NULL probes while backend is gated. Include deterministic accepted-sequence and busy→idle baseline tests, plus owned-child live CPU/I/O evidence from task 1.
- [ ] Run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test-process-details-ui.ps1` and PowerShell 7 counterpart with unique `work/process-details-ui.obj`, `.res`, `process-details-ui-tests.exe` and logs. Integrate all three new test steps into verify-development; run standard/admin builds and aggregate only after individual suites are green. Independent reviewer checks x64 calling convention, heap ownership, stale-result acceptance, menu routing and actual assertions.
- [ ] Update checkpoint/docs with actual native behavior and explicitly pending optional lookup/WMI. Retain full goal and remaining full-table/GPU scope in ROADMAP; do not mark the whole details design complete.

Acceptance predicate for a native result, implemented without touching latest-requested sequence:

```text
size/version/lane valid
AND result(cookie, generation, PID, expectedBirth) == live window identity
AND result.nativeSequence > lastAppliedNativeSequence
```

## Task 4 — Required optional lane, account name and supported WMI

This is the next implementation phase after native review, not a silently removed requirement. Service owner adds the optional collector/tests and extends the service; parent/UI integrates only after optional deterministic tests are green. ABI layout remains unchanged.

**Interfaces:** `PdCollectOptional(RCX=request*, RDX=snapshot*, R8D=capacity) -> EAX Win32`, same structural/no-write contract, accepting lane 2 only. The integration ruling uses a separate `PdOptionalWorkerEnsure/Submit/Take/Free/Cancel/Stop/TakeFailure` API with the same register/ownership contracts as the native service. This keeps each lane's synchronization and error mailbox independent. UI close and application stop explicitly call both services. Optional READY uses `8062h`; native READY remains `8060h`. Optional sequence may be zero; epoch and deadline must be nonzero.

- [ ] RED deterministic backend: optional result delayed 2500 ms must be accepted after native polls at 1000/2000 ms; explicit Refresh must reject the previous epoch while preserving current native sample and baseline. Use controlled clock/gates for deterministic assertions; a separate live delay test proves message-loop responsiveness.
- [ ] Implement an entirely separate active/pending/result lane with the same bounds and copied scalar identities. No new thread on Refresh, target change or close/reopen. Do not start pending requests whose 3000 ms acceptance deadline expired; report provider-busy/timeout and require a new explicit Refresh epoch. A blocked old active request cannot make unbounded new contexts or workers.
- [ ] Optional collector initializes COM on its own thread and balances initialization, proxies, enumerators, BSTRs and VARIANTs. Resolve account name from verified token SID with LookupAccountSidW; failure preserves native SID. WMI namespace is ROOT\\CIMV2, query `SELECT ProcessId, CreationDate, CommandLine FROM Win32_Process WHERE ProcessId=<decimal PID>`, flags RETURN_IMMEDIATELY|FORWARD_ONLY, finite Next waits. Check returned-count even with WBEM_S_FALSE; preserve HRESULTs and null versus valid empty command.
- [ ] Hold/recheck native process identity and normalize WMI CreationDate UTC/precision before accepting PID/birth. Reject unverifiable/mismatched results. Respect WCHAR caps and disclose overflow; never read PEB or change privilege policy. Account or command errors do not erase native fields or each other.
- [ ] UI accepts optional results only for live cookie/generation/PID/birth, matching current optional epoch and before its deadline. Compare no native sequence. Merge only account/command/optional status; native refresh cannot erase accepted optional data. Explicit Refresh clears previous optional data into loading, increments epoch and queues one replacement without resetting native rates.
- [ ] Deadline limits result acceptance, not duration of every COM/account-provider call. Keep a blocked worker globally single, show timeout/provider-busy, and keep native samples/WM_NULL responsive. Stop cancels pending/results without TerminateThread or a UI wait; active global resources retain ownership until return or process exit. No new per-window worker may substitute for a blocked one.
- [ ] GREEN bounded-lifetime test: gate optional active call, run 100 close/reopen and Refresh cycles, assert exactly two worker creations total, one active/pending/result maximum per lane and no contexts/handles growing with window count; unblock and prove release. Include failure-to-post, expired pending, late epoch, wrong birth and HWND-reuse cases.
- [ ] Run `test-process-details-optional.ps1` under both shells with unique `work/process-details-optional.obj`, `.res` if used and `process-details-optional-tests.exe`. Live WMI owned-child test compares independently captured exact Unicode/spaces/long command line or emits explicit optional coverage SKIP with reason. Deterministic tests remain mandatory even when live WMI is unavailable. Rerun UI/aggregate after integration and independently review COM vtables/x64 ABI, lifetime and partial-field merging.
- [ ] Only after these results update docs from optional pending to actual supported behavior. Full table customization, new bulk columns, per-process GPU and any new mutating actions remain outside this first-window design.

## Review and evidence ledger

- [ ] Preserve each task's first meaningful RED assertion and final GREEN command, actual exit code, assertion count and log paths; a build-only failure is not the behavioral RED evidence.
- [ ] Reviewer examines current source/tests, not just passing totals: independently observed values, no mirrored collector oracle, all stale/error paths freeing ownership, no hidden skip counted PASS, no global execution-policy changes or unrelated process cleanup.
- [ ] Run the existing aggregate once after native integration and again after optional integration; inspect every result and skipped-coverage field. Broaden reruns only for subsequent changes/failures. Keep generated release metadata honest about supplied binaries.
- [ ] Record remaining optional or manual desktop coverage explicitly at each checkpoint. This document is an implementation plan, not evidence that any new feature is already implemented.
