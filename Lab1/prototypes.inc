; prototypes.inc - 所有函数原型声明
; .386

; 资源管理模块
initFonts PROTO
cleanupResources PROTO

; 游戏逻辑模块
initGameData PROTO
moveUp PROTO
moveDown PROTO
moveLeft PROTO
moveRight PROTO
canMove PROTO
updateScore PROTO
updateBestScore PROTO
randomLCG PROTO

; 界面绘制模块
updateGameWnd PROTO :DWORD, :DWORD

; 输入处理模块
handleKeyDown PROTO :DWORD, :DWORD
handleKeyUp PROTO :DWORD
handleMouseClick PROTO :DWORD, :DWORD