; game_logic.asm - 游戏逻辑模块
.386
.model flat, stdcall
option casemap:none

include windows.inc
include msvcrt.inc
includelib msvcrt.lib
include kernel32.inc
includelib kernel32.lib

include constants.inc
include structs.inc
include globals.inc

; 声明公共函数
PUBLIC initGameData, moveUp, moveDown, moveLeft, moveRight
PUBLIC canMove, updateScore, updateBestScore, randomLCG

.CODE

;==============================================================
;>> 在矩阵中生成随机数字
;==============================================================
randomLCG proc     
    local lcg_a, lcg_c, randNum, randPos

    pusha
    mov eax, gameData.randomSeed

    mov lcg_a, 0343fdh
    mov lcg_c, 269ec3h

lcg:
    mul lcg_a
    add eax, lcg_c
    xor edx, edx
    mov ebx, lcgMaxNum
    div ebx
    mov eax, edx
    mov randPos, eax

    mov eax, randPos
    .if gameData.numMat[eax * 4] != 0
        jmp lcg
    .endif

    mul lcg_a
    add eax, lcg_c
    xor edx, edx
    mov ebx, lcgMaxNum
    div ebx
    mov eax, edx
    xor edx, edx
    mov ebx, 2
    div ebx
    .if edx
        mov randNum, 2
    .else 
        mov randNum, 4
    .endif

    mov eax, randPos
    mov edx, randNum
    mov gameData.numMat[eax * 4], edx
    mov gameData.randomSeed, eax

    popa
    ret
randomLCG endp

;==============================================================
;>> 游戏初始化函数
;==============================================================
initGameData proc
    ; 初始化游戏数据
    mov gameData.gameEnd, 0
    mov gameData.reached2048, 0
    mov gameData.infiniteMode, 0
    mov gameData.score, 0
    ; mov gameData.bestScore, 0
    
    ; 清空数字矩阵
    mov esi, 0
    .while esi < 16
        mov gameData.numMat[esi * 4], 0
        inc esi
    .endw

    ; 初始化随机数种子
    invoke GetTickCount
    mov gameData.randomSeed, eax

    ; 生成两个初始数字
    invoke randomLCG
    invoke randomLCG
    
    ret
initGameData endp


;==============================================================
;>> 根据当前分数更新最高分
;==============================================================
updateBestScore proc
    pushad
    mov eax, gameData.score
    .if gameData.bestScore < eax
        mov gameData.bestScore, eax
    .endif
    popad
    ret
updateBestScore endp

;==============================================================
;>> 根据矩阵当前状态计算分数
;==============================================================
updateScore proc
    pushad
    mov ecx, 0
    .while ecx < 16
        mov esi, gameData.numMat[ecx*4]
        .if esi == 2048
            mov gameData.reached2048, 1
            .break
        .endif
        inc ecx
    .endw
    popad
    ret
updateScore endp

;==============================================================
;>> 数字向上移动
;==============================================================
moveUp proc
    local col, row, changed, r
    local currentNum, tmpNum, targetRow
    
    pushad
    mov changed, 0
    mov gameData.movedUp, 0
    mov col, 0
    
    .while col < 4
        xor eax, eax
        mov gameData.flag[0], eax
        mov gameData.flag[4], eax
        mov gameData.flag[8], eax
        mov gameData.flag[12], eax
        
        mov row, 1
        .while row < 4
            mov eax, row
            mov ebx, col
            shl eax, 2
            add eax, ebx
            mov edi, gameData.numMat[eax*4]
            mov currentNum, edi
            mov edi, row
            mov targetRow, edi
            dec edi
            mov r, edi
            
            .while r >= 0 && currentNum != 0
                mov eax, r
                mov ebx, col
                shl eax, 2
                add eax, ebx
                mov edi, gameData.numMat[eax*4]
                mov tmpNum, edi
                mov ecx, currentNum
                mov edi, r
                
                .if tmpNum == 0
                    mov eax, r
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    mov gameData.numMat[eax*4], ecx
                    
                    mov eax, targetRow
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov ecx, r
                    mov targetRow, ecx
                    mov changed, 1
                    
                .elseif tmpNum == ecx && gameData.flag[edi*4] == 0
                    mov eax, r
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    shl ecx, 1
                    mov gameData.numMat[eax*4], ecx
                    
                    add gameData.score, ecx
                    
                    mov eax, targetRow
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov edi, r
                    mov gameData.flag[edi*4], 1
                    mov changed, 1
                    .break
                .else
                    .break
                .endif
                
                .if r == 0
                    .break
                .endif
                dec r
            .endw
            inc row
        .endw
        inc col
    .endw
    
    .if changed == 1
        mov gameData.movedUp, 1
    .endif
    popad
    ret
