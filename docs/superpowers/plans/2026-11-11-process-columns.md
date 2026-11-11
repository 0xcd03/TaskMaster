# Process Columns Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** configurable, persistent process columns with twelve snapshot-backed fields and correct identity/sort/render behavior.

**Architecture:** a pure128-byte layout plus atomic explicit-path store; a native ListView adapter maps physicalsubitems to stableIDs; parent integrates a transactional native picker and app routing. Row4096 ABI and collector remain unchanged.

**Tech Stack:** MASM x64/Win32, versioned local binary preferences, EN/RU resources, PS5.1/7 fixtures.

**Spec:** `docs/superpowers/specs/2026-11-11-process-columns-design.md`.

## Global constraints

- Preserve previous uncommitted fixes, native/optional details workers and downloaded outputs/.
- Shared feature branch checkout is intentional: required prior changes are uncommitted. No automatic commit/push/merge.
- All record offsets/IDs/defaults come from src/process_columns_contract.inc and the spec. Name0first and PID6visible are mandatory.
- No new process collection, privilege changes, or target mutations. No per-process settings/data in the saved file.
- Tests use owned explicit paths and hidden windows; no production LoadSettings or real preference write.
- Data/SRW structures align8; UTF16 API literals align2 independently of earlier ASCII buffers.
- Process mode geometry and stable sorting are separate from other panel semantics and process metrics/baselines.

## Task1 — Layout model and atomic store

Own `src/process_columns_model.inc`, `src/process_columns_model_tests.inc`, `test-process-columns.ps1`.
Parent includes shared contract/model/tests. Public APIs:
`PcDefaultLayout(RCX out128)` writes defaults;
`PcValidateLayout(RCX record)->EAX0valid/87invalid`, no mutation;
`PcEnsureLayout()` initializes `pcLayout[128]`, `pcLayoutReady DWORD` if needed;
`PcLoadLayout(RCX path,RDX out128)->EAX Win32`, exact128/read/validate before output;
`PcSaveLayout(RCX path,RDX record)->EAX Win32`, atomic unique owned temp/flush/replace.

- [x] Postimplementation RED proof: valid defaults/roundtrip against a stub120. This was not completed before implementation. A default record must contain count7,sort0,IDs0..6 thenFFFFFFFF, twelve exact widths.
```asm
lea rcx,pctLayout
call PcDefaultLayout
lea rcx,pctLayout
call PcValidateLayout
test eax,eax
jnz pctFail
cmp dword ptr [pctLayout+PC_L_COUNT],7
jne pctFail
```
- [x] Implement pure validation before file writes; malformed read/write preserves output/oldfile. Apply exact spec caps, version, trailingbyte and reserved rules. Atomically replace only after fullflush; preserve immediately captured errors over cleanup.
- [x] GREEN deterministic malformed records and independent exactbytes readback; second owned process loads same file; fail replacement and verify originalbytes plus no unowned deletion. Mutation omitting duplicateID or reserved check must fail.
- [x] Runner builds actualmain uniqueobject/res/fixtures, strict native helper and reports SKIP honestly. Add native/UI entries after their owners deliver; parent owns main/includes/resources.

## Task2 — Native table adapter and presentation

Own `src/process_columns_native.inc`, `src/process_columns_native_tests.inc`, and scoped changes to `src/process_ui.inc`, `src/threads.inc`.
Consumes pcLayout/PcEnsureLayout and contract. Produces:
`PcBeforeModeColumns()` capture outgoing mode using pcAppliedMode/Dpi; set rebuildingguard;
`PcBuildProcessColumns()` insert current visible process columns;
`PcAfterModeColumns()` mark appliedmode/dpi, dragstyle, restore process/nonprocesssort;
`PcCaptureLayout()->EAX0/error` updates model from real geometry without persistence;
`PcApplyLayout(RCX record)->EAX0/error` validates/captures old layout, installs a copied new layout and rebuilds columns/rowmap preserving selected identity; caller persists explicitly after apply;
`PcColumnId(ECX physical)->EAX stableID/FFFFFFFF`;
`PcColumnCaption(ECX ID)->RAX NULcaption` (current seven labels, new resources52214..52223);
`PcHeaderNotify(RCX NMHEADER*)->EAX0allow/1cancel, EDX1ifhandled/0otherwise`;
`PcDisplayExtra(RCX row,EDX stableID)->RAX text` for7..11;
`ProcessColumnsNativeTest` entry.
Globals: `pcVisibleIds DWORD[12]`, `pcVisibleCount DWORD`, `pcAppliedMode DWORD=-1`, `pcAppliedDpi DWORD=96`, `pcApplying DWORD`, `pcCapturePending DWORD`.
Parent supplies `PcPersistCurrent()` (noargs Win32) and handles message8063 by capture/persist.

- [x] Postimplementation native mapping/geometry proof and wrong-mapping mutation. Behavioral RED before implementation was not completed. Name+PID mandatory, physical IDs differ from visual order, semantic click invariant.
```asm
mov ecx,1
call PcColumnId
cmp eax,PC_PID ; fixture layout Name,PID,Read,Created...
jne pcnFail
```
- [x] Replace fixed7 deletion, creation and literal index use in renderer/comparator/heat/header; retain otherpanel7 semantics. Add extra source-backed fields with explicit unavailable, full64bit numeric sorting and stable identity ties. Use current collector/tree offsets and source semantics from spec.
- [x] Capture using oldmode/oldDPI before rebuild; restore mode-specificsort; block invalid namedrag, coalesce posted capture. Do not synchronoussave on each trackingpixel. Iterate header rectangles for tailbackground maxright.
- [x] GREEN hidden actualcontrols across 96/144/192, modes, tree/flat and themes; selected PID+birth and pinned details unaffected. Mutation wrong physicalmapping must fail. Report any fixture-dependent existing assumptions before changing expectations.

## Task3 — Transactional picker and app integration

Parent owns `src/process_columns_ui.inc`, `res/process-columns.rc`, `src/taskmanager.asm`, `src/settings.inc`, `src/dpi.inc`, menus/resourcegenerator/aggregate/docs.
An independent test owner may create only `src/process_columns_ui_tests.inc`, entry `ProcessColumnsUiTest` against the parent contract.
Main includes contract before consumers, PD-style `PC_TESTS` gate around newtests. Resource dialogs307/308, controllist4300, Up4301,Down4302,Defaults4303,status4304,OK1,Cancel2; command2502; message8063.

- [x] RED hidden actualmodal dialog stub against checkeditems/Cancel and no global/file mutation. Independent ownedpaths and testhook keepdialogs hidden, no productionprefs.
- [x] Implement all twelve checked-list entries with stableIDs, a draft128 record and roworder; Up/Down reorder exceptname, mandatoryPID/name cannotdisable. Description clarifies own counters/treeaggregation. Defaults changes draft only. OK validates/applies after capture-oldlayout completes; saved failure remains explicit while active layout retained. Cancel neverapplies/saves.
- [x] Initialize model/filepath from existingstorageDir afterLoadSettings; load failure defaults+diagnostic. Applicationclose captures/persists before destroyinglist. Main callback maps physicalsubitem, clicks mapbeforebounds andsemantic sort, header subclass handles native notifications; nonprocess commands cannotopenpicker.
- [x] GREEN modal cases, maincommandroute idle/busy and unrelatedpanels, messagecoalescing, font/theme/DPI and preservation of native/optional details. Add process-columns step to aggregate; both shells and productionPE gate checked.
- [x] Review actualfinalpatch, fix findings, record exactaggregate/timestamp/size plus remainingfulltabledata scope. Do not mark TaskManager/PCManager goalcomplete.
