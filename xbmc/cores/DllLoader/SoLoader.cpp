

#include <dlfcn.h>
#include "SoLoader.h"
#include "utils/StdString.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"

SoLoader::SoLoader(const char *so, bool bGlobal) : LibraryLoader(so)
{
  m_soHandle = NULL;
  m_bGlobal = bGlobal;
  m_bLoaded = false;
}

SoLoader::~SoLoader()
{
  if (m_bLoaded)
    Unload();
}

bool SoLoader::Load()
{
  if (m_soHandle != NULL)
    return true;

  CStdString strFileName= _P(GetFileName());
  int flags = RTLD_LAZY;
  if (m_bGlobal) flags |= RTLD_GLOBAL;
  if (strFileName == "xbmc.so")
  {
    CLog::Log(LOGDEBUG, "Loading Internal Library\n");
    m_soHandle = RTLD_DEFAULT;
  }
  else
  {
    CLog::Log(LOGDEBUG, "Loading: %s\n", strFileName.c_str());
    m_soHandle = dlopen(strFileName.c_str(), flags);
    if (!m_soHandle)
    {
      CLog::Log(LOGERROR, "Unable to load %s, reason: %s", strFileName.c_str(), dlerror());
      return false;
    }
  }
  m_bLoaded = true;
  return true;
}

void SoLoader::Unload()
{
  CLog::Log(LOGDEBUG, "Unloading: %s\n", GetName());

  if (m_soHandle)
  {
    if (dlclose(m_soHandle) != 0)
       CLog::Log(LOGERROR, "Unable to unload %s, reason: %s", GetName(), dlerror());
  }
  m_bLoaded = false;
  m_soHandle = NULL;
}

int SoLoader::ResolveExport(const char* symbol, void** f, bool logging)
{
  if (!m_bLoaded && !Load())
  {
    if (logging)
      CLog::Log(LOGWARNING, "Unable to resolve: %s %s, reason: so not loaded", GetName(), symbol);
    return 0;
  }

  void* s = dlsym(m_soHandle, symbol);
  if (!s)
  {
    if (logging)
      CLog::Log(LOGWARNING, "Unable to resolve: %s %s, reason: %s", GetName(), symbol, dlerror());
    return 0;
  }

  *f = s;
  return 1;
}

bool SoLoader::IsSystemDll()
{
  return false;
}

HMODULE SoLoader::GetHModule()
{
  return m_soHandle;
}

bool SoLoader::HasSymbols()
{
  return false;
}