moveUp endp

;==============================================================
;>> 数字向下移动
;==============================================================
moveDown proc
    local col:DWORD, row:DWORD, changed:DWORD, r:DWORD
    local currentNum:DWORD, tmpNum:DWORD, targetRow:DWORD
    
    pushad
    mov changed, 0
    mov gameData.movedDown, 0
    mov col, 0
    
    .while col < 4
        xor eax, eax
        mov gameData.flag[0], eax
        mov gameData.flag[4], eax
        mov gameData.flag[8], eax
        mov gameData.flag[12], eax
        
        mov row, 2
        .while row >= 0
            mov eax, row
            mov ebx, col
            shl eax, 2
            add eax, ebx
            mov edi, gameData.numMat[eax*4]
            mov currentNum, edi
            mov edi, row
            mov targetRow, edi
            inc edi
            mov r, edi
            
            .while r < 4 && currentNum != 0
                mov eax, r
                mov ebx, col
                shl eax, 2
                add eax, ebx
                mov edi, gameData.numMat[eax*4]
                mov tmpNum, edi
                mov ecx, currentNum
                mov edi, r
                
                .if tmpNum == 0
                    mov eax, r
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    mov gameData.numMat[eax*4], ecx
                    
                    mov eax, targetRow
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov ecx, r
                    mov targetRow, ecx
                    mov changed, 1
                    
                .elseif tmpNum == ecx && gameData.flag[edi*4] == 0
                    mov eax, r
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    shl ecx, 1
                    mov gameData.numMat[eax*4], ecx
                    
                    add gameData.score, ecx
                    
                    mov eax, targetRow
                    mov ebx, col
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov edi, r
                    mov gameData.flag[edi*4], 1
                    mov changed, 1
                    .break
                .else
                    .break
                .endif
                
                inc r
            .endw
            
            .if row == 0
                .break
            .endif
            dec row
        .endw
        inc col
    .endw
    
    .if changed == 1
        mov gameData.movedDown, 1
    .endif
    popad
    ret
moveDown endp

;==============================================================
;>> 数字向左移动
;==============================================================
moveLeft proc
    local col:DWORD, row:DWORD, changed:DWORD, co:DWORD
    local currentNum:DWORD, tmpNum:DWORD, targetCol:DWORD
    
    pushad
    mov changed, 0
    mov gameData.movedLeft, 0
    mov row, 0
    
    .while row < 4
        xor eax, eax
        mov gameData.flag[0], eax
        mov gameData.flag[4], eax
        mov gameData.flag[8], eax
        mov gameData.flag[12], eax
        
        mov col, 1
        .while col < 4
            mov eax, row
            mov ebx, col
            shl eax, 2
            add eax, ebx
            mov edi, gameData.numMat[eax*4]
            mov currentNum, edi
            mov edi, col
            mov targetCol, edi
            dec edi
            mov co, edi
            
            .while co >= 0 && currentNum != 0
                mov eax, row
                mov ebx, co
                shl eax, 2
                add eax, ebx
                mov edi, gameData.numMat[eax*4]
                mov tmpNum, edi
                mov ecx, currentNum
                mov edi, co
                
                .if tmpNum == 0
                    mov eax, row
                    mov ebx, co
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    mov gameData.numMat[eax*4], ecx
                    
                    mov eax, row
                    mov ebx, targetCol
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov ecx, co
                    mov targetCol, ecx
                    mov changed, 1
                    
                .elseif tmpNum == ecx && gameData.flag[edi*4] == 0
                    mov eax, row
                    mov ebx, co
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    shl ecx, 1
                    mov gameData.numMat[eax*4], ecx
                    
                    add gameData.score, ecx
                    
                    mov eax, row
                    mov ebx, targetCol
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov edi, co
                    mov gameData.flag[edi*4], 1
                    mov changed, 1
                    .break
                .else
                    .break
                .endif
                
                .if co == 0
                    .break
                .endif
                dec co
            .endw
            inc col
        .endw
        inc row
    .endw
    
    .if changed == 1
        mov gameData.movedLeft, 1
    .endif
    popad
    ret
