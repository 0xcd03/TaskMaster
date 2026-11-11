# Process Table Metadata Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** complete account/path/command/handles fields in the configurable process table with independently bounded background reads.

**Architecture:** migrate layout to16 stableIDs; a UI-owned sparse cache receives immutable results from3 fixed worker lanes. Handle count joins ordinary collector snapshots. Existing pinned details services are unchanged.

**Tech Stack:** MASM x64, Win32/token/WMI APIs, native common controls, PS5.1/PS7.

**Spec:** docs/superpowers/specs/2026-11-11-table-metadata-design.md.

## Global constraints

- Preserve all previous uncommitted work and downloaded outputs/. Shared feature branch is intentional; no commit/push/merge required.
- User authorized autonomous roadmap execution. No repeated design-scenario approval.
- No production LoadSettings/userprefs/foreign process mutation in tests; only owned paths/children/hidden windows.
- Fixed8192rows/4096rowABI, PM192entry/8192slots/16384hash/32MiB string payload. Never134080bytesperrow or unbounded replacement threads.
- Native strings align2, SRW/data structs align8; preserve LARGEADDRESSAWARE. Use LEA before indexed globals.
- Each task must capture meaningful failing behavioral evidence before implementation, then GREEN and relevant mutation where required. Do not claim test-first chronology if missed.
- Parent owns shared main includes/resources/aggregate integration. Worker/cache interfaces come from parent-owned process_metadata_contract.inc.

### Task 1: Version2 layout and compatibility

Own process_columns_contract.inc/model.inc/model_tests.inc, test-process-columns.ps1;
also mechanical limits/buffers in native.inc/ui.inc/native_tests/ui_tests/route_tests
and process_ui_tests.inc. Do not implement new metadata display/sort/caption bodies
or controller hooks in those shared files; parent does that after task stable.

Interfaces: preserve Pc* API names. New16 IDs0..15, Account12/Path13/Command14/
Handles15. V2size160/version2/IDs24/widths88/tail152. Defaults7visible; new widths
180,360,480,90. Loader accepts strictV1 orV2 and outputsV2; saver onlyV2.

- [x] RED independent160byte default oracle + nondefaultV1 migration using a work-only stub or previous implementation; failure must assert behavior, not missing symbols.
```asm
lea rcx,pctLayout
call PcDefaultLayout
cmp dword ptr [pctLayout],160
jne pctFail
```
- [x] Move loader/saver locals out of expanded record buffer before changing copies. Implement V1 validator/conversion and V2 strict validation. Preserve oldfile/output on error and atomicownedtemp behavior.
- [x] Replace actual record/count couplings with constants, preserve unrelated legacy7/numeric/time constants. Expand caption storage to9slots but parent owns new dispatch behavior. UI path suffix/migration adapters remain parent-owned; do not change them.
- [x] GREEN full existing feature suite with meaningful updated expectations; V1 fixed independent binary oracle covers reordered visibility, hidden widths and descending oldsort; malformedV1/V2 and maximal16/default7. Record exactfiles changed for parentsharedfile takeover.

### Task 2: UI-owned cache model

Own process_metadata_cache.inc and process_metadata_cache_tests.inc only. Read the
parent contract verbatim for offsets/API. No worker/UI/process API calls in model.
Heap allocation/free, clock input and formatting of error strings are permitted.

- [x] RED on Ensure before implementation; Get/Prepare stub proofs were recorded later as mutation evidence (see task report).
```asm
mov ecx,101
mov edx,123
call PmCacheEnsure
test rax,rax
jz pmtFail
```
- [x] Implement openaddress hash/freelist descriptors, frame marking/prune, generation/cookie/serial checks, statuses/TTL/retry/demand, exact-sized string budget and pointer ownership. Completion validates all referenced snapshot fields/lengths/capacities/NUL before copy. Unknown never becomes numeric zero or valid empty.
- [x] GREEN5000 identities, reuse/stale outputs, independent3fields, validempty, TTL/retry/deadline boundaries, memory/slot cap, failure seams, longstrings, nooutputmutation on malformedrequest. Mutation skipping cookie or deadline must fail.

### Task 3: Reentrant field collectors and fixed lane workers

Own process_metadata_collect.inc, process_metadata_worker.inc,
process_metadata_collect_tests.inc, process_metadata_worker_tests.inc. Existing
Pd* sources/services remain read-only. Consume parent contract and PD ABI.

- [x] RED collector wrongidentity/deadline and realworker mailbox tests against minimal stubs; retain failing logs.
- [x] Implement lane1path through PdCollectNative, and independent optional lane2account/lane3command wrappers around PdOptionalVerify/Account/Wmi/Check with copied96byte requests and caller-owned134080snapshot. Preserve field status/error/length/flags and identity/liveness distinctions.
- [x] Implement one generic context/state machine instantiated3times, one thread/event/activejob/result/failure each. No allocated failure mailbox, no overwrite of unconsumed results, no restart afterstop. Take transfers snapshot ownership toUI; free onlyowned blocks. Use exclusive lock only around transitions, never across collection/providercall.
- [x] GREEN realowned child values, staleID/deadline errors, independentfieldseams andgatedcommand while native/accountfinish; boundedthreads/results, duplicatebusy, postfailure/deadline/readfailure/allocationfailure, stop/delayedreturn. Mutation acceptanceafterdeadline or requestidentity mismatch must fail.

### Task 4: Handle snapshots, controller, UI and integration (parent)

Own process_metadata_ui.inc, process_metadata_ui_tests.inc,
test-process-metadata.ps1; sharedcollector/main/columnspresentation/resources/docs
only after Task1 yields sharedfiles. Use fresh independent tests/review agent.

- [x] RED handles owned-process independentGetProcessHandleCount before implementation; native controller stubs were verified later as behavioral mutations.
- [x] Collect handles+valid at row3904/3908 only after samehandleGetProcessTimes; no oldvalue fallback. Add native metadata text/numeric sorting and new captions/resources; leftalign12..14; maintain PID/birthties and unknownlast.
- [x] Controller marks/prunes/ensures currentidentities, services3lanes on250mstimer+8064, prioritizesselection/viewport thenfaircursor, detectsdeadlinewithoutthreadreplacement, updatesselection/sort/display onacceptance. Derive demand from visiblecols/processmode; do not queriesfixturePIDs unless explicitPMtestenable.
- [x] Load V2 then V1 onlywhenV2absent, preserve diagnostic/defaults forbadV2. Add tests for actual16picker/layout/menu/display/sort/demand/deadlines and preservation of pinned details.
- [x] Run all new gatesPS5/PS7, fullaggregate, productionPE/security/size gates. Review actualwholefeature, addressfindings, documentexactevidence/limitations. Goal remainsactive until fullroadmap achieved.
