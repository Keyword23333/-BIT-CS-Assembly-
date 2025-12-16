; resource_manager.asm - 资源管理模块
.386
.model flat, stdcall
option casemap:none

include windows.inc
includelib gdi32.lib
include gdi32.inc

include constants.inc
include structs.inc
include globals.inc

; 公开函数
PUBLIC initFonts, cleanupResources

.CODE

;==============================================================
;>> 初始化字体和画刷
;==============================================================
initFonts proc
    push eax
    invoke CreateFont, 90, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_CELL_NUM, eax
    
    invoke CreateFont, 70, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_TITLE, eax
    
    invoke CreateFont, 35, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_SCORE_TXT, eax
    
    invoke CreateFont, 30, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_SCORE_NUM, eax
    
    invoke CreateFont, 40, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_RESTART, eax
    
    invoke CreateFont, 50, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_INTRO_TITLE, eax
    
    invoke CreateFont, 30, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_INTRO_TXT, eax

    ; 数字Block的字体
    invoke CreateFont, 72, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_CELL_NUM100, eax
    
    invoke CreateFont, 59, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_CELL_NUM1000, eax
    
    invoke CreateFont, 46, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, \
            DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, \
            DEFAULT_QUALITY, DEFAULT_PITCH or FF_DONTCARE, addr szFontName
    mov drawRes.FONT_CELL_NUM10000, eax

    ; 数字Block的背景画刷
    invoke CreateSolidBrush, COLOR_NUM0_BG
    mov drawRes.BRUSH_CELL_0, eax
    
    invoke CreateSolidBrush, COLOR_NUM2_BG
    mov drawRes.BRUSH_CELL_2, eax
    
    invoke CreateSolidBrush, COLOR_NUM4_BG
    mov drawRes.BRUSH_CELL_4, eax
    
    invoke CreateSolidBrush, COLOR_NUM8_BG
    mov drawRes.BRUSH_CELL_8, eax
    
    invoke CreateSolidBrush, COLOR_NUM16_BG
    mov drawRes.BRUSH_CELL_16, eax
    
    invoke CreateSolidBrush, COLOR_NUM32_BG
    mov drawRes.BRUSH_CELL_32, eax
    
    invoke CreateSolidBrush, COLOR_NUM64_BG
    mov drawRes.BRUSH_CELL_64, eax
    
    invoke CreateSolidBrush, COLOR_NUM128_BG
    mov drawRes.BRUSH_CELL_128, eax
    
    invoke CreateSolidBrush, COLOR_NUM256_BG
    mov drawRes.BRUSH_CELL_256, eax
    
    invoke CreateSolidBrush, COLOR_NUM512_BG
    mov drawRes.BRUSH_CELL_512, eax
    
    invoke CreateSolidBrush, COLOR_NUM1024_BG
    mov drawRes.BRUSH_CELL_1024, eax
    
    invoke CreateSolidBrush, COLOR_NUM2048_BG
    mov drawRes.BRUSH_CELL_2048, eax
    
    pop eax
    ret
initFonts endp

;==============================================================
;>> 清理资源
;==============================================================
cleanupResources proc
    ; 删除所有字体对象
    .if drawRes.FONT_TITLE != 0
        invoke DeleteObject, drawRes.FONT_TITLE
    .endif
    
    .if drawRes.FONT_SCORE_TXT != 0
        invoke DeleteObject, drawRes.FONT_SCORE_TXT
    .endif
    
    .if drawRes.FONT_SCORE_NUM != 0
        invoke DeleteObject, drawRes.FONT_SCORE_NUM
    .endif
    
    .if drawRes.FONT_RESTART != 0
        invoke DeleteObject, drawRes.FONT_RESTART
    .endif
    
    .if drawRes.FONT_INTRO_TITLE != 0
        invoke DeleteObject, drawRes.FONT_INTRO_TITLE
    .endif
    
    .if drawRes.FONT_INTRO_TXT != 0
        invoke DeleteObject, drawRes.FONT_INTRO_TXT
    .endif
    
    .if drawRes.FONT_CELL_NUM != 0
        invoke DeleteObject, drawRes.FONT_CELL_NUM
    .endif
    
    .if drawRes.FONT_CELL_NUM100 != 0
        invoke DeleteObject, drawRes.FONT_CELL_NUM100
    .endif
    
    .if drawRes.FONT_CELL_NUM1000 != 0
        invoke DeleteObject, drawRes.FONT_CELL_NUM1000
    .endif
    
    .if drawRes.FONT_CELL_NUM10000 != 0
        invoke DeleteObject, drawRes.FONT_CELL_NUM10000
    .endif

    ; 删除所有画刷对象
    .if drawRes.BRUSH_CELL_0 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_0
    .endif
    
    .if drawRes.BRUSH_CELL_2 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_2
    .endif
    
    .if drawRes.BRUSH_CELL_4 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_4
    .endif
    
    .if drawRes.BRUSH_CELL_8 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_8
    .endif
    
    .if drawRes.BRUSH_CELL_16 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_16
    .endif
    
    .if drawRes.BRUSH_CELL_32 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_32
    .endif
    
    .if drawRes.BRUSH_CELL_64 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_64
    .endif
    
    .if drawRes.BRUSH_CELL_128 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_128
    .endif
    
    .if drawRes.BRUSH_CELL_256 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_256
    .endif
    
    .if drawRes.BRUSH_CELL_512 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_512
    .endif
    
    .if drawRes.BRUSH_CELL_1024 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_1024
    .endif
    
    .if drawRes.BRUSH_CELL_2048 != 0
        invoke DeleteObject, drawRes.BRUSH_CELL_2048
    .endif
    
    ret
cleanupResources endp

END
