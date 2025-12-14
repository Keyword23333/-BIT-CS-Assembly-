#pragma execution_character_set("utf-8")
#include "GuiLayer.h"
#include "Common.h"
#include "USBCore.h"
#include "RenderSystem.h" // 需要 Direct3D 设备声明
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
    // 每帧更新的逻辑
    UpdateRealTimeMonitor();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("MainPanel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // --- 标题 ---
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "USB BUS INTERFACE LAB");
    ImGui::SameLine();
    ImGui::TextDisabled("| %s", g_userBuffer);
    ImGui::Separator();

    // =======================================================
    // [左侧面板]
    // =======================================================
    ImGui::Columns(2, "MainColumns");
    ImGui::SetColumnWidth(0, 320); // ���̶�����

    // ��ħ������ 1������¼������ʼ�� Y ��߶�
    float start_y = ImGui::GetCursorPosY();

    // =======================================================
    // [功能按钮] 按钮 + 日志
    // =======================================================

    // 1. 可移动存储设备
    ImGui::Text("可移动存储设备");
    if (ImGui::Button("刷新列表", ImVec2(120, 26))) {
        // 同时刷新 USB 设备信息和逻辑驱动器列表
        ScanUSBDevices();
        RefreshDrives();
        AppLog("已刷新设备与驱动器列表");
    }

    const char* preview = (g_selectedDriveIdx >= 0 && g_selectedDriveIdx < g_logicalDrives.size())
        ? g_logicalDrives[g_selectedDriveIdx].c_str() : "未选择";
    if (ImGui::BeginCombo("目标盘符", preview)) {
        for (int i = 0; i < g_logicalDrives.size(); i++) {
            bool is_selected = (g_selectedDriveIdx == i);

            // ��ʾ�̷� + ���� (���� F:\ [SanDisk])
            std::string label = GetDriveLabel(g_logicalDrives[i]);
            std::string itemText = g_logicalDrives[i] + " [" + label + "]";

            if (ImGui::Selectable(itemText.c_str(), is_selected))
                g_selectedDriveIdx = i;
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Dummy(ImVec2(0, 10));

    // 2. 文件操作
    ImGui::Text("文件操作");
    if (ImGui::Button("写测试文件", ImVec2(150, 30))) WriteTest(); ImGui::SameLine();
    if (ImGui::Button("打开U盘目录", ImVec2(150, 30))) {
        if (g_selectedDriveIdx >= 0 && g_selectedDriveIdx < g_logicalDrives.size()) {
            ListFilesInDrive(g_logicalDrives[g_selectedDriveIdx]);
            g_showDriveFiles = true;
        }
    }
    if (ImGui::Button("复制当前可执行文件", ImVec2(150, 30))) CopyFileToUSB(); ImGui::SameLine();
    if (ImGui::Button("删除测试文件", ImVec2(150, 30))) DeleteFileFromUSB();
    ImGui::Dummy(ImVec2(0, 10));

    // 3. 性能监控
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "性能监控");
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

    // 4. 系统日志 (显示最近的运行信息)
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Text("系统日志");
    ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
    for (const auto& log : g_logs) ImGui::TextWrapped("%s", log.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();


    // =======================================================
    // [�л����Ҳ���]
    // =======================================================
    ImGui::NextColumn();

    // ��ħ������ 2����ǿ�ưѹ�����ص�������
    ImGui::SetCursorPosY(start_y);

    // =======================================================
    // [�Ҳ�������]��ͼ�� + �б�
    // =======================================================

    // 1. 绘制实时速度曲线 (带 Target 标签)
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImGui::BeginChild("GraphRegion", ImVec2(0, 240), true); // �̶��߶�

    // --- 标题 ---
    ImGui::Text("实时速度曲线");

    // �Ҷ�����ʾ״̬
    ImGui::SameLine();
    float avail_w = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(avail_w - 120);
    ImGui::TextDisabled(g_isMonitorRunning ? "[运行中...]" : "[停止]");

    // ==============================================
    // ���ѻָ�������Ż�ɫ������ʾ Target Ŀ��
    // ==============================================
    std::string targetLabel = "未选择目标 (No Target)";
    if (g_selectedDriveIdx >= 0 && g_selectedDriveIdx < g_logicalDrives.size()) {
        // ���� GetDriveLabel ��ȡ���� (���� SanDisk)
        std::string label = GetDriveLabel(g_logicalDrives[g_selectedDriveIdx]);
        targetLabel = g_logicalDrives[g_selectedDriveIdx] + " (" + label + ")";
    }

    ImGui::SetWindowFontScale(1.3f); // �Ŵ� 1.3 ��
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Target: %s", targetLabel.c_str());
    ImGui::SetWindowFontScale(1.0f); // �ָ�������С
    // ==============================================

    // �������ֵ
    float max_val = 1.0f;
    for (int i = 0; i < 90; i++) if (g_speedHistory[i] > max_val) max_val = g_speedHistory[i];
    max_val *= 1.2f;

    // ��ʾ��ǰ�ٶ���ֵ
    float currentSpeed = g_historyOffset > 0 ? g_speedHistory[(g_historyOffset - 1 + 90) % 90] : 0.0f;
    char overlay[64];
    sprintf_s(overlay, "Current: %.1f MB/s", currentSpeed);

    // ����ͼ��
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
    ImGui::PlotLines("##SpeedGraphBig", g_speedHistory, 90, g_historyOffset, overlay, 0.0f, max_val, ImVec2(-1, -1));
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 5));

    // 2. 右侧设备列表
    ImGui::Text("USB 连接设备 (Live)");
    ImGui::SameLine();
    if (ImGui::Button("刷新设备")) {
        // 唯一的“刷新设备”按钮，用于刷新设备信息与驱动器列表
        ScanUSBDevices();
        RefreshDrives();
        AppLog("已刷新设备与驱动器列表");
    }
    
        // 如果用户打开了U盘目录，则在右下区域显示文件列表（支持递归进入）
        if (g_showDriveFiles) {
            ImGui::Separator();
            // 面包屑路径显示
            ImGui::Text("路径: %s", g_currentPath.c_str());

            ImGui::BeginChild("DriveFiles", ImVec2(0, -30), true);
            // 使用带双击支持的 selectable，双击进入目录
            for (int i = 0; i < g_currentDriveFiles.size(); ++i) {
                const std::string& item = g_currentDriveFiles[i];
                bool isDir = (!item.empty() && item.back() == '\\');
                std::string display = isDir ? ("[D] " + item) : ("    " + item);
                bool selected = (g_selectedFileIndex == i);
                ImGui::PushID(i);
                if (ImGui::Selectable(display.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    g_selectedFileIndex = i;
                }
                // 双击进入目录
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (isDir) {
                        EnterDirectory(item);
                        g_selectedFileIndex = -1;
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndChild();

            // 底部小按钮：返回、删除、拷贝到电脑、关闭
            ImGui::BeginGroup();
            if (ImGui::Button("<- 返回", ImVec2(90, 24))) {
                GoUpDirectory();
            }
            ImGui::SameLine();
            if (ImGui::Button("删除", ImVec2(90, 24))) {
                if (g_selectedFileIndex >= 0 && g_selectedFileIndex < g_currentDriveFiles.size()) {
                    std::string sel = g_currentDriveFiles[g_selectedFileIndex];
                    std::string full = g_currentPath + sel;
                    if (DeleteFileOnDrive(full)) AppLog("删除成功: " + full);
                    else AppLog("删除失败: " + full);
                    ListFilesInDrive(g_currentPath);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("拷贝到电脑", ImVec2(110, 24))) {
                if (g_selectedFileIndex >= 0 && g_selectedFileIndex < g_currentDriveFiles.size()) {
                    std::string sel = g_currentDriveFiles[g_selectedFileIndex];
                    CopyFileFromDriveToPC(sel);
                    ListFilesInDrive(g_currentPath);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("从电脑拷贝到U盘", ImVec2(150, 24))) {
                if (g_selectedDriveIdx >= 0) {
                    CopyLocalFileToDriveWithDialog(g_logicalDrives[g_selectedDriveIdx]);
                    ListFilesInDrive(g_currentPath.empty() ? g_logicalDrives[g_selectedDriveIdx] : g_currentPath);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("关闭目录", ImVec2(90, 24))) {
                g_showDriveFiles = false;
                g_selectedFileIndex = -1;
            }
            ImGui::EndGroup();
        }

    static ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

    // ע�⣺�����õ��� 4 �� (������֮ǰ�Ĵ���)
    // �����֮ǰ�Ѿ����˵�5��(���۴���)�����԰� 4 �ĳ� 5������������м���
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