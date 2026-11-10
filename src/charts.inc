extern D2D1CreateFactory:proc, DWriteCreateFactory:proc
extern BeginPaint:proc, EndPaint:proc, DrawTextW:proc, FillRect:proc
extern lstrlenW:proc
.data
chartWnd dq 0
d2dFactory dq 0
dwriteFactory dq 0
chartTarget dq 0
chartBrush dq 0
chartFormat dq 0
chartStatus dd 0
chartHead dd 0
chartSamples dd 0
chartFrame dd 0
chartSeries dd 0
chartPoint dd 0
chartScale dd 100
chartXStep real4 1.0
chartYScale real4 1.0
chartTop real4 0.0
chartBottom real4 0.0
chartRight real4 0.0
chartLineWidth real4 1.7
chartWidth dd 0
chartHeight dd 0
chartRect dd 0,0,0,0
chartFRect real4 12.0,0.0,0.0,0.0
chartP1 real4 0.0,0.0
chartP2 real4 0.0,0.0
chartClear real4 0.98,0.98,0.99,1.0
chartInk real4 0.1,0.15,0.2,1.0
chartBlue real4 0.0,0.45,0.85,1.0
chartColors real4 0.0,0.45,0.85,1.0, 0.55,0.2,0.7,1.0, 0.0,0.55,0.35,1.0, 0.8,0.4,0.0,1.0, 0.2,0.5,0.65,1.0
d2dIID dd 06152247h
dw 6F50h,465Ah
db 92h,45h,11h,8Bh,0FDh,3Bh,60h,7
dwriteIID dd 0B859EE5Ah
dw 0D838h,4B5Bh
db 0A2h,0E8h,1Ah,0DCh,7Dh,93h,0DBh,48h
chartClass dw 'T','M','X','C','h','a','r','t',0
chartLabels dq chartCpuLabel,chartRamLabel,chartGpuLabel,chartDiskLabel,chartNetLabel
chartLocale dw 'e','n','-','u','s',0
.data?
align 8
chartWC db 80 dup(?)
chartPaint db 72 dup(?)
chartProps db 28 dup(?)
chartHwndProps db 24 dup(?)
chartData dd 900 dup(?) ; 5 x 180 UI-owned samples
chartBack dd 5 dup(?) ; published by worker, copied before ack
chartText dw 256 dup(?)
.code
ReleaseCom PROC
  test rcx,rcx
  jz relDone
  mov rax,[rcx]
  jmp qword ptr [rax+16]
relDone:
  ret
ReleaseCom ENDP

DropChartTarget PROC
  sub rsp,28h
  mov rcx,chartBrush
  call ReleaseCom
  mov chartBrush,0
  mov rcx,chartTarget
  call ReleaseCom
  mov chartTarget,0
  add rsp,28h
  ret
DropChartTarget ENDP

CreateChart PROC
  sub rsp,68h
  xor ecx,ecx
  lea rdx,d2dIID
  xor r8d,r8d
  lea r9,d2dFactory
  call D2D1CreateFactory
  mov chartStatus,eax
  xor ecx,ecx
  lea rdx,dwriteIID
  lea r8,dwriteFactory
  call DWriteCreateFactory
  test eax,eax
  js ccWindow
  call CreateChartFormat
ccWindow:
  mov dword ptr [chartWC],80
  lea rax,ChartWndProc
  mov qword ptr [chartWC+8],rax
  mov rax,gInst
  mov qword ptr [chartWC+24],rax
  lea rax,chartClass
  mov qword ptr [chartWC+64],rax
  lea rcx,chartWC
  call RegisterClassExW
  xor ecx,ecx
  lea rdx,chartClass
  lea r8,chartTitle
  mov r9d,40000000h
  mov qword ptr [rsp+20h],200
  mov qword ptr [rsp+28h],300
  mov qword ptr [rsp+30h],700
  mov qword ptr [rsp+38h],300
  mov rax,gMain
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],9100
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov chartWnd,rax
  add rsp,68h
  ret
CreateChart ENDP

ChartWndProc PROC
  sub rsp,28h
  cmp edx,14h
  je cwpErased
  cmp edx,0Fh
  je cwpPaint
  cmp edx,5
  je cwpSize
  cmp edx,2
  je cwpDestroy
  call DefWindowProcW
  jmp cwpReturn
cwpPaint:
  mov chartWnd,rcx
  call PaintChart
  xor eax,eax
  jmp cwpReturn
cwpSize:
  call DropChartTarget
  xor eax,eax
  jmp cwpReturn
cwpDestroy:
  call DropChartTarget
  xor eax,eax
  jmp cwpReturn
cwpErased:
  mov eax,1
cwpReturn:
  add rsp,28h
  ret
ChartWndProc ENDP

AddChartSample PROC
  lea rdx,chartBack
  lea r8,chartData
  mov ecx,chartHead
  xor eax,eax
acsLoop:
  mov r9d,[rdx+rax*4]
  mov [r8+rcx*4],r9d
  add r8,720
  inc eax
  cmp eax,5
  jb acsLoop
  inc chartHead
  cmp chartHead,180
  jb acsCount
  mov chartHead,0
