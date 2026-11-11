# Native production and test build separation

The previous goal turn made verified progress: table metadata and the full16-step
aggregate passed at2026-11-11T11:54:50.4189551Z. The full Task Manager/PC Manager
goal remains active. GPU expansion follows; this build task recovers space from
test-only procedures rather than changing the existing256KiB production gate.

Production currently includes test entrypoints, fixtures and hidden regression
code in its monolithic MASM object. Test builds must explicitly define TM_TESTS.
PD_TESTS/PC_TESTS/PM_TESTS/GP_TESTS imply TM_TESTS so feature runners retain all
their shared fixtures. Production keeps every runtime procedure/data it needs,
including uiRegressionMode=0 if referenced. Runtime test branches may be gated;
no production action, security flag, icon or user-visible behavior is removed.

All scripts that assemble test objects add TM_TESTS or inherit it from feature
defines. build-tests.ps1 must compile its own test object, never reuse the last
production taskmanager.obj. It must continue all existing entrypoints/suites.
Production and test objects have distinct paths; building in either order works.
Native test nonzero/timeout handling remains strict. Tests do not write user
settings, affect foreign processes, or launch production UI. Downloaded outputs
remain unchanged; existing ignored test executables may be generated as before.

Acceptance: behavioral RED checks that test-only symbols are present in the
current production map; after separation production map lacks known test symbols,
test map/object links and runs them, production size/security checks pass and
all16 existing aggregate steps pass. Verify representative include exclusions,
not an arbitrary byte-saving threshold. No permission/commit/push is required.
