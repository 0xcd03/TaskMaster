; Runtime suppression flag is zero in production. Tests alone set it to one.
; Keep the symbol outside the hidden UI fixture and preserve section state.
; These APIs were previously declared as a side effect of fixture includes.
extern CreateProcessW:proc
extern GetWindowTextLengthW:proc, IsDialogMessageW:proc
.data
align 4
uiRegressionMode dd 0
.code
