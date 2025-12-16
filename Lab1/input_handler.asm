; input_handler.asm - 输入处理模块
.386
.model flat, stdcall
option casemap:none

include windows.inc
include user32.inc
includelib user32.lib

include constants.inc
include structs.inc
include globals.inc
include prototypes.inc    ; 包含函数原型

; 声明公共函数
PUBLIC handleKeyDown, handleKeyUp, handleMouseClick

.CODE

;==============================================================
;>> 处理按键按下
;==============================================================
handleKeyDown proc uses ebx, wParam:DWORD, hWnd:DWORD
    mov ebx, wParam
    
    .if ebx == VK_UP || ebx == "W" || ebx == "w"
        .if inputState.isUpPressed == 0
            mov inputState.isUpPressed, 1
            invoke moveUp
            .if gameData.movedUp == 1
                invoke randomLCG
            .endif
            invoke InvalidateRect, hWnd, NULL, FALSE
        .endif
    
    .elseif ebx == VK_DOWN || ebx == "S" || ebx == "s"
        .if inputState.isDownPressed == 0
            mov inputState.isDownPressed, 1
            invoke moveDown
            .if gameData.movedDown == 1
                invoke randomLCG
            .endif
            invoke InvalidateRect, hWnd, NULL, FALSE
        .endif
    
    .elseif ebx == VK_LEFT || ebx =="A" || ebx == "a"
        .if inputState.isLeftPressed == 0
            mov inputState.isLeftPressed, 1
            invoke moveLeft
            .if gameData.movedLeft == 1
                invoke randomLCG
            .endif
            invoke InvalidateRect, hWnd, NULL, FALSE
        .endif
    
    .elseif ebx == VK_RIGHT || ebx == "D" || ebx == "d"
        .if inputState.isRightPressed == 0
            mov inputState.isRightPressed, 1
            invoke moveRight
            .if gameData.movedRight == 1
                invoke randomLCG
            .endif
            invoke InvalidateRect, hWnd, NULL, FALSE
        .endif
    .endif
    
    invoke canMove
    .if gameData.gameEnd == 1
        invoke MessageBox, windowHandles.hWinMain, \
                offset MSG_GAME_OVER_TXT, offset MSG_GAME_OVER_TITLE, MB_OK
        .if eax == IDOK
            invoke initGameData
            invoke InvalidateRect, hWnd, NULL, FALSE
        .endif
    .endif
    
    ret
handleKeyDown endp

;==============================================================
;>> 处理按键释放
;==============================================================
handleKeyUp proc uses ebx, wParam:DWORD
    mov ebx, wParam
    .if ebx == VK_UP || ebx == "W" || ebx == "w"
        mov inputState.isUpPressed, 0
    .elseif ebx == VK_DOWN || ebx == "S" || ebx == "s"
        mov inputState.isDownPressed, 0
    .elseif ebx == VK_LEFT || ebx == "A" || ebx == "a"
        mov inputState.isLeftPressed, 0
    .elseif ebx == VK_RIGHT || ebx == "D" || ebx == "d"
        mov inputState.isRightPressed, 0
    .endif
    ret
handleKeyUp endp

;==============================================================
;>> 处理鼠标点击
;==============================================================
handleMouseClick proc uses ecx edx, lParam:DWORD, hWnd:DWORD
    mov eax, lParam
    movzx ecx, ax        ; X坐标
    shr eax, 16          ; Y坐标
    movzx edx, ax        ; Y坐标
    
    ; 判断是否点击了重启按钮区域 (640,275)-(960,325)
    .if ecx >= 640 && ecx <= 960
        .if edx >= 275 && edx <= 325
            invoke MessageBox, hWnd, addr MSG_RESTART_CONFIRM, \
                    addr MSG_RESTART_TITLE, MB_YESNO
            .if eax == IDYES
                invoke initGameData
                invoke InvalidateRect, hWnd, NULL, FALSE
            .endif
        .endif
    .endif
    ret
handleMouseClick endp

END
