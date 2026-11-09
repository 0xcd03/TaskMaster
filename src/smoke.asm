option casemap:none
includelib kernel32.lib
includelib user32.lib
extern MessageBoxW:proc
extern ExitProcess:proc
.data
cap dw 'T','a','s','k','M','a','n','a','g','e','r',0
txt dw 'M','A','S','M',' ','x','6','4',' ','G','U','I',' ','s','m','o','k','e',' ','t','e','s','t',' ','p','a','s','s','e','d','.',0
.code
SmokeEntry PROC
 sub rsp,28h
 xor ecx,ecx
 lea rdx,txt
 lea r8,cap
 xor r9d,r9d
 call MessageBoxW
 xor ecx,ecx
 call ExitProcess
SmokeEntry ENDP
END


