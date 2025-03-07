

#include "DllLoaderContainer.h"
#ifdef _LINUX
#include "SoLoader.h"
#endif
#ifdef _WIN32
#include "Win32DllLoader.h"
#endif
#include "DllLoader.h"
#include "dll_tracker.h" // for python unload hack
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#define ENV_PARTIAL_PATH "special://bin/system/;" \
                 "special://bin/system/players/mplayer/;" \
                 "special://bin/system/players/dvdplayer/;" \
                 "special://bin/system/players/paplayer/;" \
                 "special://bin/system/python/;" \
                 "special://root/system/;" \
                 "special://root/system/players/mplayer/;" \
                 "special://root/system/players/dvdplayer/;" \
                 "special://root/system/players/paplayer/;" \
                 "special://root/system/python/"

#ifdef __APPLE__
#define ENV_PATH ENV_PARTIAL_PATH \
                 ";special://frameworks/"
#else
#define ENV_PATH ENV_PARTIAL_PATH
#endif

//Define this to get loggin on all calls to load/unload of dlls
//#define LOGALL


using namespace XFILE;

LibraryLoader* DllLoaderContainer::m_dlls[64] = {};
int        DllLoaderContainer::m_iNrOfDlls = 0;
bool       DllLoaderContainer::m_bTrack = true;

void DllLoaderContainer::Clear()
{
}

HMODULE DllLoaderContainer::GetModuleAddress(const char* sName)
{
  return (HMODULE)GetModule(sName);
}

LibraryLoader* DllLoaderContainer::GetModule(const char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (stricmp(m_dlls[i]->GetName(), sName) == 0) return m_dlls[i];
    if (!m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetFileName(), sName) == 0) return m_dlls[i];
  }

  return NULL;
}

LibraryLoader* DllLoaderContainer::GetModule(HMODULE hModule)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (m_dlls[i]->GetHModule() == hModule) return m_dlls[i];
  }
  return NULL;
}

LibraryLoader* DllLoaderContainer::LoadModule(const char* sName, const char* sCurrentDir/*=NULL*/, bool bLoadSymbols/*=false*/)
{
  LibraryLoader* pDll=NULL;

  if (IsSystemDll(sName))
  {
    pDll = GetModule(sName);
  }
  else if (sCurrentDir)
  {
    CStdString strPath=sCurrentDir;
    strPath+=sName;
    pDll = GetModule(strPath.c_str());
  }

  if (!pDll)
  {
    pDll = GetModule(sName);
  }

  if (!pDll)
  {
    pDll = FindModule(sName, sCurrentDir, bLoadSymbols);
  }
  else if (!pDll->IsSystemDll())
  {
    pDll->IncrRef();

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Already loaded Dll %s at 0x%x", pDll->GetFileName(), pDll);
#endif

  }

  return pDll;
}

