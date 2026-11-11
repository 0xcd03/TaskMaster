# Native Test Build Separation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** keep native regression code out of production while preserving every existing test and runtime behavior.

**Architecture:** TM_TESTS gates test-only definitions/includes; feature test defines imply it. Runners compile a separate owned test object, independent of the production object.

**Tech Stack:** MASM x64, Win32, PowerShell5.1/7, existing build harnesses.

**Spec:** docs/superpowers/specs/2026-11-11-native-test-build-design.md.

## Global Constraints

- Preserve existing uncommitted files and outputs/. No commits/push/merge/production app/CUA/user preference changes.
- Keep256KiB/AMD64/security gates and all native/hidden UI tests. No deleting functionality/assets to pass size.
- Parent is designing GPU work but will not edit shared main/collector/test runners until this task releases them.
- Existing user instruction authorizes autonomous implementation. Do not ask another permission question.

### Task 1: Isolate production/test compilation and verify all consumers

**Files:** modify src/taskmanager.asm, test-only include guards/data boundaries in src/*.inc as required; build-tests.ps1 and every test runner assembling taskmanager.asm. Create test-build-isolation.ps1 if needed and integrate verify-development.ps1 as one additional explicit gate. No unrelated telemetry implementation.

**Interfaces:** TM_TESTS is defined by any PD_TESTS/PC_TESTS/PM_TESTS/GP_TESTS flag. Production has none; feature tests retain their entrypoints. All existing script options remain valid. build-tests.ps1 assembles work/test-taskmanager.obj independently.

- [ ] Capture RED from current production map containing SelfTest/ModelTest/UiRegressionTest; assert expected absence fails for behavioral reason.
```powershell
$map=[IO.File]::ReadAllText('./work/TaskManagerX64.map')
if($map -match '\s(SelfTest|ModelTest|UiRegressionTest)\s'){throw 'Production contains test entrypoint'}
```
- [ ] Identify references from runtime to test-only data/procedures before gating. Keep production-required globals in an explicit small support include or conditional declarations. Wrap runtime calls/branches only where they are exclusively test injection. Align UTF16/SRW/ABI buffers.
```asm
IFDEF PD_TESTS
  TM_TESTS equ 1
ENDIF
; guard redefinition if another test flag is also present
IFDEF TM_TESTS
  include selftest.inc
ENDIF
```
- [ ] Add TM_TESTS to all test assembly commands; replace production-object reuse in build-tests.ps1 with explicit ML64 test assembly. Retain strict exit/timeout behavior and all existing fixture/scheduler entrypoints.
- [ ] Build production first then test, and test first then production. Confirm test symbols absent from production map but test object links/runs SelfTest/ModelTest/UI fixtures. Record measured size/section changes, not estimate.
- [ ] Run the complete aggregate with all existing16 steps plus the build-isolation gate and no SKIP; independently review shared symbol/runners coverage. Write report in this plan's SDD workspace with RED/GREEN paths, changed files, size delta and limitations. Return shared-file handover before parent GPU main integration.
