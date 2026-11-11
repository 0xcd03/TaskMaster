; GPU sampler ABI. UI owns taken frames; provider/model are one-worker-only.
GP_VERSION equ 1
GP_MASK_GLOBAL equ 1
GP_MASK_PROCESS equ 2
GP_MASK_ALL equ 3
GP_MESSAGE equ 8065h
GP_TIMER_ID equ 65h
GP_SAMPLE_MS equ 1000
GP_STALE_MS equ 3000
GP_MAX_GAP_MS equ 5000
GP_ARRAY_BUDGET equ 8388608
GP_ARRAY_RETRIES equ 3
GP_NAME_CAPACITY equ 512
GP_TYPE_CAPACITY equ 64
GP_PROCESS_CAPACITY equ 8192
GP_HASH_CAPACITY equ 16384
GP_ADAPTER_CAPACITY equ 64
GP_ENGINE_CAPACITY equ 4096
GP_HEADER_SIZE equ 256
GP_PROCESS_SIZE equ 256
GP_ADAPTER_SIZE equ 512
GP_ADAPTER_COPY_SIZE equ GP_HEADER_SIZE+GP_ADAPTER_CAPACITY*GP_ADAPTER_SIZE
GP_ENGINE_SIZE equ 192
GP_S_HASH equ GP_HEADER_SIZE
GP_S_PROCESSES equ GP_S_HASH+GP_HASH_CAPACITY*4
GP_S_ADAPTERS equ GP_S_PROCESSES+GP_PROCESS_CAPACITY*GP_PROCESS_SIZE
GP_S_ENGINES equ GP_S_ADAPTERS+GP_ADAPTER_CAPACITY*GP_ADAPTER_SIZE
GP_SNAPSHOT_SIZE equ GP_S_ENGINES+GP_ENGINE_CAPACITY*GP_ENGINE_SIZE
GP_S_SIZE equ 0
GP_S_VERSION equ 4
GP_S_SEQUENCE equ 8
GP_S_EPOCH equ 16
GP_S_TICK equ 24
GP_S_BEGIN_FT equ 32
GP_S_END_FT equ 40
GP_S_BASELINE_FT equ 48
GP_S_INTERVAL_MS equ 56
GP_S_UTIL equ 64
GP_S_UTIL_STATUS equ 68
GP_S_UTIL_ERROR equ 72
GP_S_FLAGS equ 76
GP_S_DEDICATED equ 80
GP_S_SHARED equ 88
GP_S_DEDICATED_STATUS equ 96
GP_S_DEDICATED_ERROR equ 100
GP_S_SHARED_STATUS equ 104
GP_S_SHARED_ERROR equ 108
GP_S_PROCESS_COUNT equ 112
GP_S_ENGINE_COUNT equ 116
GP_S_ADAPTER_COUNT equ 120
GP_S_COVERAGE equ 124
GP_S_SOURCES equ 128 ; five {DWORD status,error} records
GP_S_METADATA_ERROR equ 168
GP_STATUS_MISSING equ 0
GP_STATUS_WARMUP equ 1
GP_STATUS_VALID equ 2
GP_STATUS_UNAVAILABLE equ 3
GP_STATUS_PARTIAL equ 4
GP_STATUS_STALE equ 5
GP_SOURCE_ENGINE equ 0
GP_SOURCE_PROCESS_DEDICATED equ 1
GP_SOURCE_PROCESS_SHARED equ 2
GP_SOURCE_ADAPTER_DEDICATED equ 3
GP_SOURCE_ADAPTER_SHARED equ 4
GP_SOURCES equ 5
GP_P_PID equ 0
GP_P_FLAGS equ 4
GP_P_BIRTH equ 8
GP_P_UTIL equ 16
GP_P_UTIL_STATUS equ 20
GP_P_UTIL_ERROR equ 24
GP_P_UTIL_FLAGS equ 28
GP_P_DEDICATED equ 32
GP_P_DEDICATED_STATUS equ 40
GP_P_DEDICATED_ERROR equ 44
GP_P_SHARED equ 48
GP_P_SHARED_STATUS equ 56
GP_P_SHARED_ERROR equ 60
GP_P_LUID equ 64
GP_P_PHYS equ 72
GP_P_ENGINE equ 76
GP_P_TYPE equ 80 ;64 WCHAR
GP_P_COUNTERS equ 208
GP_P_IDENTITY_ERROR equ 212
GP_A_LUID equ 0
GP_A_PHYS equ 8
GP_A_FLAGS equ 12 ; bit0 metadata valid,bit1 software,bit2 type truncated
GP_A_UTIL equ 16
GP_A_UTIL_STATUS equ 20
GP_A_UTIL_ERROR equ 24
GP_A_DEDICATED equ 32
GP_A_DEDICATED_STATUS equ 40
GP_A_DEDICATED_ERROR equ 44
GP_A_SHARED equ 48
GP_A_SHARED_STATUS equ 56
GP_A_SHARED_ERROR equ 60
GP_A_DEDICATED_CAPACITY equ 64
GP_A_SHARED_CAPACITY equ 72
GP_A_METADATA_STATUS equ 80
GP_A_METADATA_ERROR equ 84
GP_A_DESCRIPTION equ 88 ;128 WCHAR
GP_A_VENDOR equ 344
GP_A_DEVICE equ 348
GP_A_ENGINE_COUNT equ 352
GP_E_LUID equ 0
GP_E_PHYS equ 8
GP_E_ENGINE equ 12
GP_E_UTIL equ 16
GP_E_STATUS equ 20
GP_E_ERROR equ 24
GP_E_FLAGS equ 28
GP_E_TYPE equ 32 ;64 WCHAR
GP_E_PROCESS_COUNT equ 160
; Parsed instance:160bytes. Full engine suffix validated, copiedtype capped63.
GP_PARSED_SIZE equ 160
GP_N_PID equ 0
GP_N_KIND equ 4 ;1engine,2processmemory,3adaptermemory
GP_N_LUID equ 8
GP_N_PHYS equ 16
GP_N_ENGINE equ 20
GP_N_FLAGS equ 24 ;bit0 type truncated
GP_N_TYPE equ 32