acsCount:
  cmp chartSamples,180
  jae acsDone
  inc chartSamples
acsDone:
  ret
AddChartSample ENDP

LayoutContent PROC
  sub rsp,38h
  mov rcx,gMain
  lea rdx,gRect
  call GetClientRect
  mov rcx,chartWnd
  xor edx,edx
  cmp frontMode,4
  jne lcShow
  mov edx,5
lcShow:
  call ShowWindow
  mov eax,gRect.rcRight
  sub eax,uiContentSpace
  cmp eax,uiMinListHeight
  jge lcWidth
  mov eax,uiMinListHeight
lcWidth:
  mov [rsp+30h],eax
  mov eax,gRect.rcBottom
  sub eax,uiBottomSpace
  sub eax,uiStatusHeight
  cmp frontMode,4
  jne lcList
  sub eax,uiChartSpace
lcList:
  cmp eax,uiMinListHeight
  jge lcHeight
  mov eax,uiMinListHeight
lcHeight:
  mov [rsp+20h],eax
  mov qword ptr [rsp+28h],1
  mov rcx,gList
  mov edx,uiContentLeft
  mov r8d,uiListTop
  mov r9d,[rsp+30h]
  call MoveWindow
  ; The final column is fitted after the selected panel has finished
  ; rebuilding its columns. WM_SIZE may arrive reentrantly during creation.
lcSkipFit:
  ; The result panel occupies the bottom strip and uses the same themed
  ; background as the rest of the application.
  cmp uiRegressionMode,0
  jne lcSkipBanner
  mov rcx,actionBanner
  mov edx,uiContentLeft
  mov eax,gRect.rcBottom
  sub eax,uiStatusHeight
  mov r8d,eax
  mov r9d,gRect.rcRight
  sub r9d,uiContentSpace
  mov eax,uiStatusHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
lcSkipBanner:
  mov rcx,actionBannerDetails
  mov edx,uiContentLeft
  add edx,530
  mov eax,gRect.rcBottom
  sub eax,uiStatusHeight
  mov r8d,eax
  mov r9d,90
  mov eax,uiStatusHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  mov eax,gRect.rcBottom
  sub eax,uiChartBottom
  cmp eax,uiChartMinTop
  jge lcChart
  mov eax,uiChartMinTop
lcChart:
  mov r8d,eax
  mov rcx,chartWnd
  mov edx,uiContentLeft
  mov r9d,[rsp+30h]
  mov eax,uiChartHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  call LayoutSidebar
  call LayoutSearch
  add rsp,38h
  ret
LayoutContent ENDP

PaintChart PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  mov rcx,chartWnd
  lea rdx,chartPaint
  call BeginPaint
  mov rbx,rax
  mov rcx,chartWnd
  lea rdx,chartRect
  call GetClientRect
  cmp d2dFactory,0
  je pcFallback
  cmp chartFormat,0
  je pcFallback
  cmp chartTarget,0
  jne pcBegin
  mov rax,chartWnd
  mov qword ptr [chartHwndProps],rax
  mov eax,[chartRect+8]
  mov dword ptr [chartHwndProps+8],eax
  mov eax,[chartRect+12]
  mov dword ptr [chartHwndProps+12],eax
  mov dword ptr [chartProps+12],42C00000h ; explicit 96 DPI: geometry is in scaled pixels
  mov dword ptr [chartProps+16],42C00000h
  mov rcx,d2dFactory
  lea rdx,chartProps
  lea r8,chartHwndProps
  lea r9,chartTarget
  mov rax,[rcx]
  call qword ptr [rax+112]
  mov chartStatus,eax
  test eax,eax
  js pcFallback
  mov rcx,chartTarget
  lea rdx,chartBlue
  xor r8d,r8d
  lea r9,chartBrush
  mov rax,[rcx]
  call qword ptr [rax+64]
  test eax,eax
  js pcLost
pcBegin:
  mov rcx,chartTarget
  mov rax,[rcx]
  call qword ptr [rax+384]
  mov chartClear,3F7AE148h
  mov chartClear+4,3F7AE148h
  mov chartClear+8,3F7D70A4h
  cmp themeOption,0
  je pcClear
  mov chartClear,3E000000h
  mov chartClear+4,3E000000h
  mov chartClear+8,3E000000h
pcClear:
  mov rcx,chartTarget
  lea rdx,chartClear
  mov rax,[rcx]
  call qword ptr [rax+376]
  mov eax,[chartRect+8]
  sub eax,uiInset
  sub eax,uiInset
  cvtsi2ss xmm0,eax
  movss chartRight,xmm0
  cvtsi2ss xmm1,uiInset
  movss chartFRect,xmm1
  mov eax,179
  cvtsi2ss xmm1,eax
  divss xmm0,xmm1
  movss chartXStep,xmm0
  mov chartSeries,0
