; Table metadata is independent of selected-details singleton workers.
; Includes process_details_contract.inc first. Requests/snapshots reuse PD ABI,
; with lane1path/2account/3command, and copied identity/cookie/epoch/serial.
PM_LANE_PATH equ 1
PM_LANE_ACCOUNT equ 2
PM_LANE_COMMAND equ 3
PM_LANES equ 3
PM_MASK_PATH equ 1
PM_MASK_ACCOUNT equ 2
PM_MASK_COMMAND equ 4
PM_MASK_ALL equ 7
PM_READY_MESSAGE equ 8064h
PM_TIMER_ID equ 64h
PM_TIMER_MS equ 250
PM_DEADLINE_MS equ 3000
PM_TTL_MS equ 60000
PM_RETRY_MS equ 15000
PM_CACHE_CAPACITY equ 8192
PM_HASH_CAPACITY equ 16384
PM_STRING_BUDGET equ 33554432
PM_ENTRY_SIZE equ 192
PM_E_PID equ 0
PM_E_FLAGS equ 4 ; bit0 occupied, bit1 terminal exited/mismatch
PM_E_BIRTH equ 8
PM_E_FRAME equ 16
PM_E_RESERVED equ 24
PM_E_COOKIE equ 32
PM_E_RESERVED2 equ 40
PM_E_FIELDS equ 48
PM_FIELD_SIZE equ 48
PM_F_POINTER equ 0
PM_F_BYTES equ 8
PM_F_STATUS equ 12
PM_F_ERROR equ 16
PM_F_FLAGS equ 20 ; PD source flags
PM_F_TICK equ 24
PM_F_SERIAL equ 32
PM_F_DUE equ 40 ; acceptance deadline while loading, refresh/retry otherwise
PM_STATUS_MISSING equ 0
PM_STATUS_LOADING equ 1
PM_STATUS_VALID equ 2
PM_STATUS_UNAVAILABLE equ 3
PM_STATUS_TIMEOUT equ 4
PM_ROW_HANDLES equ 3904
PM_ROW_HANDLES_VALID equ 3908
PM_WORKER_CONTEXT_SIZE equ 256
PM_W_LOCK equ 0
PM_W_EVENT equ 8
PM_W_THREAD equ 16
PM_W_RESULT equ 24
PM_W_STATE equ 32 ; 0idle/1active/2ready
PM_W_STOP equ 36
PM_W_ERROR equ 40
PM_W_THREADS equ 44
PM_W_COMPLETED equ 48
PM_W_PUBLISHED equ 52
PM_W_LANE equ 56
PM_W_REQUEST equ 64 ; immutable96bytes while state1/2

; CACHE APIs (all UI-thread only; no process/provider calls):
; PmCacheReset() frees allpayload, clearsdescriptors/hash/demand/counters,
;   but cookie/serial generators MUST NOT reuse numbers over process lifetime.
; PmCacheSetClock(RCX now) sets pmNow; tests provide clock explicitly.
; PmCacheSetDemand(ECX mask0..7,RDXnotifyHWND) ->0/87. Changes increment
;   pmDemandEpoch and invalidate loading serials; existing valid cache retained.
; PmCacheBeginFrame() increments pmFrame.
; PmCacheMark(ECXpid,RDXbirth) marks existing only, returns entry/0.
; PmCachePrune() removes unmarkedentries, freesfields, invalidates oldcookie.
; PmCacheEnsure(ECXpid,RDXbirth) ->RAXentry/0; nonzero PID/birth required.
; PmCacheGet(ECXpid,RDXbirth,R8Dlane1..3) ->RAX immutableUTF16/0,
;   EDX PMstatus, R8Derror, R9Dflags. Expired valid is effectivelyloading,
;   never presented as fresh. pmNow is stable through one UI dispatch.
; PmCachePrepare(ECXpid,RDXbirth,R8Dlane,R9outRequest96) ->0ready/170notdue/
;   87invalid/1168missing. Requires demandedlane, nonterminalentry. Marksloading,
;   new serial/deadline, frees oldexpiredpayload, fills PD request completely:
;   version1, flags/reserved/context/cancel0, cookieentrycookie,
;   generation=epoch=pmDemandEpoch, unique sequence, notify=cacheconfiguredHWND.
; PmCacheNext(ECXlane,RDXoutRequest96)->0ready/259none/87invalid; fair cursor
;   advances after acceptedrequest, checks all admittedslots once atmost.
; PmCacheComplete(RCXrequest96,RDXsnapshot134080or0,R8DWin32error)->0accepted/
;   13staleorinvalid/1460expired. Matchrequest cookie/PID/birth/generation/epoch/
;   serial/lane/deadline before statechanges. Completion may match loading or
;   timer-marked timeout. On late current request settimeout1460/retry15s and
;   serial0. Error beforedeadline setsunavailable/error/retry; noallocationneeded.
;   Validate snapshot size/version/lane/cookie/gen/seq/epoch/PID/birth and source
;   status/length/capacity/exactNUL/noembeddedNUL before copying valid text.
;   Relevant source fields: pathPD_F_PATH,accountPD_F_ACCOUNT,commandPD_F_COMMAND.
;   Source VALID empty string is valid (allocatedNUL). Invalidmalformedmetadata
;   never becomes VALID. Exit/mismatch of a wellformed matching result makes
;   entryterminal and clears all3fields; stale/malformed headers do not.
; PmCacheExpire(RCXrequest96)->EAX1ifcurrentloadingbecametimeout/0otherwise.
; Globals pmNowQ,pmDemandEpochQ,pmFrameQ,pmBytesQ,pmEntryCountDWORD,
;   pmCacheAllocateQ=HeapAlloc and pmCacheBudgetQ=PM_STRING_BUDGET testseams.
; pmCacheBudget may be lowered only by ownedtests with no concurrentoperations.

; WORKER APIs:
; PmCollectField(RCXrequest96,RDXsnapshot,R8Dcapacity)->EAX0orWin32.
;   Strict PMlane/header validation before touching output. Path delegates to
;   PdCollectNative; optional uses separate verifiedheldhandle/helperwrappers.
; PmWorkerSubmit(RCXrequest96)->0accepted/170busy/Win32failure. Validaterequest
;   before mutation. Lazycreateoneperlane; neveroverwriteactive/unreadresult.
; PmWorkerBusy(ECXlane)->EAX1active/ready/stopping,0idle; invalidlane=>1.
; PmWorkerTake(ECXlane,RDXoutRequest96,R8outErrorDWORD)->RAXownedSnapshot/0,
;   EDX1taken/0notready. Fixed request+error copied even on allocationfailure.
;   State becomesidle aftertake, unlessstopping. No nullout pointers accepted.
; PmWorkerFree(RCXownedSnapshot) frees returnedownedblock; nullsafe.
; PmWorkerStop() asynchronousalllanes, no join/replacement/restart.
; Globals pmWorkerContexts[3*256], pmWorkerCollectorQ=PmCollectField,
; pmWorkerAllocateQ=HeapAlloc, pmWorkerClockQ=GetTickCount64.
; Requests require PDsize/version1, lane1..3, nonzeroPID/birth/cookie/sequence/
;   generation/epoch, flags/reserved/context/cancel0, nonzero deadline andnotify.
; Workerchecksdeadline beforecollection andpublication; late results free and
;   fixed failure1460 published. Stop dropsresults andclosesownresourceslater.
