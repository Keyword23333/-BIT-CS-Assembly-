; ui_draw.asm - 界面绘制模块
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
include globals.inc
include prototypes.inc

; 公开函数
PUBLIC updateGameWnd

.CODE

;==============================================================
;>> 更新窗口显示
;==============================================================
updateGameWnd proc hWnd:DWORD, hDc:DWORD
    local len:DWORD
    local szBuffer[10]:BYTE
    local tmpHandler:DWORD, _hDc:DWORD, _cptBmp:DWORD
    local cellStartX:DWORD, cellEndX:DWORD, cellStartY:DWORD, cellEndY:DWORD
    local numStartX:DWORD, numStartY:DWORD
    local i:DWORD, j:DWORD, currentNum:DWORD
    
    pushad

    invoke CreateCompatibleDC, hDc
    mov _hDc, eax
    invoke CreateCompatibleBitmap, hDc, WINDOW_WIDTH, WINDOW_HEIGHT
    mov _cptBmp, eax
    invoke SelectObject, _hDc, _cptBmp

    ; 绘制整体背景
    invoke CreateSolidBrush, COLOR_WINDOW_BG
    mov tmpHandler, eax
    invoke SelectObject, _hDc, eax
    invoke Rectangle, _hDc, -10, -10, WINDOW_WIDTH, WINDOW_HEIGHT
    invoke DeleteObject, tmpHandler

    ; 绘制4*4数字矩阵背景
    invoke GetStockObject, NULL_PEN
    invoke SelectObject, _hDc, eax
    invoke CreateSolidBrush, COLOR_MATRIX_BG
    mov tmpHandler, eax
    invoke SelectObject, _hDc, eax
    invoke RoundRect, _hDc, WIDGET_MAT_START_XY, WIDGET_MAT_START_XY, \
            WIDGET_MAT_END_XY, WIDGET_MAT_END_XY, 6, 6
    invoke SetBkMode, _hDc, TRANSPARENT
    
    ; 右侧信息栏背景
    invoke RoundRect, _hDc, WIDGET_TITLE_START_X, WIDGET_TITLE_START_Y, \
            WIDGET_TITLE_END_X, WIDGET_TITLE_END_Y, 3, 3
    invoke RoundRect, _hDc, WIDGET_TITLE_START_X, WIDGET_SCORE_START_Y, \
            WIDGET_CURRENT_SCORE_END_X, WIDGET_SCORE_END_Y, 3, 3
    invoke RoundRect, _hDc, WIDGET_MAX_SCORE_START_X, WIDGET_SCORE_START_Y, \
            WIDGET_TITLE_END_X, WIDGET_SCORE_END_Y, 3, 3
    invoke RoundRect, _hDc, WIDGET_TITLE_START_X, WIDGET_INTRO_START_Y, \
            WIDGET_TITLE_END_X, WIDGET_INTRO_END_Y, 3, 3
    invoke RoundRect, _hDc, WIDGET_TITLE_START_X, WIDGET_RE_START_Y, \
            WIDGET_TITLE_END_X, WIDGET_RE_END_Y, 3, 3
    invoke DeleteObject, tmpHandler

    ; 右侧信息栏文字
    invoke SelectObject, _hDc, drawRes.FONT_TITLE
    invoke SetTextColor, _hDc, COLOR_TITLE_TXT
    invoke TextOut, _hDc, TXT_TITLE_START_X, TXT_TITLE_START_Y, \
            addr TXT_TITLE, 10
    
    invoke SelectObject, _hDc, drawRes.FONT_SCORE_TXT
    invoke SetTextColor, _hDc, COLOR_SCORE_TXT
    invoke TextOut, _hDc, TXT_CURRENT_SCORE_TXT_START_X, TXT_SCORE_TXT_START_Y, \
            addr TXT_CURRENT_SCORE, 5
    
    invoke updateScore
    invoke wsprintf, addr szBuffer, addr szNumFormat, gameData.score
    mov len, eax
    invoke SelectObject, _hDc, drawRes.FONT_SCORE_NUM
    invoke SetTextColor, _hDc, COLOR_SCORE_NUM
    invoke TextOut, _hDc, TXT_CURRENT_SCORE_NUM_START_X, TXT_SCORE_NUM_START_Y, \
            addr szBuffer, len
    
    invoke SelectObject, _hDc, drawRes.FONT_SCORE_TXT
    invoke SetTextColor, _hDc, COLOR_SCORE_TXT
    invoke TextOut, _hDc, TXT_MAX_SCORE_TXT_START_X, TXT_SCORE_TXT_START_Y, \
            addr TXT_BEST_SCORE, 4
    
    invoke updateBestScore
    invoke wsprintf, addr szBuffer, addr szNumFormat, gameData.bestScore
    mov len, eax
    invoke SelectObject, _hDc, drawRes.FONT_SCORE_NUM
    invoke SetTextColor, _hDc, COLOR_SCORE_NUM
    invoke TextOut, _hDc, TXT_MAX_SCORE_NUM_START_X, TXT_SCORE_NUM_START_Y, \
            addr szBuffer, len
    
    invoke SelectObject, _hDc, drawRes.FONT_RESTART
    invoke SetTextColor, _hDc, COLOR_BUTTON_TXT
    invoke TextOut, _hDc, TXT_RE_START_X, TXT_RE_START_Y, \
            addr TXT_BUTTON_RESTART, 8
    
    invoke SelectObject, _hDc, drawRes.FONT_INTRO_TITLE
    invoke SetTextColor, _hDc, COLOR_INTRO_TITLE
    invoke TextOut, _hDc, TXT_INTRO_TITLE_START_X, TXT_INTRO_TITLE_START_Y, \
            addr TXT_INTRO_TITLE, 8
    
    invoke SelectObject, _hDc, drawRes.FONT_INTRO_TXT
    invoke SetTextColor, _hDc, COLOR_INTRO_TXT
    invoke TextOut, _hDc, TXT_INTRO_INFO_START_X, TXT_INTRO_INFO_START_Yw, \
            addr TXT_INTRO_TXTw, 7
    invoke TextOut, _hDc, TXT_INTRO_INFO_START_X, TXT_INTRO_INFO_START_Ya, \
            addr TXT_INTRO_TXTa, 7
    invoke TextOut, _hDc, TXT_INTRO_INFO_START_X, TXT_INTRO_INFO_START_Ys, \
            addr TXT_INTRO_TXTs, 7
    invoke TextOut, _hDc, TXT_INTRO_INFO_START_X, TXT_INTRO_INFO_START_Yd, \
            addr TXT_INTRO_TXTd, 7

    ; 绘制数字单元格
    mov eax, WIDGET_MAT_START_XY
    add eax, WIDGET_MARGIN_SMALL
    mov cellStartY, eax
    add eax, WIDGET_CELL_EDGE
    mov cellEndY, eax

    mov i, 0
    .while i < 4
        mov j, 0
        mov eax, WIDGET_MAT_START_XY
        add eax, WIDGET_MARGIN_SMALL
        mov cellStartX, eax
        add eax, WIDGET_CELL_EDGE
        mov cellEndX, eax
        
        .while j < 4
            pushad
            mov eax, i
            shl eax, 2
            add eax, j
            mov ecx, gameData.numMat[eax*4]
            mov currentNum, ecx

            ; 根据数字选择画刷
            .if currentNum == 0
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_0
            .elseif currentNum == 2
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_2
            .elseif currentNum == 4
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_4
            .elseif currentNum == 8
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_8
            .elseif currentNum == 16
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_16
            .elseif currentNum == 32
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_32
            .elseif currentNum == 64
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_64
            .elseif currentNum == 128
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_128
            .elseif currentNum == 256
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_256
            .elseif currentNum == 512
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_512
            .elseif currentNum == 1024
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_1024
            .else
                invoke SelectObject, _hDc, drawRes.BRUSH_CELL_2048
            .endif
            
            invoke RoundRect, _hDc, cellStartX, cellStartY, cellEndX, cellEndY, 3, 3

            .if currentNum != 0
                .if currentNum < 8
                    invoke SetTextColor, _hDc, COLOR_NUM_DARK
                .else
                    invoke SetTextColor, _hDc, COLOR_NUM_LIGHT
                .endif
                
                mov eax, cellStartX
                mov numStartX, eax
                mov eax, cellStartY
                mov numStartY, eax
                invoke SelectObject, _hDc, drawRes.FONT_CELL_NUM1000
                add numStartX, OFFSET_CELL_1000_X
                add numStartY, OFFSET_CELL_10000_Y
                
                .if currentNum == 2
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz2, 4
                .elseif currentNum == 4
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz4, 4
                .elseif currentNum == 8
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz8, 4
                .elseif currentNum == 16
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz16, 4
                .elseif currentNum == 32
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz32, 4
                .elseif currentNum == 64
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz64, 4
                .elseif currentNum == 128
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz128, 4
                .elseif currentNum == 256
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz256, 4
                .elseif currentNum == 512
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz512, 4
                .elseif currentNum == 1024
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz1024, 4
                .elseif currentNum == 2048
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz2048, 4
                .elseif currentNum == 4096
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz4096, 4
                .elseif currentNum == 8192
                    invoke TextOut, _hDc, numStartX, numStartY, addr sz8192, 4
                .else
                    invoke TextOut, _hDc, numStartX, numStartY, addr szOther, 4
                .endif
            .endif
            
            popad
            add cellStartX, WIDGET_CELL_EDGE + WIDGET_MARGIN_SMALL
            add cellEndX, WIDGET_CELL_EDGE + WIDGET_MARGIN_SMALL
            inc j
        .endw
        
        add cellStartY, WIDGET_CELL_EDGE + WIDGET_MARGIN_SMALL
        add cellEndY, WIDGET_CELL_EDGE + WIDGET_MARGIN_SMALL
        inc i
    .endw

    invoke BitBlt, hDc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, _hDc, 0, 0, SRCCOPY
    invoke DeleteObject, _cptBmp
    invoke DeleteDC, _hDc

    ; 游戏胜利弹窗
    .if gameData.infiniteMode == 0
        .if gameData.reached2048 == 1
            invoke MessageBox, windowHandles.hWinMain, offset MSG_WIN, \
                offset MSG_WIN_TITLE, MB_OK
            .if eax == IDOK
                invoke MessageBox, windowHandles.hWinMain, offset MSG_CONTINUE, \
                    offset MSG_WIN_TITLE, MB_YESNO
                .if eax == IDYES
                    mov gameData.infiniteMode, 1
                .elseif eax == IDNO
                    invoke DestroyWindow, windowHandles.hWinMain
                .endif
            .endif
        .endif
    .endif

    popad
    ret
updateGameWnd endp

END