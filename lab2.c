#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dbt.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <time.h>
#include <io.h>
#include <direct.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

#define ID_LISTBOX 1001
#define ID_REFRESH 1002
#define ID_WRITE_TXT 1003
#define ID_COPY_FILE 1004
#define ID_DELETE_FILE 1005
#define ID_USB_LIST 1006
#define ID_FILE_LIST 1007
#define ID_STATUS 1008
#define ID_USER_INFO 1009

#define WM_DEVICE_CHANGE (WM_USER + 1)
#define MAX_PATH_LEN 1024

void CopyFileToUSB();

typedef struct {
    wchar_t deviceDesc[256];
    wchar_t manufacturer[256];
    wchar_t deviceId[256];
    wchar_t instanceId[256];
    wchar_t busNumber[32];
    wchar_t speed[32];
    wchar_t driveLetter;
} USBDeviceInfo;

USBDeviceInfo usbDevices[50];
int usbDeviceCount = 0;
wchar_t currentDrive = 0;
wchar_t userName[256];
HWND hWndMain, hListUSB, hListFiles, hStatus;
wchar_t targetFilePath[MAX_PATH_LEN] = L"";
wchar_t copySourcePath[MAX_PATH_LEN] = L"";

// 获取当前登录用户名
void GetCurrentUserName() {
    DWORD size = sizeof(userName) / sizeof(wchar_t);
    GetUserNameW(userName, &size);
}

// 获取USB设备信息
void EnumerateUSBDevices() {
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA deviceInfoData;
    DWORD index = 0;
    
    usbDeviceCount = 0;
    
    // 获取USB设备列表
    hDevInfo = SetupDiGetClassDevsW(NULL, L"USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) return;
    
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    while (SetupDiEnumDeviceInfo(hDevInfo, index++, &deviceInfoData)) {
        wchar_t buffer[256];
        DWORD size;
        
        // 获取设备描述
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &deviceInfoData, 
            SPDRP_DEVICEDESC, NULL, (PBYTE)buffer, sizeof(buffer), &size)) {
            wcscpy(usbDevices[usbDeviceCount].deviceDesc, buffer);
        } else {
            wcscpy(usbDevices[usbDeviceCount].deviceDesc, L"未知设备");
        }
        
        // 获取制造商
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &deviceInfoData, 
            SPDRP_MFG, NULL, (PBYTE)buffer, sizeof(buffer), &size)) {
            wcscpy(usbDevices[usbDeviceCount].manufacturer, buffer);
        } else {
            wcscpy(usbDevices[usbDeviceCount].manufacturer, L"未知制造商");
        }
        
        // 获取设备ID
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &deviceInfoData, 
            SPDRP_HARDWAREID, NULL, (PBYTE)buffer, sizeof(buffer), &size)) {
            wcscpy(usbDevices[usbDeviceCount].deviceId, buffer);
        } else {
            wcscpy(usbDevices[usbDeviceCount].deviceId, L"未知ID");
        }
        
        // 获取实例ID（包含总线信息）
        if (CM_Get_Device_IDW(deviceInfoData.DevInst, buffer, sizeof(buffer) / sizeof(wchar_t), 0) == CR_SUCCESS) {
            wcscpy(usbDevices[usbDeviceCount].instanceId, buffer);
            
            // 从实例ID中解析总线编号
            wchar_t* busPos = wcsstr(buffer, L"USB\\");
            if (busPos) {
                wchar_t busNum[32];
                swscanf(busPos + 4, L"VID_%*4x&PID_%*4x\\%31[^\\]", busNum);
                wcscpy(usbDevices[usbDeviceCount].busNumber, busNum);
            } else {
                wcscpy(usbDevices[usbDeviceCount].busNumber, L"未知总线");
            }
        } else {
            wcscpy(usbDevices[usbDeviceCount].instanceId, L"未知实例ID");
            wcscpy(usbDevices[usbDeviceCount].busNumber, L"未知总线");
        }
        
        // 获取USB设备速度（简化处理）
        HKEY hKey;
        wchar_t keyPath[512];
        swprintf(keyPath, 512, L"SYSTEM\\CurrentControlSet\\Enum\\%s", 
                 usbDevices[usbDeviceCount].instanceId);
        
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD type, dataSize = sizeof(buffer);
            if (RegQueryValueExW(hKey, L"DeviceDesc", NULL, &type, (LPBYTE)buffer, &dataSize) == ERROR_SUCCESS) {
                // 尝试从描述中判断速度
                if (wcsstr(buffer, L"USB 3.0") || wcsstr(buffer, L"USB3")) {
                    wcscpy(usbDevices[usbDeviceCount].speed, L"USB 3.0 (5 Gbps)");
                } else if (wcsstr(buffer, L"USB 2.0")) {
                    wcscpy(usbDevices[usbDeviceCount].speed, L"USB 2.0 (480 Mbps)");
                } else if (wcsstr(buffer, L"USB 1.1")) {
                    wcscpy(usbDevices[usbDeviceCount].speed, L"USB 1.1 (12 Mbps)");
                } else {
                    wcscpy(usbDevices[usbDeviceCount].speed, L"未知速度");
                }
            } else {
                wcscpy(usbDevices[usbDeviceCount].speed, L"未知速度");
            }
            RegCloseKey(hKey);
        } else {
            wcscpy(usbDevices[usbDeviceCount].speed, L"未知速度");
        }
        
        // 检查是否是存储设备并获取盘符
        wchar_t drivePath[] = L"?:\\";
        for (wchar_t drive = L'A'; drive <= L'Z'; drive++) {
            drivePath[0] = drive;
            UINT type = GetDriveTypeW(drivePath);
            if (type == DRIVE_REMOVABLE) {
                wchar_t volumeName[MAX_PATH];
                if (GetVolumeInformationW(drivePath, volumeName, sizeof(volumeName) / sizeof(wchar_t), 
                    NULL, NULL, NULL, NULL, 0)) {
                    usbDevices[usbDeviceCount].driveLetter = drive;
                    break;
                }
            }
        }
        
        usbDeviceCount++;
        if (usbDeviceCount >= 50) break;
    }
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
}

