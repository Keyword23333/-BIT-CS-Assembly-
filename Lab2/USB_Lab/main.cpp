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
    // 1. ��������
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui USB Lab"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("USB �ӿ�ʵ�� (Modular)"), WS_OVERLAPPEDWINDOW, 100, 100, 1000, 700, NULL, NULL, wc.hInstance, NULL);

    // 2. ��ʼ�� D3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 3. ��ʼ�� GUI
    InitGui(hwnd);

    // 4. ��ʼ����ɨ��
    GetUserInfo();
    ScanUSBDevices();
    RefreshDrives();

    // 5. ��ѭ��
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // 6. ������Ⱦ����
        RenderGuiFrame(); // ���� ImGui ����

        // ִ�� DirectX ����
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // 7. ����
    ShutdownGui();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}