LibraryLoader* DllLoaderContainer::FindModule(const char* sName, const char* sCurrentDir, bool bLoadSymbols)
{
  if (URIUtils::IsInArchive(sName))
  {
    CURL url(sName);
    CStdString newName = "special://temp/";
    newName += url.GetFileName();
    CFile::Cache(sName, newName);
    return FindModule(newName, sCurrentDir, bLoadSymbols);
  }

  if (CURL::IsFullPath(sName))
  { //  Has a path, just try to load
    return LoadDll(sName, bLoadSymbols);
  }
#ifdef _LINUX
  else if (strcmp(sName, "xbmc.so") == 0)
    return LoadDll(sName, bLoadSymbols);
#endif
  else if (sCurrentDir)
  { // in the path of the parent dll?
    CStdString strPath=sCurrentDir;
    strPath+=sName;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

  //  in environment variable?
  CStdStringArray vecEnv;
  StringUtils::SplitString(ENV_PATH, ";", vecEnv);
  LibraryLoader* pDll = NULL;

  for (int i=0; i<(int)vecEnv.size(); ++i)
  {
    CStdString strPath=vecEnv[i];

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Searching for the dll %s in directory %s", sName, strPath.c_str());
#endif

    strPath+=sName;

    // Have we already loaded this dll
    if ((pDll = GetModule(strPath.c_str())) != NULL)
      return pDll;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

  // can't find it in any of our paths - could be a system dll
  if ((pDll = LoadDll(sName, bLoadSymbols)) != NULL)
    return pDll;

  CLog::Log(LOGDEBUG, "Dll %s was not found in path", sName);
  return NULL;
}

void DllLoaderContainer::ReleaseModule(LibraryLoader*& pDll)
{
  if (!pDll)
    return;
  if (pDll->IsSystemDll())
  {
    CLog::Log(LOGFATAL, "%s is a system dll and should never be released", pDll->GetName());
    return;
  }

  int iRefCount=pDll->DecrRef();
  if (iRefCount==0)
  {

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Releasing Dll %s", pDll->GetFileName());
#endif

    if (!pDll->HasSymbols())
    {
      pDll->Unload();
      delete pDll;
      pDll=NULL;
    }
    else
      CLog::Log(LOGINFO, "%s has symbols loaded and can never be unloaded", pDll->GetName());
  }
#ifdef LOGALL
  else
  {
    CLog::Log(LOGDEBUG, "Dll %s is still referenced with a count of %d", pDll->GetFileName(), iRefCount);
  }
#endif
}

LibraryLoader* DllLoaderContainer::LoadDll(const char* sName, bool bLoadSymbols)
{

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "Loading dll %s", sName);
#endif

  LibraryLoader* pLoader;
#ifdef _LINUX
  if (strstr(sName, ".so") != NULL || strstr(sName, ".vis") != NULL || strstr(sName, ".xbs") != NULL
      || strstr(sName, ".mvis") != NULL || strstr(sName, ".dylib") != NULL || strstr(sName, ".framework") != NULL)
    pLoader = new SoLoader(sName, bLoadSymbols);
  else
#elif defined(_WIN32)
  if (1)
    pLoader = new Win32DllLoader(sName);
  else
#endif
    pLoader = new DllLoader(sName, m_bTrack, false, bLoadSymbols);

  if (!pLoader)
  {
    CLog::Log(LOGERROR, "Unable to create dll %s", sName);
    return NULL;
  }

  if (!pLoader->Load())
  {
    delete pLoader;
    return NULL;
  }

  return pLoader;
}

bool DllLoaderContainer::IsSystemDll(const char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetName(), sName) == 0) return true;
  }

  return false;
}

int DllLoaderContainer::GetNrOfModules()
{
  return m_iNrOfDlls;
}

LibraryLoader* DllLoaderContainer::GetModule(int iPos)
{
  if (iPos < m_iNrOfDlls) return m_dlls[iPos];
  return NULL;
}

void DllLoaderContainer::RegisterDll(LibraryLoader* pDll)
{
  for (int i = 0; i < 64; i++)
  {
    if (m_dlls[i] == NULL)
    {
      m_dlls[i] = pDll;
      m_iNrOfDlls++;
      break;
    }
  }
}

void DllLoaderContainer::UnRegisterDll(LibraryLoader* pDll)
{
  if (pDll)
  {
    if (pDll->IsSystemDll())
    {
      CLog::Log(LOGFATAL, "%s is a system dll and should never be removed", pDll->GetName());
    }
    else
    {
      // remove from the list
      bool bRemoved = false;
      for (int i = 0; i < m_iNrOfDlls && m_dlls[i]; i++)
      {
        if (m_dlls[i] == pDll) bRemoved = true;
        if (bRemoved && i + 1 < m_iNrOfDlls)
        {
          m_dlls[i] = m_dlls[i + 1];
        }
      }
      if (bRemoved)
      {
        m_iNrOfDlls--;
        m_dlls[m_iNrOfDlls] = NULL;
      }
    }
  }
}

void DllLoaderContainer::UnloadPythonDlls()
{
  // unload all dlls that python24.dll could have loaded
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    char* name = m_dlls[i]->GetName();
    if (strstr(name, ".pyd") != NULL)
    {
      LibraryLoader* pDll = m_dlls[i];
      ReleaseModule(pDll);
      i = 0;
    }
  }

  // last dll to unload, python24.dll
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    char* name = m_dlls[i]->GetName();

#ifdef HAVE_LIBPYTHON2_6
    if (strstr(name, "python26.dll") != NULL)
#else
    if (strstr(name, "python24.dll") != NULL)
#endif
    {
      LibraryLoader* pDll = m_dlls[i];
      pDll->IncrRef();
      while (pDll->DecrRef() > 1) pDll->DecrRef();

      // since we freed all python extension dlls first, we have to remove any associations with them first
      DllTrackInfo* info = tracker_get_dlltrackinfo_byobject((DllLoader*) pDll);
      if (info != NULL)
      {
        info->dllList.clear();
      }

      ReleaseModule(pDll);
      break;
    }
  }


}
