#pragma once
#include <string>
#include <vector>
#include <windows.h>

// ���� USB �豸�ṹ��
struct USBDevice {
    std::string name;           // �豸���ƣ��磺USB Mass Storage Device��
    std::string vendor;         // ������Ϣ��Vendor / Manufacturer��
    std::string hwid;           // Ӳ�� ID��Hardware ID��
	std::string protocol;       // Э��汾���� USB 2.0 / USB 3.0��
};

// ���ٽ���ṹ��
struct SpeedResult {
	bool valid;                  // ���Խ���Ƿ���Ч
	double writeSpeedMBps;       // д���ٶ� (MB/s)
	double readSpeedMBps;        // ��ȡ�ٶ� (MB/s)
	std::string message;         // ״̬��Ϣ
};

extern bool g_isMonitorRunning;         // ����
extern float g_speedHistory[90];        // �������90�ε�����(ͼ����)
extern int g_historyOffset;             // ͼ������ƫ����
extern std::string g_monitorDevName;    // ��ǰ���ڲ���豸��
extern DWORD g_lastMonitorTick;         // ��һ�β��ٵ�ʱ���

 
extern std::vector<USBDevice> g_usbDevices;             // �洢��ǰ��⵽�� USB �豸�б�
extern std::vector<std::string> g_logicalDrives;        // �洢ϵͳ���߼��������б����� F:\��
extern std::vector<std::string> g_logs;                 // Ӧ�ó�����־�����ڼ�¼�¼��������Ϣ
extern int g_selectedDriveIdx;                          // ��ǰѡ�е����������������� UI ���ڲ��߼���
extern int g_selectedDeviceIndex;                       // ��ǰѡ�е� USB �豸����
extern char g_userBuffer[128];                          // �û�������ַ�����������СΪ 128 �ֽڣ�
extern SpeedResult g_lastSpeedTest;                     // ������һ�δ��̲��ٽ���Ľṹ��

extern std::vector<std::string> g_currentDriveFiles;    // 当前显示的U盘文件列表（只列出根目录）
extern int g_selectedFileIndex;                         // UI中选中的文件索引
extern bool g_showDriveFiles;                           // 是否在右下区域显示U盘文件列表
extern std::string g_currentPath;                       // 当前浏览的路径（包含盘符），例如 "F:\\"
extern std::vector<std::string> g_pathStack;            // 路径栈，用于返回


// ���ߺ�������
void AppLog(const std::string& msg);                    // ��¼��־������Ϣ׷�ӵ� g_logs ��
std::string WStringToString(const std::wstring& wstr);  // �� std::wstring ת��Ϊ std::string��ͨ�����ڴ��� Windows API ���صĿ��ַ���
void RunDiskBenchmark();                                // ���д��̻�׼���ԣ�������д�ٶ�
void UpdateRealTimeMonitor();                           // ����ʵʱ�������
void CopyFileToUSB();                                   // ��ָ���ļ����Ƶ�ѡ�е� USB �豸
void DeleteFileFromUSB();                               // ��ѡ�е� USB �豸ɾ��ָ���ļ�
std::string GetDriveLabel(const std::string& driveLetter);		  // ��ȡָ���������ľ��꣨�� SanDisk��

// 文件列表操作（在UI中使用）
void ListFilesInDrive(const std::string& drive);
bool DeleteFileOnDrive(const std::string& fullPath);
bool CopyLocalFileToDriveWithDialog(const std::string& drive);
bool EnterDirectory(const std::string& dirname);
bool GoUpDirectory();
bool CopyFileFromDriveToPC(const std::string& srcRelative);