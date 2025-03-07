

#include "DarwinStorageProvider.h"
#include "utils/RegExp.h"
#include "utils/StdString.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"

#include <sys/mount.h>
#if !defined(__arm__)
#include <DiskArbitration/DiskArbitration.h>
#endif
#include "CocoaInterface.h"

bool CDarwinStorageProvider::m_event = false;

CDarwinStorageProvider::CDarwinStorageProvider()
{
  PumpDriveChangeEvents(NULL);
}

void CDarwinStorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;

  // User home folder
  share.strPath = getenv("HOME");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  localDrives.push_back(share);

#if !defined(__arm__)
  // User desktop folder
  share.strPath = getenv("HOME");
  share.strPath += "/Desktop";
  share.strName = "Desktop";
  share.m_ignore = true;
  localDrives.push_back(share);

  // Volumes (all mounts are present here)
  share.strPath = "/Volumes";
  share.strName = "Volumes";
  share.m_ignore = true;
  localDrives.push_back(share);

  // This will pick up all local non-removable disks including the Root Disk.
  DASessionRef session = DASessionCreate(kCFAllocatorDefault);
  if (session)
  {
    unsigned i, count = 0;
    struct statfs *buf = NULL;
    CStdString mountpoint, devicepath;

    count = getmntinfo(&buf, 0);
    for (i=0; i<count; i++)
    {
      mountpoint = buf[i].f_mntonname;
      devicepath = buf[i].f_mntfromname;

      DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, devicepath.c_str());
      if (disk)
      {
        CFDictionaryRef details = DADiskCopyDescription(disk);
        if (details)
        {
          if (kCFBooleanFalse == CFDictionaryGetValue(details, kDADiskDescriptionMediaRemovableKey))
          {
            CMediaSource share;

            share.strPath = mountpoint;
            Cocoa_GetVolumeNameFromMountPoint(mountpoint, share.strName);
            share.m_ignore = true;
            localDrives.push_back(share);
          }
          CFRelease(details);
        }
        CFRelease(disk);
      }
    }

    CFRelease(session);
  }
#endif
}

void CDarwinStorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
#if !defined(__arm__)
  DASessionRef session = DASessionCreate(kCFAllocatorDefault);
  if (session)
  {
    unsigned i, count = 0;
    struct statfs *buf = NULL;
    CStdString mountpoint, devicepath;

    count = getmntinfo(&buf, 0);
    for (i=0; i<count; i++)
    {
      mountpoint = buf[i].f_mntonname;
      devicepath = buf[i].f_mntfromname;

      DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, devicepath.c_str());
      if (disk)
      {
        CFDictionaryRef details = DADiskCopyDescription(disk);
        if (details)
        {
          if (kCFBooleanTrue == CFDictionaryGetValue(details, kDADiskDescriptionMediaRemovableKey))
          {
            CMediaSource share;

            share.strPath = mountpoint;
            Cocoa_GetVolumeNameFromMountPoint(mountpoint, share.strName);
            share.m_ignore = true;
            removableDrives.push_back(share);
          }
          CFRelease(details);
        }
        CFRelease(disk);
      }
    }

    CFRelease(session);
  }
#endif
}

std::vector<CStdString> CDarwinStorageProvider::GetDiskUsage()
{
  std::vector<CStdString> result;
  char line[1024];

#ifdef TARGET_DARWIN_IOS
  FILE* pipe = popen("df -ht hfs", "r");
#else
  FILE* pipe = popen("df -hT ufs,cd9660,hfs,udf", "r");
#endif

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      result.push_back(line);
    }
    pclose(pipe);
  }

  return result;
}

bool CDarwinStorageProvider::Eject(CStdString mountpath)
{
  return false;
}

bool CDarwinStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  bool event = m_event;
  m_event = false;
  return event;
}

void CDarwinStorageProvider::SetEvent(void)
{
  CDarwinStorageProvider::m_event = true;
}
