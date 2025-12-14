#pragma execution_character_set("utf-8") // ����ӣ���ֹ��������
#include "USBCore.h"
#include "Common.h"
#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <devguid.h>
#include <fstream>
#include <sstream>
#include <commdlg.h>

#pragma comment(lib, "setupapi.lib")

// ==========================================
// ȫ�ֱ������� (����������ڴ�)
// ==========================================
std::vector<USBDevice> g_usbDevices;                                      // �洢��ǰ��⵽�� USB �豸�б�
std::vector<std::string> g_logicalDrives;                                 // �洢ϵͳ���߼��������б����� F:\��
std::vector<std::string> g_logs;                                          // Ӧ�ó�����־�����ڼ�¼�¼��������Ϣ
int g_selectedDriveIdx = -1;                                              // ��ǰѡ�е����������������� UI ���ڲ��߼���
int g_selectedDeviceIndex = -1;                                           // ��ǰѡ�е� USB �豸����
char g_userBuffer[128] = "";                                              // �û�������ַ�����������СΪ 128 �ֽڣ�
SpeedResult g_lastSpeedTest = { false, 0.0, 0.0, "�ȴ�����..." };         // ������һ�δ��̲��ٽ���Ľṹ��

bool g_isMonitorRunning = false;                                          // ����
float g_speedHistory[90] = { 0.0f };                                      // ��ʼ��Ϊ0
int g_historyOffset = 0;                                                  // ͼ������ƫ����
std::string g_monitorDevName = "Unknown";                                 // ��ǰ���ڲ���豸��
DWORD g_lastMonitorTick = 0;                                              // ��һ�β��ٵ�ʱ���

// ==========================================
// ���ߺ���ʵ��
// ==========================================
std::string GuessUSBProtocol(const std::string& hwid, const std::string& name) {
    // ����һ������ʽ�жϣ�׼ȷ��ȡ��Ҫ���ӵ� IOCTL ��ѯ
    if (hwid.find("ROOT_HUB30") != std::string::npos) return "USB 3.0 (Root)";
    if (hwid.find("USB 3.0") != std::string::npos) return "USB 3.0";
    if (name.find("USB 3.0") != std::string::npos) return "USB 3.0";
    if (name.find("SuperSpeed") != std::string::npos) return "USB 3.x";
    if (name.find("Composite") != std::string::npos) return "Composite";
    // �󲿷�Ĭ���� 2.0
    return "USB 2.0 / 1.1";
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 1) return std::string();
    // 不包含末尾的 NUL 字节
    std::string strTo(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &strTo[0], size_needed - 1, NULL, NULL);
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
// ҵ���߼�ʵ��
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
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(NULL, L"USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA DeviceInfoData;
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
        DWORD DataT;
        wchar_t buffer[1024];
        DWORD buffersize = 0;

        if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &DeviceInfoData, SPDRP_FRIENDLYNAME, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize))
            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize);
        std::wstring name = buffer;

        SetupDiGetDeviceRegistryPropertyW(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize);
        std::wstring hwid = buffer;

        SetupDiGetDeviceRegistryPropertyW(hDevInfo, &DeviceInfoData, SPDRP_MFG, &DataT, (PBYTE)buffer, sizeof(buffer), &buffersize);
        std::wstring mfg = buffer;

        USBDevice dev;
        dev.name = WStringToString(name);
        dev.hwid = WStringToString(hwid);
        dev.vendor = WStringToString(mfg);

        bool is30 = false;
        // �򵥵Ĺؼ���ƥ��
        if (dev.hwid.find("ROOT_HUB30") != std::string::npos) is30 = true;
        if (dev.hwid.find("USB 3.0") != std::string::npos) is30 = true;
        if (dev.name.find("USB 3.0") != std::string::npos) is30 = true;
        if (dev.name.find("SuperSpeed") != std::string::npos) is30 = true;

        if (is30) dev.protocol = "USB 3.0";
        else dev.protocol = "USB 2.0 / 1.1";

        g_usbDevices.push_back(dev);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    AppLog("USB 设备列表已刷新！");
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

    if (g_logicalDrives.empty()) AppLog("未检测到可移动设备。");
    else AppLog("检测到 U 盘。");

    // 如果 UI 正在显示当前 U 盘目录，尝试刷新该目录的列表（保持在当前目录）
    if (g_showDriveFiles) {
        if (!g_currentPath.empty()) {
            ListFilesInDrive(g_currentPath);
        }
        else if (g_selectedDriveIdx >= 0 && g_selectedDriveIdx < g_logicalDrives.size()) {
            ListFilesInDrive(g_logicalDrives[g_selectedDriveIdx]);
        }
    }
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
        AppLog("写入失败。");
    }
}

