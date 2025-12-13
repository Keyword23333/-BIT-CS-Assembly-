#pragma once
#include <windows.h>

// 初始化 GUI (加载字体、风格)
void InitGui(HWND hwnd);

// 每帧渲染 (所有的 ImGui::Text, Button 都在这里)
void RenderGuiFrame();

// 清理 GUI 资源
void ShutdownGui();