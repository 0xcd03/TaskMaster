# Native Run new task

Approved direction: user confirmed on 2026-11-10 and authorized subsequent roadmap
work without scenario-by-scenario approval. Keep the full replacement goal and
Task Manager/process-monitoring priority in `docs/ROADMAP.md`.

Expose command 2500 in both Process menus, routed before SettingsCommand. It opens
a modal native EN/RU dialog (303/304), using standard keyboard/tab/Enter/Escape
behavior, with executable, separate literal arguments, optional working directory,
Browse, an explicit administrator checkbox, inline error, Run and Cancel.

The executable and any supplied working directory must be absolute Windows paths.
Use explicit `lpApplicationName` with CreateProcessW and construct only a quoted
argv[0] plus the literal argument field. No shell command concatenation or handle
inheritance. A blank directory means the directory captured when the dialog opens.
Total normal command line including terminating NUL must fit 32767 WCHAR.
The executable and directory buffers must reject overlength rather than truncate.

Normal launch inherits the current TaskMaster token. Display that explicitly,
including when TaskMaster is already elevated. The administrator checkbox uses
ShellExecuteExW `runas`, keeping executable, arguments and directory separate.
Do not attempt token bypass or implicit de-elevation. Close all returned handles.
UAC cancellation (1223) keeps the dialog and fields intact, with a localized status.
Other validation/API failures also keep inputs and show numeric/localized Win32
reason. Successful launch closes the dialog. Do not persist history or arguments.

Shared ABI: LaunchNewTask(RCX=request*) returns EAX Win32 error, zero success.
Request offsets: +0 executable pointer, +8 arguments pointer, +16 directory pointer,
+24 runAs DWORD (0/1), +28 reserved DWORD (zero). Arguments may be an empty string;
directory may be null for caller-inherited cwd in standalone engine tests. UI always
supplies its captured absolute directory. Strings remain valid until the call ends.

Browse uses a supported native picker for existing executable files, preserves
the process current directory, does not record a recent-document item, and leaves
the typed field intact on cancel. COM initialization is balanced on each code path.

Test native behavior with only owned fixture children: exact received argv, cwd,
token state, Unicode/spaces, literal quote/backslash arguments, invalid/relative
paths, command limits, success/failure and no inherited handles. Test cancel/error
branches for elevation without raising real UAC during automated runs. Exercise
actual EN/RU controls in hidden dialog tests, including retained fields, no launch
on Cancel, and command routing independent of selected process/actionBusy.

Do not claim interactive UAC or desktop visual verification from fixture tests.