pcSeries:
  mov eax,chartSeries
  imul eax,16
  lea rdx,chartColors
  add rdx,rax
  mov rcx,chartBrush
  mov rax,[rcx]
  call qword ptr [rax+64] ; ID2D1SolidColorBrush::SetColor, slot 8
  mov eax,chartSeries
  imul eax,uiGraphRow
  add eax,uiGraphTop
  cvtsi2ss xmm0,eax
  movss chartTop,xmm0
  add eax,uiGraphHeight
  cvtsi2ss xmm0,eax
  movss chartBottom,xmm0
  mov eax,chartSeries
  imul eax,720
  lea rsi,chartData
  add rsi,rax
  mov chartScale,100
  cmp chartSeries,3
  jb pcLabel
  mov chartScale,1
  xor ecx,ecx
pcMax:
  mov eax,[rsi+rcx*4]
  cmp eax,-1
  je pcMaxNext
  cmp eax,chartScale
  jbe pcMaxNext
  mov chartScale,eax
pcMaxNext:
  inc ecx
  cmp ecx,180
  jb pcMax
pcLabel:
  mov eax,chartHead
  add eax,179
  xor edx,edx
  mov ecx,180
  div ecx
  mov r9d,[rsi+rdx*4]
  mov eax,chartSeries
  lea rdx,chartLabels
  mov r8,[rdx+rax*8]
  lea rcx,chartText
  lea rdx,chartCaptionFmt
  cmp r9d,-1
  jne pcAvailable
  lea rdx,chartUnavailableFmt
pcAvailable:
  mov eax,chartScale
  mov [rsp+20h],eax
  call wsprintfW
  movss xmm0,chartTop
  mov eax,uiGraphLabel
  cvtsi2ss xmm1,eax
  subss xmm0,xmm1
  movss [chartFRect+4],xmm0
  movss xmm0,chartBottom
  movss [chartFRect+12],xmm0
  movss xmm0,chartRight
  movss [chartFRect+8],xmm0
  mov rcx,chartTarget
  lea rdx,chartText
  mov r8d,eax ; overwritten by wsprintf result? use string length below
  mov [rsp+58h],rcx
  mov rcx,rdx
  call lstrlenW
  mov r8d,eax
  mov rcx,chartTarget
  lea rdx,chartText
  mov r9,chartFormat
  lea rax,chartFRect
  mov [rsp+20h],rax
  mov rax,chartBrush
  mov [rsp+28h],rax
  mov qword ptr [rsp+30h],2 ; clip
  mov qword ptr [rsp+38h],0
  mov rax,[rcx]
  call qword ptr [rax+216]
  xor edi,edi
  mov ebx,chartHead
  mov eax,180
  sub eax,chartSamples
  add ebx,eax
  cmp ebx,180
  jb pcStart
  sub ebx,180
pcStart:
  mov chartPoint,0
pcPoint:
  cmp edi,chartSamples
  jae pcNextSeries
  mov eax,180
  sub eax,chartSamples
  add eax,edi
  cvtsi2ss xmm0,eax
  mulss xmm0,chartXStep
  mov eax,uiInset
  cvtsi2ss xmm1,eax
  addss xmm0,xmm1
  movss [chartP2],xmm0
  mov eax,[rsi+rbx*4]
  cmp eax,-1
  jne pcValidPoint
  mov chartPoint,0
  jmp pcSkipPoint
pcValidPoint:
  cmp eax,chartScale
  jbe pcClamp
  mov eax,chartScale
pcClamp:
  cvtsi2ss xmm0,rax
  cvtsi2ss xmm1,chartScale
  divss xmm0,xmm1
  mov eax,uiGraphHeight
  cvtsi2ss xmm1,eax
  mulss xmm0,xmm1
  movss xmm1,chartBottom
  subss xmm1,xmm0
  movss [chartP2+4],xmm1
  cmp chartPoint,0
  jz pcFirst
  mov rcx,chartTarget
  mov rdx,qword ptr [chartP1]
  mov r8,qword ptr [chartP2]
  mov r9,chartBrush
  mov eax,chartLineWidth
  mov [rsp+20h],eax
  mov qword ptr [rsp+28h],0
  mov rax,[rcx]
  call qword ptr [rax+120]
pcFirst:
  mov chartPoint,1
  mov rax,qword ptr [chartP2]
  mov qword ptr [chartP1],rax
pcSkipPoint:
  inc edi
  inc ebx
  cmp ebx,180
  jb pcPoint
  xor ebx,ebx
  jmp pcPoint
pcNextSeries:
  inc chartSeries
  cmp chartSeries,5
  jb pcSeries
  mov rcx,chartTarget
  xor edx,edx
  xor r8d,r8d
  mov rax,[rcx]
  call qword ptr [rax+392]
  mov chartStatus,eax
  test eax,eax
  jns pcEnd
pcLost:
  call DropChartTarget
  jmp pcEnd
pcFallback:
  mov rcx,rbx
  lea rdx,chartRect
  mov r8d,6
  call FillRect
  mov rcx,rbx
  lea rdx,chartFallback
  mov r8d,-1
  lea r9,chartRect
  mov qword ptr [rsp+20h],10h
  call DrawTextW
pcEnd:
  mov rcx,chartWnd
  lea rdx,chartPaint
  call EndPaint
  add rsp,60h
  pop rdi
  pop rsi
  pop rbx
  ret
PaintChart ENDP
