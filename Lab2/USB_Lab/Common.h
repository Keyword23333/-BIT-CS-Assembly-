#pragma once
#include <string>
#include <vector>
#include <windows.h>

// 定义 USB 设备结构体
struct USBDevice {
    std::string name;           // 设备名称（如：USB Mass Storage Device）
    std::string vendor;         // 厂商信息（Vendor / Manufacturer）
    std::string hwid;           // 硬件 ID（Hardware ID）
	std::string protocol;       // 协议版本（如 USB 2.0 / USB 3.0）
};

// 测速结果结构体
struct SpeedResult {
	bool valid;                  // 测试结果是否有效
	double writeSpeedMBps;       // 写入速度 (MB/s)
	double readSpeedMBps;        // 读取速度 (MB/s)
	std::string message;         // 状态消息
};

extern bool g_isMonitorRunning;         // 开关
extern float g_speedHistory[90];        // 保存最近90次的数据(图表用)
extern int g_historyOffset;             // 图表滚动偏移量
extern std::string g_monitorDevName;    // 当前正在测的设备名
extern DWORD g_lastMonitorTick;         // 上一次测速的时间戳

 
extern std::vector<USBDevice> g_usbDevices;             // 存储当前检测到的 USB 设备列表
extern std::vector<std::string> g_logicalDrives;        // 存储系统的逻辑驱动器列表（如 F:\）
extern std::vector<std::string> g_logs;                 // 应用程序日志，用于记录事件或调试信息
extern int g_selectedDriveIdx;                          // 当前选中的驱动器索引（用于 UI 或内部逻辑）
extern int g_selectedDeviceIndex;                       // 当前选中的 USB 设备索引
extern char g_userBuffer[128];                          // 用户输入的字符缓冲区（大小为 128 字节）
extern SpeedResult g_lastSpeedTest;                     // 保存上一次磁盘测速结果的结构体


// 工具函数声明
void AppLog(const std::string& msg);                    // 记录日志，将消息追加到 g_logs 中
std::string WStringToString(const std::wstring& wstr);  // 将 std::wstring 转换为 std::string（通常用于处理 Windows API 返回的宽字符）
void RunDiskBenchmark();                                // 运行磁盘基准测试，测量读写速度
void UpdateRealTimeMonitor();                           // 更新实时监控数据
void CopyFileToUSB();                                   // 将指定文件复制到选中的 USB 设备
void DeleteFileFromUSB();                               // 从选中的 USB 设备删除指定文件
std::string GetDriveLabel(const std::string& driveLetter);		  // 获取指定驱动器的卷标（如 SanDisk）