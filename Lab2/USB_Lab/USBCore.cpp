#pragma execution_character_set("utf-8") // 必须加，防止中文乱码
#include "USBCore.h"
#include "Common.h"
#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <devguid.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "setupapi.lib")

// ==========================================
// 全局变量定义 (在这里分配内存)
// ==========================================
std::vector<USBDevice> g_usbDevices;                                      // 存储当前检测到的 USB 设备列表
std::vector<std::string> g_logicalDrives;                                 // 存储系统的逻辑驱动器列表（如 F:\）
std::vector<std::string> g_logs;                                          // 应用程序日志，用于记录事件或调试信息
int g_selectedDriveIdx = -1;                                              // 当前选中的驱动器索引（用于 UI 或内部逻辑）
int g_selectedDeviceIndex = -1;                                           // 当前选中的 USB 设备索引
char g_userBuffer[128] = "";                                              // 用户输入的字符缓冲区（大小为 128 字节）
SpeedResult g_lastSpeedTest = { false, 0.0, 0.0, "等待测试..." };         // 保存上一次磁盘测速结果的结构体

bool g_isMonitorRunning = false;                                          // 开关
float g_speedHistory[90] = { 0.0f };                                      // 初始化为0
int g_historyOffset = 0;                                                  // 图表滚动偏移量
std::string g_monitorDevName = "Unknown";                                 // 当前正在测的设备名
DWORD g_lastMonitorTick = 0;                                              // 上一次测速的时间戳

// ==========================================
// 工具函数实现
// ==========================================
std::string GuessUSBProtocol(const std::string& hwid, const std::string& name) {
    // 这是一个启发式判断，准确获取需要复杂的 IOCTL 查询
    if (hwid.find("ROOT_HUB30") != std::string::npos) return "USB 3.0 (Root)";
    if (hwid.find("USB 3.0") != std::string::npos) return "USB 3.0";
    if (name.find("USB 3.0") != std::string::npos) return "USB 3.0";
    if (name.find("SuperSpeed") != std::string::npos) return "USB 3.x";
    if (name.find("Composite") != std::string::npos) return "Composite";
    // 大部分默认是 2.0
    return "USB 2.0 / 1.1";
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void AppLog(const std::string& msg) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    sprintf_s(buf, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
    g_logs.push_back(std::string(buf) + msg);
    if (g_logs.size() > 100) g_logs.erase(g_logs.begin());
}

// ==========================================
// 业务逻辑实现
// ==========================================
void GetUserInfo() {
    wchar_t username[256];
    DWORD len = 256;
    if (GetUserNameW(username, &len)) {
        std::string u = WStringToString(username);
        sprintf_s(g_userBuffer, "当前用户: %s", u.c_str());
    }
}

void ScanUSBDevices() {
    g_usbDevices.clear();
    g_selectedDeviceIndex = -1;
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA DeviceInfoData;
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
        DWORD DataT;
        wchar_t buffer[1024];
        DWORD buffersize = 0;

        if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_FRIENDLYNAME, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize))
            SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize);
        std::wstring name = buffer;

        SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize);
        std::wstring hwid = buffer;

        SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_MFG, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize);
        std::wstring mfg = buffer;

        USBDevice dev;
        dev.name = WStringToString(name);
        dev.hwid = WStringToString(hwid);
        dev.vendor = WStringToString(mfg);

        bool is30 = false;
        // 简单的关键词匹配
        if (dev.hwid.find("ROOT_HUB30") != std::string::npos) is30 = true;
        if (dev.hwid.find("USB 3.0") != std::string::npos) is30 = true;
        if (dev.name.find("USB 3.0") != std::string::npos) is30 = true;
        if (dev.name.find("SuperSpeed") != std::string::npos) is30 = true;

        if (is30) dev.protocol = "USB 3.0";
        else dev.protocol = "USB 2.0 / 1.1";

        g_usbDevices.push_back(dev);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    AppLog("USB 设备列表已刷新。");
}

void RefreshDrives() {
    g_logicalDrives.clear();
    g_selectedDriveIdx = -1;
    DWORD drives = GetLogicalDrives();
    wchar_t driveName[] = L"A:\\";

    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            driveName[0] = L'A' + i;
            if (GetDriveTypeW(driveName) == DRIVE_REMOVABLE) {
                g_logicalDrives.push_back(WStringToString(driveName));
            }
        }
    }
    if (!g_logicalDrives.empty()) g_selectedDriveIdx = 0;

    if (g_logicalDrives.empty()) AppLog("未检测到可移动磁盘。");
    else AppLog("检测到 U盘挂载。");
}

void WriteTest() {
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) return;
    std::string path = g_logicalDrives[g_selectedDriveIdx] + "imgui_test.txt";
    std::ofstream out(path);
    if (out.is_open()) {
        out << "ImGui Interface Test.\n" << g_userBuffer;
        out.close();
        AppLog("写入文件成功: " + path);
    }
    else {
        AppLog("写入失败！");
    }
}

