#pragma execution_character_set("utf-8")
#include "GuiLayer.h"
#include "Common.h"
#include "USBCore.h"
#include "RenderSystem.h" // 需要 D3D 设备指针
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

void InitGui(HWND hwnd) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    // 加载字体
    ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
}

void ShutdownGui() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void RenderGuiFrame() {
    // 每一帧调用监控逻辑
    UpdateRealTimeMonitor();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("MainPanel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // --- 标题栏 ---
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "USB BUS INTERFACE LAB");
    ImGui::SameLine();
    ImGui::TextDisabled("| %s", g_userBuffer);
    ImGui::Separator();

    // =======================================================
    // [布局开始]
    // =======================================================
    ImGui::Columns(2, "MainColumns");
    ImGui::SetColumnWidth(0, 320); // 左侧固定宽度

    // 【魔法代码 1】：记录分栏起始的 Y 轴高度
    float start_y = ImGui::GetCursorPosY();

    // =======================================================
    // [左侧列内容]：按钮 + 日志
    // =======================================================

    // 1. 逻辑存储设备控制
    ImGui::Text("逻辑存储设备控制");
    if (ImGui::Button("刷新磁盘", ImVec2(120, 30))) RefreshDrives();

    const char* preview = (g_selectedDriveIdx >= 0 && g_selectedDriveIdx < g_logicalDrives.size())
        ? g_logicalDrives[g_selectedDriveIdx].c_str() : "未选择";
    if (ImGui::BeginCombo("目标盘符", preview)) {
        for (int i = 0; i < g_logicalDrives.size(); i++) {
            bool is_selected = (g_selectedDriveIdx == i);

            // 显示盘符 + 卷标 (例如 F:\ [SanDisk])
            std::string label = GetDriveLabel(g_logicalDrives[i]);
            std::string itemText = g_logicalDrives[i] + " [" + label + "]";

            if (ImGui::Selectable(itemText.c_str(), is_selected))
                g_selectedDriveIdx = i;
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Dummy(ImVec2(0, 10));

    // 2. 文件操作测试
    ImGui::Text("文件操作测试");
    if (ImGui::Button("写入测试文件", ImVec2(280, 30))) WriteTest();
    if (ImGui::Button("模拟拷贝文件", ImVec2(280, 30))) CopyFileToUSB();
    if (ImGui::Button("删除测试文件", ImVec2(280, 30))) DeleteFileFromUSB();
    ImGui::Dummy(ImVec2(0, 10));

    // 3. 性能监控开关
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "性能监控开关");
    if (g_isMonitorRunning) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("停止实时监测 (STOP)", ImVec2(280, 50))) g_isMonitorRunning = false;
        ImGui::PopStyleColor();
    }
    else {
        if (ImGui::Button("开始实时监测 (START)", ImVec2(280, 50))) {
            g_isMonitorRunning = true;
            g_lastMonitorTick = 0;
            memset(g_speedHistory, 0, sizeof(g_speedHistory));
            g_historyOffset = 0;
        }
    }
    ImGui::Separator();

    // 4. 系统日志 (填满左侧剩余空间)
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Text("系统日志");
    ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
    for (const auto& log : g_logs) ImGui::TextWrapped("%s", log.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();


    // =======================================================
    // [切换到右侧列]
    // =======================================================
    ImGui::NextColumn();

    // 【魔法代码 2】：强制把光标拉回到顶部！
    ImGui::SetCursorPosY(start_y);

    // =======================================================
    // [右侧列内容]：图表 + 列表
    // =======================================================

    // 1. 顶部：实时折线图 (已恢复 Target 标签)
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImGui::BeginChild("GraphRegion", ImVec2(0, 240), true); // 固定高度

    // --- 标题行 ---
    ImGui::Text("实时传输速率可视化");

    // 右对齐显示状态
    ImGui::SameLine();
    float avail_w = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(avail_w - 120);
    ImGui::TextDisabled(g_isMonitorRunning ? "[监控中...]" : "[待机]");

    // ==============================================
    // 【已恢复】：大号黄色字体显示 Target 目标
    // ==============================================
    std::string targetLabel = "未选择目标 (No Target)";
    if (g_selectedDriveIdx >= 0 && g_selectedDriveIdx < g_logicalDrives.size()) {
        // 调用 GetDriveLabel 获取卷标 (例如 SanDisk)
        std::string label = GetDriveLabel(g_logicalDrives[g_selectedDriveIdx]);
        targetLabel = g_logicalDrives[g_selectedDriveIdx] + " (" + label + ")";
    }

    ImGui::SetWindowFontScale(1.3f); // 放大 1.3 倍
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Target: %s", targetLabel.c_str());
    ImGui::SetWindowFontScale(1.0f); // 恢复正常大小
    // ==============================================

    // 计算最大值
    float max_val = 1.0f;
    for (int i = 0; i < 90; i++) if (g_speedHistory[i] > max_val) max_val = g_speedHistory[i];
    max_val *= 1.2f;

    // 显示当前速度数值
    float currentSpeed = g_historyOffset > 0 ? g_speedHistory[(g_historyOffset - 1 + 90) % 90] : 0.0f;
    char overlay[64];
    sprintf_s(overlay, "Current: %.1f MB/s", currentSpeed);

    // 绘制图表
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
    ImGui::PlotLines("##SpeedGraphBig", g_speedHistory, 90, g_historyOffset, overlay, 0.0f, max_val, ImVec2(-1, -1));
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 5));

    // 2. 底部：设备列表
    ImGui::Text("USB 总线设备树 (Live)");
    ImGui::SameLine();
    if (ImGui::Button("刷新列表")) ScanUSBDevices();

    static ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

    // 注意：这里用的是 4 列 (兼容你之前的代码)
    // 如果你之前已经加了第5列(理论带宽)，可以把 4 改成 5，并把下面的列加上
    if (ImGui::BeginTable("DevTable", 4, table_flags, ImVec2(0, -1))) {
        ImGui::TableSetupColumn("厂商", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("协议", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("设备名称");
        ImGui::TableSetupColumn("硬件 ID");
        ImGui::TableHeadersRow();

        for (int i = 0; i < g_usbDevices.size(); i++) {
            const auto& dev = g_usbDevices[i];
            ImGui::PushID(i);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(dev.vendor.c_str(), g_selectedDeviceIndex == i, ImGuiSelectableFlags_SpanAllColumns))
                g_selectedDeviceIndex = i;

            ImGui::TableSetColumnIndex(1);
            if (dev.protocol.find("3.0") != std::string::npos)
                ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "%s", dev.protocol.c_str());
            else
                ImGui::Text("%s", dev.protocol.c_str());

            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", dev.name.c_str());
            ImGui::TableSetColumnIndex(3); ImGui::TextDisabled("%s", dev.hwid.c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
    ImGui::Render();
}