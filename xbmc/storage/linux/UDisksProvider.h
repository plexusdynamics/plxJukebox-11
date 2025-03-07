#pragma once

#include "storage/IStorageProvider.h"
#ifdef HAS_DBUS
#include "DBusUtil.h"

class CUDiskDevice
{
public:
  CUDiskDevice(const char *DeviceKitUDI);
  ~CUDiskDevice() { }

  void Update();

  bool Mount();
  bool UnMount();

  bool IsApproved();

  CStdString toString();

  CMediaSource ToMediaShare();

  CStdString m_UDI, m_DeviceKitUDI, m_MountPath, m_FileSystem, m_Label;
  bool m_isMounted, m_isMountedByUs, m_isRemovable, m_isPartition, m_isFileSystem, m_isSystemInternal, m_isOptical;
  float m_PartitionSizeGiB;
};

class CUDisksProvider : public IStorageProvider
{
public:
  CUDisksProvider();
  virtual ~CUDisksProvider();

  virtual void Initialize();
  virtual void Stop() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives) { GetDisks(localDrives, false); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { GetDisks(removableDrives, true); }

  virtual bool Eject(CStdString mountpath);

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents(IStorageEventsCallback *callback);

  static bool HasUDisks();
private:
  typedef std::map<CStdString, CUDiskDevice *> DeviceMap;
  typedef std::pair<CStdString, CUDiskDevice *> DevicePair;

  void DeviceAdded(const char *object, IStorageEventsCallback *callback);
  void DeviceRemoved(const char *object, IStorageEventsCallback *callback);
  void DeviceChanged(const char *object, IStorageEventsCallback *callback);

  std::vector<CStdString> EnumerateDisks();

  void GetDisks(VECSOURCES& devices, bool EnumerateRemovable);

  int m_DaemonVersion;

  DeviceMap m_AvailableDevices;

  DBusConnection *m_connection;
  DBusError m_error;
};
#endif