; MODEL (one provider worker, no OS/provider calls):
; GpParseName(RCX UTF16name,RDXout160,R8DmaxChars<=512) ->0/13. Output
; unchanged on error. Grammar: pid_D_luid_0xHHHHHHHH_0xLLLLLLLL_phys_D
; optional _eng_D_engtype_NONEMPTY forengine; adapter names omit pid_D_.
; DecimalDWORD/hex8 overflow/completeNUL validation; suffix spaces permitted.
; ASCII structural tokens are case-insensitive; hex accepts bothcases. Ties
; choose ascending unsignedLUID/phys/engine, independently of source order.
; GpModelInit(RCXframe,RDXsequence,R8epoch,R9tick) clears all fixedframe/maps,
; sets size/version/tags; source states defaultMISSING.
; GpModelArray(RCXframe,EDXsource0..4,R8buffer,R9DusedBytes,[rsp+28]DWORDcount)
; ->0valid/error. item24: nameptr0,CStatus8,value16; sources0DOUBLE,
; memory1..4LARGE. Validate count/stride/name bounds/UTF16/NUL before reading.
; AcceptCStatus0/1, finite nonnegativeDOUBLE, nonnegativeLARGE. No silentdrop.
; Whole source becomes PARTIAL on anybaditem/duplicateexactkey/capacity/overflow;
; count0->UNAVAILABLE21. Values from unusablesource never become validzero.
; Model preserves DOUBLE sum samephysicalengine across PIDs; processutil=max
; oneengine; adapterutil=maxengine; globalutil=maxengine. Convertaftergrouping
; to basispoints(roundnearest), clamp0..10000. Type display is notidentity.
; Processmemory sums distinct(LUID,phys) perPID; adaptermemory isindependent.
; GpModelSourceError(RCXframe,EDXsource,R8Dstatus,R9Derror) marks sourceerror.
; GpModelFinalize(RCXframe) materializes statuses/values/ties into records;
; source PARTIAL invalidates all derivedfieldsofthatlane. No birth assigned.
; GpFindProcess(RCXimmutableframe,EDXPID)->RAXrecord/0 uses in-framehash.
; Numeric/modelworkspaces are fixed; noHeapAlloc inmodel.

