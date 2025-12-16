; main.asm - 主程序入口
.386
.model flat, stdcall
option casemap:none

include windows.inc
include msvcrt.inc
includelib msvcrt.lib
include kernel32.inc
includelib kernel32.lib
include user32.inc
includelib user32.lib
includelib gdi32.lib
include gdi32.inc

include constants.inc
include structs.inc
include prototypes.inc

;==============================================================
;>> 数据段 - 所有变量在这里定义一次
;==============================================================
.data
    ; 窗口句柄
    windowHandles WINDOW_HANDLES <>
    
    ; 游戏数据
    gameData GAME_DATA <>
    
    ; 输入状态
    inputState INPUT_STATE <>
    
    ; 绘制资源
    drawRes DRAW_RESOURCES <>
    
    ; 输出格式
    szNumFormat db "%d", 0
    
    ;==========================================================
    ;>> 所有字符串常量定义
    ;==========================================================
    szClassName db "MainWindowClass", 0
    szTitleMain db "合成北理工", 0
    szFontName db "Microsoft YaHei", 0
    
    TXT_TITLE db "合成北理工", 0
    TXT_CURRENT_SCORE db "SCORE", 0
    TXT_BEST_SCORE db "BEST", 0
    TXT_INTRO_TITLE db "操作指南", 0
    TXT_INTRO_TXTw db "W：向上", 0
    TXT_INTRO_TXTa db "A：向左", 0
    TXT_INTRO_TXTs db "S：向下", 0
    TXT_INTRO_TXTd db "D：向右", 0
    
    sz2 db "民大", 0
    sz4 db "北林", 0
    sz8 db "农大", 0
    sz16 db "央财", 0
    sz32 db "北交", 0
    sz64 db "北邮", 0
    sz128 db "北师", 0
    sz256 db "北航", 0
    sz512 db "北大", 0
    sz1024 db "清华", 0
    sz2048 db "北理", 0
    sz4096 db "良理", 0
    sz8192 db "学神", 0
    szOther db "魔鬼", 0
    
    MSG_WIN_TITLE db "游戏胜利！", 0
    MSG_WIN db "您的分数已经到达2048!", 0
    MSG_CONTINUE db "您希望继续游戏吗？（进入无尽模式）", 0
    MSG_GAME_OVER_TITLE db "游戏结束！", 0
    MSG_GAME_OVER_TXT db "无法继续移动，游戏结束！", 0
    
    TXT_BUTTON_RESTART db "重新开始", 0
    MSG_RESTART_TITLE db "重新开始", 0
    MSG_RESTART_CONFIRM db "确定要重新开始游戏吗？当前进度将丢失。", 0

;==============================================================
;>> 代码段
;==============================================================
.code

; 声明所有全局变量为PUBLIC，让其他模块可以访问
PUBLIC windowHandles, gameData, inputState, drawRes, szNumFormat
PUBLIC szClassName, szTitleMain, szFontName
PUBLIC TXT_TITLE, TXT_CURRENT_SCORE, TXT_BEST_SCORE
PUBLIC TXT_INTRO_TITLE, TXT_INTRO_TXTw, TXT_INTRO_TXTa
PUBLIC TXT_INTRO_TXTs, TXT_INTRO_TXTd
PUBLIC sz2, sz4, sz8, sz16, sz32, sz64, sz128, sz256
PUBLIC sz512, sz1024, sz2048, sz4096, sz8192, szOther
PUBLIC MSG_WIN_TITLE, MSG_WIN, MSG_CONTINUE
PUBLIC MSG_GAME_OVER_TITLE, MSG_GAME_OVER_TXT
PUBLIC TXT_BUTTON_RESTART, MSG_RESTART_TITLE, MSG_RESTART_CONFIRM

; 声明函数为PUBLIC
PUBLIC main, WinMain, WinMainProc

;==============================================================
;>> 窗口主函数
;==============================================================
WinMain proc
    LOCAL wcex:WNDCLASSEX
    LOCAL msg:MSG

    ; 获取模块句柄
    invoke GetModuleHandle, NULL
    mov windowHandles.hInstance, eax

    ; 设置窗口类的信息
    invoke RtlZeroMemory, addr wcex, sizeof wcex
    mov wcex.cbSize, sizeof WNDCLASSEX
    push windowHandles.hInstance
    pop wcex.hInstance
    invoke LoadCursor, 0, IDC_ARROW
    mov wcex.hCursor, eax
    mov wcex.style, CS_HREDRAW or CS_VREDRAW
    mov wcex.hbrBackground, COLOR_WINDOW+1
    mov wcex.lpszClassName, offset szClassName
    mov wcex.lpfnWndProc, offset WinMainProc

    ; 注册窗口类
    invoke RegisterClassEx, addr wcex
    .if eax == 0
        ; 注册失败，退出
        invoke ExitProcess, NULL
    .endif

    invoke initFonts ; 初始化字体
    ; 创建窗口
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, \
            offset szClassName, offset szTitleMain, \
            WS_OVERLAPPED or WS_CAPTION or WS_SYSMENU, \
            400, 50, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, \
            windowHandles.hInstance, NULL
    
    mov windowHandles.hWinMain, eax
    
    ; 显示窗口
    invoke ShowWindow, windowHandles.hWinMain, SW_SHOWNORMAL
    invoke UpdateWindow, windowHandles.hWinMain  

    ; 消息循环
    .while TRUE
        invoke GetMessage, addr msg, NULL, 0, 0
        .if eax == 0
            .break
        .endif
        invoke TranslateMessage, addr msg
        invoke DispatchMessage, addr msg
    .endw
    
    ; 清理资源
    invoke cleanupResources
    
    ret
WinMain endp

;==============================================================
;>> 窗口过程函数
;==============================================================
WinMainProc proc, hWnd:DWORD, uMsg:DWORD, wParam:DWORD, lParam:DWORD
    LOCAL paintStruct:PAINTSTRUCT
    LOCAL hDc:DWORD

    .if uMsg == WM_CLOSE
        invoke DestroyWindow, windowHandles.hWinMain
        invoke PostQuitMessage, 0

    .elseif uMsg == WM_DESTROY
        invoke PostQuitMessage, 0
    
    .elseif uMsg == WM_CREATE
        invoke initFonts
        invoke initGameData

    .elseif uMsg == WM_KEYDOWN
        ; 调用输入处理模块
        invoke handleKeyDown, wParam, hWnd
    
    .elseif uMsg == WM_KEYUP
        ; 调用输入处理模块
        invoke handleKeyUp, wParam
    
    .elseif uMsg == WM_LBUTTONDOWN
        ; 处理鼠标点击
        invoke handleMouseClick, lParam, hWnd
    
    .elseif uMsg == WM_PAINT
        invoke BeginPaint, hWnd, addr paintStruct
        mov hDc, eax
        invoke updateGameWnd, hWnd, hDc
        invoke EndPaint, hWnd, addr paintStruct
    
    .else
        invoke DefWindowProc, hWnd, uMsg, wParam, lParam
        ret
    .endif
    
    xor eax, eax
    ret
WinMainProc endp

;==============================================================
;>> 主函数
;==============================================================
main proc
    invoke WinMain ;调用窗口函数
    invoke ExitProcess, NULL ;退出
    ret
main endp

END main