void RunDiskBenchmark() {
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) {
        g_lastSpeedTest.message = "错误: 未选择 U 盘";
        return;
    }

    std::string drive = g_logicalDrives[g_selectedDriveIdx];
    std::string testFile = drive + "speed_test.tmp";
    const size_t DATA_SIZE = 10 * 1024 * 1024; // ���� 10MB ����
    char* buffer = new char[DATA_SIZE];

    // ���������ݷ�ֹ��ѹ��
    for (size_t i = 0; i < DATA_SIZE; i++) buffer[i] = (char)(i % 256);

    AppLog("开始基准测试 (大小: 10MB)...");
    g_lastSpeedTest.message = "正在写入...";

    // 1. д�����
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);

    QueryPerformanceCounter(&start);
    std::ofstream out(testFile, std::ios::binary);
    out.write(buffer, DATA_SIZE);
    out.close(); // �ر���ȷ�� flush
    QueryPerformanceCounter(&end);

    double timeWrite = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    double speedWrite = (DATA_SIZE / 1024.0 / 1024.0) / timeWrite;

    // 2. ��ȡ����
    g_lastSpeedTest.message = "正在读取...";
    QueryPerformanceCounter(&start);
    std::ifstream in(testFile, std::ios::binary);
    in.read(buffer, DATA_SIZE);
    in.close();
    QueryPerformanceCounter(&end);

    double timeRead = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    double speedRead = (DATA_SIZE / 1024.0 / 1024.0) / timeRead;

    // 3. ��������
    DeleteFileA(testFile.c_str());
    delete[] buffer;

    g_lastSpeedTest.valid = true;
    g_lastSpeedTest.writeSpeedMBps = speedWrite;
    g_lastSpeedTest.readSpeedMBps = speedRead;

    char msg[128];
    sprintf_s(msg, "д��: %.1f MB/s | ��ȡ: %.1f MB/s", speedWrite, speedRead);
    g_lastSpeedTest.message = msg;
    AppLog(std::string("基准测试结果: ") + std::string(msg));
}

void UpdateRealTimeMonitor() {
    // 1. ���û�����أ�����û��ѡ�豸��ֱ���˳�
    if (!g_isMonitorRunning) return;
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) {
        g_isMonitorRunning = false; // 停止
        AppLog("监测已停止：设备不可用");
        return;
    }

    // 2. ���ʱ���� (ÿ 1000ms ִ��һ��)
    DWORD currentTick = GetTickCount();
    if (currentTick - g_lastMonitorTick < 1000) {
        return; // ʱ��û������ִ��
    }
    g_lastMonitorTick = currentTick; // ����ʱ���

    // 3. ִ��һ��΢�Ͳ��� (ֻ��д�룬Ϊ�˿�����Ӧ)
    // Ϊʲôֻ�� 512KB����Ϊ����� 10MB��USB 2.0��Ҫд2�룬����Ῠ����
    std::string drive = g_logicalDrives[g_selectedDriveIdx];
    std::string testFile = drive + "monitor.tmp";
    const size_t CHUNK_SIZE = 512 * 1024; // 512KB
    char* buffer = new char[CHUNK_SIZE];

    // �����豸����������ʾ
    g_monitorDevName = drive + " (实时)";

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    // д�ļ�
    std::ofstream out(testFile, std::ios::binary);
    out.write(buffer, CHUNK_SIZE);
    out.close();

    QueryPerformanceCounter(&end);

    // ���ٶ�
    double timeSec = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    float speedMB = (float)((CHUNK_SIZE / 1024.0 / 1024.0) / timeSec);

    // 4. ����ͼ������ (���λ���)
    g_speedHistory[g_historyOffset] = speedMB;
    g_historyOffset = (g_historyOffset + 1) % 90; // ����

    // 5. ����
    DeleteFileA(testFile.c_str());
    delete[] buffer;
}