; PROVIDER (one worker only; OS calls never run under worker lock):
; GpProviderSample(RCXframe,RDXsequence,R8epoch,R9Dmask) ->0/Win32. Always
; initialized frame; query/counter errors represented independently inheaders.
; Owns PDHquery5counters and bounded array scratch8MiB, sizingzero-probe then
; atmost3attempts; nonzeroMORE_DATAalwayszero-reprobe. Rawcollectfailure resets
; ratebaseline. Fresh epoch/gap>5000/warmupcannotproducevalidutilization.
; CounterformatsengineDOUBLE|NOCAP100, memoryLARGE. No arbitraryIDcreation.
; Resolve each observednonzeroPID throughheld limitedqueryhandle/GetProcessTimes
; +GetProcessId afterread; memorybirth<=beginFT. Util additionally requires exact
; previoussamplePIDbirth and birth<=baselineFT. Unknownbirth invalidates process
; fields; PID0 maystillcontribute globalengine. Outputbirth alwaysactual.
; Copycurrentbirthmapbeforepublishing; providerretainsnoframepointer.
; DXGIFactory1EnumAdapters1/GetDesc1 maps exactLUID todescription/capacity/flags;
; unmappedPDHLUID remainsvisibleasLUID/phys. Software adapters labeled, notguessed
; physicalGPU. Refreshmetadata/missingcounter retries <=once30s.
; GpProviderClose() closesquery/freesownedarrays/COM, resetsallbaselines.
; APIcall pointers in gpApi* globals are testseams; GetTick/FILETIME/identity
; resolution replaceable onlyinownedfixtures withno concurrentproviders.

; WORKER (onefixedlifetime thread/event, latestframe mailbox):
; GpWorkerSetDemand(ECXmask0..3,RDXnotifyHWND)->RAXepoch,EDXWin32(0success).
; Nonzero mask requiresGLOBALbitandHWND; lazythreadstartup; mask/HWNDchanges
; incrementepoch. Atmask0 waitnojobs; changedepoch closesproviderbeforefreshjob.
; Oneworkingframe, onereadyframe, oneUIfrontframe =>atmost3frames. Publishing
; replaces unreadreadyframe(freeoutside lock), noqueue/no replacementthread.
; GpWorkerTake(RCXoutErrorDWORD)->RAXownedframe/0,EDX1taken/0none. Fixedfailure
; mailbox includeserror/epoch/tick evenifHeapAllocfails; callercheckscurrentepoch
; using gpWorkerEpoch global underUI demandownership. UIclearsoldframeonfailure.
; SetDemand invalidates ready/error/summary on anyepochchange, includingmask0;
; lateold-epochresults are discarded. Take returns current-epoch failures only.
; GpWorkerFree(RCXownedframe/null) freeheapblock.
; GpWorkerCopySummary(RCXout256)->0/error; copiedunderlock, freshness checked
; using monotonicclock; ifstale/error, outputstatusesclearwitherror. NoUIprovider.
; GpWorkerCopyAdapters(RCXout33024)->0/error; header followed by64adapterrecords
; at+256 (notfullframeoffset). Stale preservesmetadata/count butmarksallusage
; statesSTALE. Failure/epochchangeinvalidatescount. No retainedframepointer.
; GpWorkerStop() asynchronous, nojoin/restart; dropsready/lateframes; worker
; closesownresourcesafterblockedproviderreturns. Noheap/providerwork underlock.
; Globals gpWorkerEpochQ,gpWorkerThreadsDWORD,gpWorkerSamplerQ=GpProviderSample,
; gpWorkerAllocatorQ=HeapAlloc,gpWorkerClockQ=GetTickCount64, gpWorkerContext.
