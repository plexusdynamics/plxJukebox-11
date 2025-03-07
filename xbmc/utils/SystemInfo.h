#pragma once



#include "md5.h"
#include "InfoLoader.h"

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define MAX_KNOWN_ATTRIBUTES  46


class CSysData
{
public:
  enum INTERNET_STATE { UNKNOWN, CONNECTED, NO_DNS, DISCONNECTED };
  CSysData()
  {
    Reset();
  };

  void Reset()
  {
    internetState = UNKNOWN;
  };

  CStdString systemUptime;
  CStdString systemTotalUptime;
  INTERNET_STATE internetState;
  CStdString videoEncoder;
  CStdString cpuFrequency;
  CStdString kernelVersion;
  CStdString macAddress;
  CStdString batteryLevel;
};

class CSysInfoJob : public CJob
{
public:
  CSysInfoJob();

  virtual bool DoWork();
  const CSysData &GetData() const;

  static CSysData::INTERNET_STATE GetInternetState();
private:
  bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);
  double GetCPUFrequency();
  CStdString GetSystemUpTime(bool bTotalUptime);
  CStdString GetCPUFreqInfo();
  CStdString GetMACAddress();
  CStdString GetVideoEncoder();
  CStdString GetBatteryLevel();

  CSysData m_info;
};

class CSysInfo : public CInfoLoader
{
public:
  CSysInfo(void);
  virtual ~CSysInfo();

  char MD5_Sign[32 + 1];

  bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
  bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
  bool GetRefurbInfo(CStdString& rfi_FirstBootTime, CStdString& rfi_PowerCycleCount);

#if defined(_LINUX) && !defined(__APPLE__) && !defined(__FreeBSD__)
  CStdString GetLinuxDistro();
#endif
#ifdef _LINUX
  CStdString GetUnameVersion();
#endif
#if defined(TARGET_WINDOWS)
  CStdString CSysInfo::GetUAWindowsVersion();
#endif
  CStdString GetUserAgent();
  bool HasInternet();
  bool IsAppleTV();
  bool IsAppleTV2();
  bool HasVDADecoder();
  bool HasVideoToolBoxDecoder();
  bool IsAeroDisabled();
  bool IsVistaOrHigher();
  static CStdString GetKernelVersion();
  CStdString GetXBVerInfo();
  bool GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
  CStdString GetHddSpaceInfo(int& percent, int drive, bool shortText=false);
  CStdString GetHddSpaceInfo(int drive, bool shortText=false);

protected:
  virtual CJob *GetJob() const;
  virtual CStdString TranslateInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CSysData m_info;
  void Reset();
};

extern CSysInfo g_sysinfo;

