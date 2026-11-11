# GPU monitoring implementation plan

Spec: `docs/superpowers/specs/2026-11-11-gpu-monitoring-design.md`; ABI: `src/gpu_contract.inc`.

## Task 1 — parser and aggregation model

Own `src/gpu_model.inc`, `src/gpu_model_tests.inc`, isolated test harness under work and model report. Implement pure fixed-workspace ABI and meaningful RED/GREEN tests: complete grammar, pointer/count bounds, doubles and memory overflow, independent lanes, duplicate detection, busiest physical vs process engine, stable ties and immutable lookup. Do not edit main/UI/provider.

## Task 2 — PDH/DXGI provider and worker

Own `src/gpu_provider.inc`, `src/gpu_provider_tests.inc`, `src/gpu_worker.inc`, `src/gpu_worker_tests.inc`, isolated test harnesses under work and report. Implement provider seams, exact identities/baselines and independent errors; one worker/latest frame/epoch/failure contract. Start with stub model external symbols if Task1 is not ready; fixture seams must test the actual production provider/worker. No UI or shared-main edits.

## Task 3 — layout V3 and process UI integration

After build isolation hands off shared files, extend V3 model/native persistence and migration tests. Add four native captions, ID mappings, numeric/engine sort, freshness and demand routing with immutable frame ownership and PID/birth matching. Native hidden-window tests, no provider calls from rendering or comparison. Parent owns shared main, process columns, resources and test runners, or delegates a clearly bounded subset after interface scan.

## Task 4 — Performance and release verification

Expose per-adapter identity/status/load/memory/capacity, wire summary/chart sequence and gap handling, close lifecycle. Add mandatory GPU gate, run original17 + GPU gates with no silent skips, PS5 and PS7 relevant fixtures, normal/admin production size/security checks. Perform current-machine live owned workload validation and report practical limits. Request independent scoped then broad review, fix concrete findings and re-run affected gates. Update README, DEVELOPMENT and ROADMAP only to measured current state. Preserve all previous work; no commit/push/publish.