// �������������ļ�����
void CopyFileToUSB() {
    if (g_selectedDriveIdx < 0 || g_logicalDrives.empty()) {
        AppLog("����δѡ��Ŀ�� U ��");
        return;
    }

    // 1. ��ȡ��ǰ�����Լ�������·�� (��ΪԴ�ļ�)
    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);

    // 2. ����Ŀ��·��
    std::string drive = g_logicalDrives[g_selectedDriveIdx];
    // Ŀ���ļ����� copy_test_file.exe
    std::string destPathStr = drive + "copy_test_file.exe";

    // ��Ҫ�� destPathStr ת�� wstring ���� API
    int len = MultiByteToWideChar(CP_UTF8, 0, destPathStr.c_str(), -1, NULL, 0);
    wchar_t* wDestPath = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, destPathStr.c_str(), -1, wDestPath, len);

    AppLog("拷贝当前可执行文件...");

    // 3. ִ�п���
    // FALSE ��ʾ����ļ������򸲸�
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

    // ��������Ҫ�������ļ��б�
    std::vector<std::string> filesToDelete = {
        "imgui_test.txt",       // д����Բ���
        "copy_test_file.exe",   // �������Բ���
        "monitor.tmp",          // ��ز���
        "speed_test.tmp"        // ���ٲ���
    };

    int successCount = 0;
    for (const auto& fname : filesToDelete) {
        std::string fullPath = drive + fname;

        // ת���ַ�
        int len = MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, NULL, 0);
        wchar_t* wPath = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, wPath, len);

        if (DeleteFileW(wPath)) {
            successCount++;
        }
        delete[] wPath;
    }

    if (successCount > 0) {
        AppLog("已删除，共删除 " + std::to_string(successCount) + " 个文件");
    }
    else {
        AppLog("未找到可删除的测试文件");
    }
}

std::string GetDriveLabel(const std::string& driveLetter) {
    char volumeName[MAX_PATH + 1] = { 0 };
    char fileSystemName[MAX_PATH + 1] = { 0 };

    // driveLetter ��ʽ������ "F:\\"
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

// ==========================================
// 文件列表 / 删除 / 从本地拷贝到U盘
// ==========================================
std::vector<std::string> g_currentDriveFiles;
int g_selectedFileIndex = -1;
bool g_showDriveFiles = false;
std::string g_currentPath = "";
std::vector<std::string> g_pathStack;

void ListFilesInDrive(const std::string& drive) {
    g_currentDriveFiles.clear();
    g_selectedFileIndex = -1;
    if (drive.empty()) return;

    std::string normalized = drive;
    if (normalized.back() != '\\') normalized += "\\";
    g_currentPath = normalized;
    AppLog(std::string("列出目录: ") + g_currentPath);

    // 使用宽字符 API 以正确读取包含非 ASCII 的文件名
    int wlen = MultiByteToWideChar(CP_UTF8, 0, normalized.c_str(), -1, NULL, 0);
    if (wlen <= 0) return;
    std::vector<wchar_t> wbuf(wlen);
    MultiByteToWideChar(CP_UTF8, 0, normalized.c_str(), -1, wbuf.data(), wlen);
    std::wstring wsearch(wbuf.data());
    std::wstring wsearchPath = wsearch + L"*";

    WIN32_FIND_DATAW findDataW;
    HANDLE hFind = FindFirstFileW(wsearchPath.c_str(), &findDataW);
    if (hFind == INVALID_HANDLE_VALUE) {
        AppLog(std::string("ListFilesInDrive FindFirstFileW failed, error=") + std::to_string(GetLastError()));
        return;
    }
    do {
        std::wstring wname = findDataW.cFileName;
        if (wname == L"." || wname == L"..") continue;
        // 转为 UTF-8（避免在 std::string 中留下 NUL 字节）
        int len = WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), -1, NULL, 0, NULL, NULL);
        if (len <= 1) continue;
        std::string name(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), -1, &name[0], len - 1, NULL, NULL);
        // 标注目录名末尾加 '\\' 以表示目录
        if (findDataW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) name.push_back('\\');
        g_currentDriveFiles.push_back(name);
    } while (FindNextFileW(hFind, &findDataW));
    FindClose(hFind);
}

