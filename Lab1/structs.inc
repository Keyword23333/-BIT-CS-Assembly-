; structs.inc - 结构体定义文件
; 游戏数据结构
GAME_DATA STRUCT
    score dd 0
    bestScore dd 0
    reached2048 dd 0
    infiniteMode dd 0
    gameEnd dd 0
    movedUp dd 0
    movedDown dd 0
    movedLeft dd 0
    movedRight dd 0
    randomSeed dd 0
    numMat dd 16 dup(?)
    numMatCopy dd 16 dup(?)
    flag dd 4 dup(?)
GAME_DATA ENDS

; 输入状态结构
INPUT_STATE STRUCT
    isUpPressed db 0
    isDownPressed db 0
    isLeftPressed db 0
    isRightPressed db 0
INPUT_STATE ENDS

; 绘制资源结构
DRAW_RESOURCES STRUCT
    ; 字体句柄
    FONT_CELL_NUM dd ?
    FONT_TITLE dd ?
    FONT_SCORE_TXT dd ?
    FONT_SCORE_NUM dd ?
    FONT_RESTART dd ?
    FONT_INTRO_TITLE dd ?
    FONT_INTRO_TXT dd ?
    FONT_CELL_NUM100 dd ?
    FONT_CELL_NUM1000 dd ?
    FONT_CELL_NUM10000 dd ?
    
    ; 画刷句柄
    BRUSH_CELL_0 dd ?
    BRUSH_CELL_2 dd ?
    BRUSH_CELL_4 dd ?
    BRUSH_CELL_8 dd ?
    BRUSH_CELL_16 dd ?
    BRUSH_CELL_32 dd ?
    BRUSH_CELL_64 dd ?
    BRUSH_CELL_128 dd ?
    BRUSH_CELL_256 dd ?
    BRUSH_CELL_512 dd ?
    BRUSH_CELL_1024 dd ?
    BRUSH_CELL_2048 dd ?
DRAW_RESOURCES ENDS

; 窗口句柄结构
WINDOW_HANDLES STRUCT
    hInstance dd ?
    hWinMain dd ?
WINDOW_HANDLES ENDS