moveLeft endp

;==============================================================
;>> 数字向右移动
;==============================================================
moveRight proc
    local col:DWORD, row:DWORD, changed:DWORD, co:DWORD
    local currentNum:DWORD, tmpNum:DWORD, targetCol:DWORD
    
    pushad
    mov changed, 0
    mov gameData.movedRight, 0
    mov row, 0
    
    .while row < 4
        xor eax, eax
        mov gameData.flag[0], eax
        mov gameData.flag[4], eax
        mov gameData.flag[8], eax
        mov gameData.flag[12], eax
        
        mov col, 2
        .while col >= 0
            mov eax, row
            mov ebx, col
            shl eax, 2
            add eax, ebx
            mov edi, gameData.numMat[eax*4]
            mov currentNum, edi
            mov edi, col
            mov targetCol, edi
            inc edi
            mov co, edi
            
            .while co < 4 && currentNum != 0
                mov eax, row
                mov ebx, co
                shl eax, 2
                add eax, ebx
                mov edi, gameData.numMat[eax*4]
                mov tmpNum, edi
                mov ecx, currentNum
                mov edi, co
                
                .if tmpNum == 0
                    mov eax, row
                    mov ebx, co
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    mov gameData.numMat[eax*4], ecx
                    
                    mov eax, row
                    mov ebx, targetCol
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov ecx, co
                    mov targetCol, ecx
                    mov changed, 1
                    
                .elseif tmpNum == ecx && gameData.flag[edi*4] == 0
                    mov eax, row
                    mov ebx, co
                    shl eax, 2
                    add eax, ebx
                    mov ecx, currentNum
                    shl ecx, 1
                    mov gameData.numMat[eax*4], ecx
                    
                    add gameData.score, ecx
                    
                    mov eax, row
                    mov ebx, targetCol
                    shl eax, 2
                    add eax, ebx
                    mov ecx, 0
                    mov gameData.numMat[eax*4], ecx
                    
                    mov edi, co
                    mov gameData.flag[edi*4], 1
                    mov changed, 1
                    .break
                .else
                    .break
                .endif
                
                inc co
            .endw
            
            .if col == 0
                .break
            .endif
            dec col
        .endw
        inc row
    .endw
    
    .if changed == 1
        mov gameData.movedRight, 1
    .endif
    popad
    ret
moveRight endp

;==============================================================
;>> 判断是否可以继续移动
;==============================================================
canMove proc
    local oldScore:dword
    
    pushad
    mov eax, gameData.score
    mov oldScore, eax
    
    ; 保存当前矩阵状态
    mov ecx, 0
    .while ecx < 16
        mov esi, gameData.numMat[ecx*4]
        mov gameData.numMatCopy[ecx*4], esi
        inc ecx
    .endw

    ; 测试向上移动
    invoke moveUp
    ; 恢复矩阵状态
    mov ecx, 0
    .while ecx < 16
        mov esi, gameData.numMatCopy[ecx*4]
        mov gameData.numMat[ecx*4], esi
        inc ecx
    .endw

    ; 测试向下移动
    invoke moveDown
    ; 恢复矩阵状态
    mov ecx, 0
    .while ecx < 16
        mov esi, gameData.numMatCopy[ecx*4]
        mov gameData.numMat[ecx*4], esi
        inc ecx
    .endw

    ; 测试向左移动
    invoke moveLeft
    ; 恢复矩阵状态
    mov ecx, 0
    .while ecx < 16
        mov esi, gameData.numMatCopy[ecx*4]
        mov gameData.numMat[ecx*4], esi
        inc ecx
    .endw

    ; 测试向右移动
    invoke moveRight
    ; 恢复矩阵状态
    mov ecx, 0
    .while ecx < 16
        mov esi, gameData.numMatCopy[ecx*4]
        mov gameData.numMat[ecx*4], esi
        inc ecx
    .endw

    ; 判断是否有移动可能
    .if gameData.movedUp == 1 || gameData.movedDown == 1 || gameData.movedLeft == 1 || gameData.movedRight == 1
        mov gameData.gameEnd, 0
    .else
        mov gameData.gameEnd, 1
    .endif

    ; 恢复分数
    mov eax, oldScore
    mov gameData.score, eax
    
    popad
    ret
canMove endp

END