// 刷新USB设备列表显示
void RefreshUSBList() {
    SendMessage(hListUSB, LB_RESETCONTENT, 0, 0);
    EnumerateUSBDevices();
    
    for (int i = 0; i < usbDeviceCount; i++) {
        wchar_t displayText[512];
        swprintf(displayText, 512, L"%s [制造商: %s] [总线: %s] [速度: %s] [盘符: %c]",
                 usbDevices[i].deviceDesc,
                 usbDevices[i].manufacturer,
                 usbDevices[i].busNumber,
                 usbDevices[i].speed,
                 usbDevices[i].driveLetter ? usbDevices[i].driveLetter : L'-');
        SendMessageW(hListUSB, LB_ADDSTRING, 0, (LPARAM)displayText);
    }
}

// 刷新U盘文件列表
void RefreshFileList() {
    SendMessage(hListFiles, LB_RESETCONTENT, 0, 0);
    
    if (!currentDrive) return;
    
    wchar_t path[MAX_PATH_LEN];
    swprintf(path, MAX_PATH_LEN, L"%c:\\*.*", currentDrive);
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(path, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                wchar_t displayText[512];
                const wchar_t* type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? L"[目录]" : L"[文件]";
                const wchar_t* hidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? L"[隐藏]" : L"";
                LONGLONG fileSize = ((LONGLONG)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
                swprintf(displayText, 512, L"%s %s %s %lld字节", 
                         findData.cFileName, type, hidden, fileSize);
                SendMessageW(hListFiles, LB_ADDSTRING, 0, (LPARAM)displayText);
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
}

// 写入文本到U盘
void WriteTextToUSB() {
    if (!currentDrive) {
        MessageBoxW(hWndMain, L"请先选择U盘", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    wchar_t filePath[MAX_PATH_LEN];
    swprintf(filePath, MAX_PATH_LEN, L"%c:\\usb_test_%d.txt", currentDrive, GetTickCount());
    
    FILE* file = _wfopen(filePath, L"w, ccs=UTF-8");
    if (file) {
        fwprintf(file, L"USB设备测试文件\n");
        fwprintf(file, L"生成时间: %s\n", L"当前时间");
        fwprintf(file, L"当前用户: %s\n", userName);
        fwprintf(file, L"设备信息: USB测试程序生成\n");
        fclose(file);
        
        wchar_t msg[256];
        swprintf(msg, 256, L"已创建测试文件: %s", filePath);
        MessageBoxW(hWndMain, msg, L"成功", MB_OK | MB_ICONINFORMATION);
        RefreshFileList();
    } else {
        MessageBoxW(hWndMain, L"无法创建文件", L"错误", MB_OK | MB_ICONERROR);
    }
}

// 删除U盘中的文件
void DeleteSelectedFile() {
    if (!currentDrive) {
        MessageBoxW(hWndMain, L"请先选择U盘", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    int index = SendMessage(hListFiles, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR) {
        MessageBoxW(hWndMain, L"请选择要删除的文件", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    wchar_t fileName[256];
    SendMessageW(hListFiles, LB_GETTEXT, index, (LPARAM)fileName);
    
    // 提取文件名（去除显示信息）
    wchar_t* spacePos = wcschr(fileName, L' ');
    if (spacePos) *spacePos = L'\0';
    
    wchar_t filePath[MAX_PATH_LEN];
    swprintf(filePath, MAX_PATH_LEN, L"%c:\\%s", currentDrive, fileName);
    
    if (MessageBoxW(hWndMain, L"确定要删除此文件吗？", L"确认删除", 
                   MB_YESNO | MB_ICONQUESTION) == IDYES) {
        if (DeleteFileW(filePath)) {
            MessageBoxW(hWndMain, L"文件删除成功", L"成功", MB_OK | MB_ICONINFORMATION);
            RefreshFileList();
        } else {
            MessageBoxW(hWndMain, L"文件删除失败", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

// 选择要拷贝的源文件
void SelectSourceFile() {
    OPENFILENAMEW ofn;
    wchar_t fileName[MAX_PATH_LEN] = L"";
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWndMain;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName) / sizeof(wchar_t);
    ofn.lpstrFilter = L"所有文件\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameW(&ofn)) {
        wcscpy(copySourcePath, fileName);
        
        wchar_t statusMsg[512];
        swprintf(statusMsg, 512, L"已选择源文件: %s", fileName);
        SetWindowTextW(hStatus, statusMsg);
        
        // 开始拷贝文件
        CopyFileToUSB();
    }
}

// 拷贝文件到U盘
void CopyFileToUSB() {
    if (!currentDrive) {
        MessageBoxW(hWndMain, L"请先选择U盘", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (wcslen(copySourcePath) == 0) {
        SelectSourceFile();
        return;
    }
    
    wchar_t* fileName = wcsrchr(copySourcePath, L'\\');
    if (!fileName) fileName = copySourcePath;
    else fileName++;
    
    wchar_t destPath[MAX_PATH_LEN];
    swprintf(destPath, MAX_PATH_LEN, L"%c:\\%s", currentDrive, fileName);
    
    clock_t start = clock();
    
    // 拷贝文件
    if (CopyFileW(copySourcePath, destPath, FALSE)) {
        clock_t end = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        
        // 获取文件大小
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesExW(copySourcePath, GetFileExInfoStandard, &fileInfo)) {
            LONGLONG fileSize = ((LONGLONG)fileInfo.nFileSizeHigh << 32) | fileInfo.nFileSizeLow;
            double speed = fileSize / elapsed / 1024 / 1024; // MB/s
            
            wchar_t msg[512];
            swprintf(msg, 512, L"文件拷贝成功！\n大小: %.2f MB\n用时: %.2f 秒\n速度: %.2f MB/s",
                     fileSize / 1024.0 / 1024.0, elapsed, speed);
            MessageBoxW(hWndMain, msg, L"拷贝完成", MB_OK | MB_ICONINFORMATION);
            
            RefreshFileList();
        }
    } else {
        MessageBoxW(hWndMain, L"文件拷贝失败", L"错误", MB_OK | MB_ICONERROR);
    }
    
    copySourcePath[0] = L'\0';
    SetWindowTextW(hStatus, L"就绪");
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // 创建控件
            CreateWindowW(L"STATIC", L"当前用户:", WS_CHILD | WS_VISIBLE, 
                         10, 10, 80, 20, hWnd, NULL, NULL, NULL);
                         
            HWND hUser = CreateWindowW(L"EDIT", userName, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
                         100, 10, 200, 20, hWnd, (HMENU)ID_USER_INFO, NULL, NULL);
                         
            CreateWindowW(L"STATIC", L"USB设备列表:", WS_CHILD | WS_VISIBLE,
                         10, 40, 100, 20, hWnd, NULL, NULL, NULL);
                         
            hListUSB = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                         10, 60, 580, 120, hWnd, (HMENU)ID_USB_LIST, NULL, NULL);
                         
            CreateWindowW(L"BUTTON", L"刷新设备列表", WS_CHILD | WS_VISIBLE,
                         10, 190, 120, 30, hWnd, (HMENU)ID_REFRESH, NULL, NULL);
                         
            CreateWindowW(L"STATIC", L"U盘文件列表:", WS_CHILD | WS_VISIBLE,
                         10, 230, 100, 20, hWnd, NULL, NULL, NULL);
                         
            hListFiles = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                         10, 250, 580, 150, hWnd, (HMENU)ID_FILE_LIST, NULL, NULL);
                         
            CreateWindowW(L"BUTTON", L"创建测试文件", WS_CHILD | WS_VISIBLE,
                         10, 410, 120, 30, hWnd, (HMENU)ID_WRITE_TXT, NULL, NULL);
                         
            CreateWindowW(L"BUTTON", L"拷贝文件到U盘", WS_CHILD | WS_VISIBLE,
                         140, 410, 120, 30, hWnd, (HMENU)ID_COPY_FILE, NULL, NULL);
                         
            CreateWindowW(L"BUTTON", L"删除选中文件", WS_CHILD | WS_VISIBLE,
                         270, 410, 120, 30, hWnd, (HMENU)ID_DELETE_FILE, NULL, NULL);
                         
            CreateWindowW(L"BUTTON", L"刷新文件列表", WS_CHILD | WS_VISIBLE,
                         400, 410, 120, 30, hWnd, NULL, NULL, NULL);
                         
            hStatus = CreateWindowW(L"STATIC", L"就绪", WS_CHILD | WS_VISIBLE | SS_LEFT,
                         10, 450, 580, 20, hWnd, (HMENU)ID_STATUS, NULL, NULL);
                         
            // 初始化
            GetCurrentUserName();
            RefreshUSBList();
            return 0;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_REFRESH:
                    RefreshUSBList();
                    break;
                    
                case ID_WRITE_TXT:
                    WriteTextToUSB();
                    break;
                    
                case ID_COPY_FILE:
                    CopyFileToUSB();
                    break;
                    
                case ID_DELETE_FILE:
                    DeleteSelectedFile();
                    break;
                    
                case ID_USB_LIST:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        int index = SendMessage(hListUSB, LB_GETCURSEL, 0, 0);
                        if (index != LB_ERR && index < usbDeviceCount) {
                            currentDrive = usbDevices[index].driveLetter;
                            RefreshFileList();
                        }
                    }
                    break;
                    
                case ID_FILE_LIST:
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        // 双击打开文件
                        DeleteSelectedFile();
                    }
                    break;
            }
            return 0;
        }
        
        case WM_DEVICECHANGE: {
            // 处理设备变化事件
            if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
                RefreshUSBList();
                
                wchar_t msg[256];
                if (wParam == DBT_DEVICEARRIVAL) {
                    wcscpy(msg, L"检测到USB设备插入");
                } else {
                    wcscpy(msg, L"检测到USB设备拔出");
                    currentDrive = 0;
                    SendMessage(hListFiles, LB_RESETCONTENT, 0, 0);
                }
                SetWindowTextW(hStatus, msg);
            }
            return 0;
        }
        
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// 注册设备变化通知
void RegisterForDeviceNotifications() {
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(NotificationFilter);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    
    RegisterDeviceNotification(hWndMain, &NotificationFilter, 
                              DEVICE_NOTIFY_WINDOW_HANDLE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"USBTestWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassW(&wc);
    
    // 创建窗口
    hWndMain = CreateWindowW(L"USBTestWindowClass", L"USB设备管理工具", 
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 620, 520,
                            NULL, NULL, hInstance, NULL);
    
    if (!hWndMain) return 0;
    
    // 注册设备通知
    RegisterForDeviceNotifications();
    
    // 显示窗口
    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}