void RunDiskBenchmark() {
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) {
        g_lastSpeedTest.message = "错误: 未选择 U 盘";
        return;
    }

    std::string drive = g_logicalDrives[g_selectedDriveIdx];
    std::string testFile = drive + "speed_test.tmp";
    const size_t DATA_SIZE = 10 * 1024 * 1024; // 测试 10MB 数据
    char* buffer = new char[DATA_SIZE];

    // 填充随机数据防止被压缩
    for (size_t i = 0; i < DATA_SIZE; i++) buffer[i] = (char)(i % 256);

    AppLog("开始测速 (数据量: 10MB)...");
    g_lastSpeedTest.message = "正在写入...";

    // 1. 写入测试
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);

    QueryPerformanceCounter(&start);
    std::ofstream out(testFile, std::ios::binary);
    out.write(buffer, DATA_SIZE);
    out.close(); // 关闭以确保 flush
    QueryPerformanceCounter(&end);

    double timeWrite = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    double speedWrite = (DATA_SIZE / 1024.0 / 1024.0) / timeWrite;

    // 2. 读取测试
    g_lastSpeedTest.message = "正在读取...";
    QueryPerformanceCounter(&start);
    std::ifstream in(testFile, std::ios::binary);
    in.read(buffer, DATA_SIZE);
    in.close();
    QueryPerformanceCounter(&end);

    double timeRead = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    double speedRead = (DATA_SIZE / 1024.0 / 1024.0) / timeRead;

    // 3. 清理与结果
    DeleteFileA(testFile.c_str());
    delete[] buffer;

    g_lastSpeedTest.valid = true;
    g_lastSpeedTest.writeSpeedMBps = speedWrite;
    g_lastSpeedTest.readSpeedMBps = speedRead;

    char msg[128];
    sprintf_s(msg, "写入: %.1f MB/s | 读取: %.1f MB/s", speedWrite, speedRead);
    g_lastSpeedTest.message = msg;
    AppLog("测速完成: " + std::string(msg));
}

void UpdateRealTimeMonitor() {
    // 1. 如果没开开关，或者没有选设备，直接退出
    if (!g_isMonitorRunning) return;
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) {
        g_isMonitorRunning = false; // 强制停止
        AppLog("监控停止：设备无效");
        return;
    }

    // 2. 检查时间间隔 (每 1000ms 执行一次)
    DWORD currentTick = GetTickCount();
    if (currentTick - g_lastMonitorTick < 1000) {
        return; // 时间没到，不执行
    }
    g_lastMonitorTick = currentTick; // 更新时间戳

    // 3. 执行一次微型测速 (只测写入，为了快速响应)
    // 为什么只测 512KB？因为如果测 10MB，USB 2.0需要写2秒，界面会卡死。
    std::string drive = g_logicalDrives[g_selectedDriveIdx];
    std::string testFile = drive + "monitor.tmp";
    const size_t CHUNK_SIZE = 512 * 1024; // 512KB
    char* buffer = new char[CHUNK_SIZE];

    // 更新设备名称用于显示
    g_monitorDevName = drive + " (实时)";

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    // 写文件
    std::ofstream out(testFile, std::ios::binary);
    out.write(buffer, CHUNK_SIZE);
    out.close();

    QueryPerformanceCounter(&end);

    // 算速度
    double timeSec = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    float speedMB = (float)((CHUNK_SIZE / 1024.0 / 1024.0) / timeSec);

    // 4. 更新图表数组 (环形缓冲)
    g_speedHistory[g_historyOffset] = speedMB;
    g_historyOffset = (g_historyOffset + 1) % 90; // 滚动

    // 5. 清理
    DeleteFileA(testFile.c_str());
    delete[] buffer;
}


// 【新增】拷贝文件测试
void CopyFileToUSB() {
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) {
        AppLog("错误：未选择目标 U 盘");
        return;
    }

    // 1. 获取当前程序自己的完整路径 (作为源文件)
    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);

    // 2. 构造目标路径
    std::string drive = g_logicalDrives[g_selectedDriveIdx];
    // 目标文件名叫 copy_test_file.exe
    std::string destPathStr = drive + "copy_test_file.exe";

    // 需要把 destPathStr 转回 wstring 传给 API
    int len = MultiByteToWideChar(CP_UTF8, 0, destPathStr.c_str(), -1, NULL, 0);
    wchar_t* wDestPath = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, destPathStr.c_str(), -1, wDestPath, len);

    AppLog("正在拷贝文件...");

    // 3. 执行拷贝
    // FALSE 表示如果文件存在则覆盖
    if (CopyFileW(selfPath, wDestPath, FALSE)) {
        AppLog("拷贝成功: " + destPathStr);
    }
    else {
        AppLog("拷贝失败，错误码: " + std::to_string(GetLastError()));
    }

    delete[] wDestPath;
}

void DeleteFileFromUSB() {
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) return;

    std::string drive = g_logicalDrives[g_selectedDriveIdx];

    // 定义我们要清理的文件列表
    std::vector<std::string> filesToDelete = {
        "imgui_test.txt",       // 写入测试产生
        "copy_test_file.exe",   // 拷贝测试产生
        "monitor.tmp",          // 监控产生
        "speed_test.tmp"        // 测速产生
    };

    int successCount = 0;
    for (const auto& fname : filesToDelete) {
        std::string fullPath = drive + fname;

        // 转宽字符
        int len = MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, NULL, 0);
        wchar_t* wPath = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, wPath, len);

        if (DeleteFileW(wPath)) {
            successCount++;
        }
        delete[] wPath;
    }

    if (successCount > 0) {
        AppLog("清理完成，删除了 " + std::to_string(successCount) + " 个测试文件。");
    }
    else {
        AppLog("未发现测试文件或删除失败。");
    }
}

std::string GetDriveLabel(const std::string& driveLetter) {
    char volumeName[MAX_PATH + 1] = { 0 };
    char fileSystemName[MAX_PATH + 1] = { 0 };

    // driveLetter 格式必须是 "F:\\"
    if (GetVolumeInformationA(
        driveLetter.c_str(),
        volumeName,
        sizeof(volumeName),
        NULL, NULL, NULL,
        fileSystemName,
        sizeof(fileSystemName)
    )) {
        std::string label = volumeName;
        if (label.empty()) label = "Removable Disk";
        return label;
    }
    return "Unknown Device";
}