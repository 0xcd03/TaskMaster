# Metadata in the process table

This is the next part of ROADMAP stage1/item6, following the verified12-column
checkpoint. User authorization to implement the roadmap autonomously persists.
The previous goal turn made progress: the authoritative15-step aggregate passed
at2026-11-11T11:01:15.7839486Z. The full TaskManager/PCManager goal remains active.

## Behavior and sources

The existing picker gains Account12, Image path13, Command line14 and Handles15.
The default seven visible fields remain. Each field refers to the row's own
process, including in tree mode. Handles is a current snapshot DWORD, never a
descendant aggregate. Account/path/command are independent dated observations,
refreshed after60seconds; expired or failed observations are not valid zero/empty
strings. A valid empty command is distinct from a missing value. Field errors,
loading and timeout are visible in EN/RU; values never become persistent history.

GetProcessHandleCount uses the collector's already-held limited-query process
handle after successful GetProcessTimes. Row3904=count and3908=valid are the
verified unused gap before I/O text3912. ClearRow clears both on every pass.

Path collection reuses the reentrant PdCollectNative on cache misses/refreshes;
its extra memory/SID/I/O work is confined to a dedicated metadata worker.
Account and command have separate workers and separate verified-held-handle
wrappers around PdOptionalAccount/PdOptionalWmi. They do not call the combined
PdCollectOptional because a WMI timeout would erase an already resolved account.
No table job calls either selected-details singleton service, collector globals,
StoreDecimal, or the icon queue's single-producer helpers.

PID plus nonzero creation FILETIME identifies every request and cache entry.
Optional wrappers verify held PID/birth before and after provider work; account
allows the existing unchanged-but-ambiguously-live identity semantics, whereas
command requires positively live before/after plus WMI ProcessId/CreationDate.
Missing rights, exited/mismatched identities, embedded NUL, overlong output,
provider errors and allocation failure are explicit. No privilege changes occur.

## Ownership and bounds

The UI thread alone mutates a table-owned cache. Workers receive copied requests
and publish immutable snapshots; they never retain a row/cache pointer. The
collector continues publishing the existing4096-byte immutable rows. Rendering
and sorting perform cache lookup only, without process/provider calls or waits.
Published strings are immutable until the UI replaces/prunes that field; no
worker mutates or frees them. Sorting runs within one UI dispatch and uses two
distinct cached strings, never two aliases of the shared numeric scratch buffer.

Cache:8192 descriptors of192bytes,16384 hash slots, at most32MiB of allocated
UTF16 string payload. Descriptors and hash are fixed BSS. Account length cap1023,
path/command cap32766 WCHAR plusNUL, inherited from PD source capacities. Exactly
one actual-sized allocation per valid cached field; no134080-byte per-row object.
Allocation/budget failure reports error8 and retries after15seconds. Completed
worker snapshots and scratch have separate fixed bounds: at most one result per
lane plus one result temporarily held by UI (4×134080bytes), and one request per
lane. Workers allocate results only after admission.

Frame update marks existing current identities, prunes unmarked identities,
then ensures all current nonzero-birth identities. Slot reuse changes a monotonic
cookie; PID reuse, removed rows and cache slot reuse cannot consume old results.
A full table with8192 distinct verified identities fits the descriptor bound.

Cache status:0 missing,1 loading,2 valid,3 unavailable,4 timeout. Each field has
its own request serial, observation tick and due/deadline tick. Success TTL60000ms,
error retry15000ms and acceptance deadline3000ms. The UI updates a monotonic
pmNow once per pump. Expired valid strings are treated as loading/unavailable
for display/sort until refreshed, rather than silently presented as current.

There are three fixed worker contexts: lane1path,2account,3command. Each has
one active request and one result/failure slot, with a lock/event and at most
one thread during its lifetime. State0idle/1active/2ready. No replacement thread
or new same-lane request is admitted while an old provider is blocked. Deadline
limits acceptance, not provider lifetime. Stop is asynchronous and does not join
a provider call on UI. A stopping lane cannot be restarted in the same process.

Demand is derived from visible metadata columns and Processes mode. Demand-mask
changes increment pmDemandEpoch and invalidate pending field serials. Ordinary
process snapshot refreshes do not change that epoch, avoiding starvation. Cached
valid fields can be retained across panel changes, subject to TTL. Late results
must match demand epoch, entry cookie, PID/birth, lane and field serial, and arrive
before their copied deadline. Failure publication does not require allocation.

Scheduler prioritizes selected and native viewport rows, then rotates a per-lane
cursor over all admitted identities. It admits only demanded, due fields. A250ms
native timer services deadlines and refresh; worker-ready message8064h drains
completions and schedules next jobs. On changed values it remembers selection,
rebuilds the map for metadata sorts and invalidates the list. Existing UI
regression mode must not query arbitrary live fixture PIDs; PM_TESTS have an
explicit test-enable seam and owned collector/worker fixtures instead.

## Layout migration and sorting

V2:160bytes,version2,count2..16; IDs24[16], widths88[16], reservedQWORD152.
IDs0..11 retain their exact meaning. New defaults: Account180, Path360,
Command480, Handles90 in96-DPI units; new fields hidden by default.
Name0first and PID6visible remain mandatory; hidden sort falls back to NameASC.

PcLoadLayout accepts exact valid V1 or V2 and publishes only a complete V2 record.
V1 validation is independent with its original12/128/72/120 offsets. Conversion
preserves all old visible IDs/order, sort/direction and all12 widths, including
hidden widths. V2 is saved only to process-columns-v2.bin. Startup consults V1
only if V2 is absent (error2); corrupt/inaccessible V2 retains its diagnostic and
defaults, without silently reviving old settings. V1 remains untouched for
rollback; later normal persistence writes V2 atomically.

All new text fields sort by StrCmpLogicalW, valid before unavailable in both
directions, then stable ascending PID/full64bit birth ties. Handles sorts as
unsigned DWORD with its explicit snapshot validity. Text alignment is left for
Account/Path/Command, numeric right for Handles. Physicalsubitem mapping,
Name/tree geometry, native drag, oldmode/DPI capture and otherpanel semantics
remain as previously verified.

## Acceptance evidence

Model/store: independent V1/V2 byte oracles, nondefault migration, malformed
legacy/new records, output canaries, exact lengths, replacement failure and
owned-temp cleanup. Cache: bounded memory/slot reuse, 5000 identities, lookup,
duplicate/late/wrong-cookie/epoch/serial/deadline rejection, separate field status,
valid empty command, TTL/retry, allocation failure and pointer ownership.
Collectors: owned child vs independent path/account/command/handle observations,
stale identity, expired deadline, error/long output and account/WMI separation.
Workers: real gated provider calls, finite3threads/mailboxes, timeout without
replacement, concurrent path/account progress while command is blocked, stop,
allocation failure and delayed completion. UI: all16 captions and alignments,
display and text/numeric sorts, unknown-last, selected identity, cache demand,
main routing, viewport fairness, EN/RU, themes and96/144/192DPI. Then full aggregate
and PS5.1/PS7 feature gates. Scope limitations remain explicit; no desktop/CUA,
real UAC or mutation of user processes/preferences during fixtures.
