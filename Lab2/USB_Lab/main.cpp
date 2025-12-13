#pragma execution_character_set("utf-8")
#include <windows.h>
#include <tchar.h>
#include "Common.h"
#include "USBCore.h"
#include "RenderSystem.h"
#include "GuiLayer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"

int main(int, char**)
{
    // 1. 创建窗口
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui USB Lab"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("USB 接口实验 (Modular)"), WS_OVERLAPPEDWINDOW, 100, 100, 1000, 700, NULL, NULL, wc.hInstance, NULL);

    // 2. 初始化 D3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 3. 初始化 GUI
    InitGui(hwnd);

    // 4. 初始数据扫描
    GetUserInfo();
    ScanUSBDevices();
    RefreshDrives();

    // 5. 主循环
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // 6. 核心渲染流程
        RenderGuiFrame(); // 生成 ImGui 数据

        // 执行 DirectX 绘制
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // 7. 清理
    ShutdownGui();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}