// 进入目录（目录名可带或不带末尾 '\\'），返回是否成功
bool EnterDirectory(const std::string& dirname) {
    if (dirname.empty() || g_currentPath.empty()) return false;
    std::string name = dirname;
    if (!name.empty() && name.back() == '\\') name.pop_back();

    // 构造下一级路径，确保只有一个末尾 '\\'
    std::string base = g_currentPath;
    if (!base.empty() && base.back() != '\\') base += "\\";
    std::string next = base + name + "\\";

    // 将 next 转为宽字符并检查是否为目录
    int wlen = MultiByteToWideChar(CP_UTF8, 0, next.c_str(), -1, NULL, 0);
    if (wlen <= 0) {
        AppLog(std::string("EnterDirectory: 路径转换失败: ") + next);
        return false;
    }
    std::vector<wchar_t> wbuf(wlen);
    MultiByteToWideChar(CP_UTF8, 0, next.c_str(), -1, wbuf.data(), wlen);
    DWORD attr = GetFileAttributesW(wbuf.data());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        AppLog(std::string("EnterDirectory: 目标不是目录或不存在: ") + next + " err=" + std::to_string(GetLastError()));
        return false;
    }

    // push current path 并进入
    g_pathStack.push_back(g_currentPath);
    AppLog(std::string("进入目录: ") + next);
    ListFilesInDrive(next);
    return true;
}

// 返回上一级目录；如果已到盘根则关闭视图并返回 false
bool GoUpDirectory() {
    if (g_pathStack.empty()) {
        g_showDriveFiles = false;
        g_currentDriveFiles.clear();
        g_currentPath.clear();
        g_selectedFileIndex = -1;
        return false;
    }
    std::string prev = g_pathStack.back();
    g_pathStack.pop_back();
    ListFilesInDrive(prev);
    return true;
}

// 将 U 盘上的文件复制到本地（弹出保存对话框），返回是否成功
bool CopyFileFromDriveToPC(const std::string& srcRelative) {
    if (g_currentPath.empty() || srcRelative.empty()) return false;
    std::string src = g_currentPath + srcRelative;
    // 转为 wide
    int srcLen = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, NULL, 0);
    std::wstring wsrc(srcLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &wsrc[0], srcLen);

    // 默认保存文件名
    size_t pos = srcRelative.find_last_of("/\\");
    std::string defaultName = (pos == std::string::npos) ? srcRelative : srcRelative.substr(pos + 1);
    int dlen = MultiByteToWideChar(CP_UTF8, 0, defaultName.c_str(), -1, NULL, 0);
    std::wstring wDefault(dlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, defaultName.c_str(), -1, &wDefault[0], dlen);

    wchar_t saveFile[MAX_PATH] = { 0 };
    wcscpy_s(saveFile, wDefault.c_str());

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = saveFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All\0*.*\0\0";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&ofn)) return false;

    if (CopyFileW(wsrc.c_str(), saveFile, FALSE)) {
        AppLog("拷贝到本地成功");
        return true;
    }
    else {
        AppLog("拷贝到本地失败，错误码: " + std::to_string(GetLastError()));
        return false;
    }
}

bool DeleteFileOnDrive(const std::string& fullPath) {
    // fullPath 是完整路径，如 "F:\\test.txt" 或目录
    // 尝试删除文件
    if (DeleteFileA(fullPath.c_str())) return true;
    // 如果是目录，尝试 RemoveDirectoryA
    if (RemoveDirectoryA(fullPath.c_str())) return true;
    return false;
}

bool CopyLocalFileToDriveWithDialog(const std::string& drive) {
    // 打开文件选择对话框（宽字符）
    OPENFILENAMEW ofn;
    wchar_t szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All\0*.*\0\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return false;

    // 源路径
    std::wstring wsrc = szFile;
    // 目标路径驱动器必须以 \ 结尾
    std::string destDir = drive; // ANSI
    // 提取源文件名为 UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, wsrc.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 1) return false;
    std::string srcUtf8(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wsrc.c_str(), -1, &srcUtf8[0], len - 1, NULL, NULL);
    // 获取文件名部分（从最后的 '\\'）
    size_t pos = srcUtf8.find_last_of("/\\");
    std::string fileName = (pos == std::string::npos) ? srcUtf8 : srcUtf8.substr(pos + 1);

    std::string destPath = destDir + fileName;

    // 转换为宽字符用于 CopyFileW
    int destLen = MultiByteToWideChar(CP_UTF8, 0, destPath.c_str(), -1, NULL, 0);
    std::wstring wDest(destLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, destPath.c_str(), -1, &wDest[0], destLen);

    if (CopyFileW(wsrc.c_str(), wDest.c_str(), FALSE)) {
        AppLog("拷贝成功: " + destPath);
        return true;
    }
    else {
        AppLog("拷贝失败，错误码: " + std::to_string(GetLastError()));
        return false;
    }
}