

#include "Repository.h"
#include "tinyXML/tinyxml.h"
#include "filesystem/File.h"
#include "AddonDatabase.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "utils/JobManager.h"
#include "addons/AddonInstaller.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKaiToast.h"

using namespace XFILE;
using namespace ADDON;

AddonPtr CRepository::Clone(const AddonPtr &self) const
{
  CRepository* result = new CRepository(*this, self);
  result->m_info = m_info;
  result->m_checksum = m_checksum;
  result->m_datadir = m_datadir;
  result->m_compressed = m_compressed;
  result->m_zipped = m_zipped;
  return AddonPtr(result);
}

CRepository::CRepository(const AddonProps& props) :
  CAddon(props)
{
  m_compressed = false;
  m_zipped = false;
}

CRepository::CRepository(const cp_extension_t *ext)
  : CAddon(ext)
{
  m_compressed = false;
  m_zipped = false;
  // read in the other props that we need
  if (ext)
  {
    m_checksum = CAddonMgr::Get().GetExtValue(ext->configuration, "checksum");
    m_compressed = CAddonMgr::Get().GetExtValue(ext->configuration, "info@compressed").Equals("true");
    m_info = CAddonMgr::Get().GetExtValue(ext->configuration, "info");
    m_datadir = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir");
    m_zipped = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir@zip").Equals("true");
    m_hashes = CAddonMgr::Get().GetExtValue(ext->configuration, "hashes").Equals("true");
  }
}

CRepository::CRepository(const CRepository &rhs, const AddonPtr &self)
  : CAddon(rhs, self)
{
}

CRepository::~CRepository()
{
}

CStdString CRepository::Checksum()
{
  if (!m_checksum.IsEmpty())
    return FetchChecksum(m_checksum);
  return "";
}

CStdString CRepository::FetchChecksum(const CStdString& url)
{
  CSingleLock lock(m_critSection);
  CFile file;
  file.Open(url);
  CStdString checksum;
  try
  {
    char* temp = new char[(size_t)file.GetLength()+1];
    file.Read(temp,file.GetLength());
    temp[file.GetLength()] = 0;
    checksum = temp;
    delete[] temp;
  }
  catch (...)
  {
  }
  return checksum;
}

CStdString CRepository::GetAddonHash(const AddonPtr& addon)
{
  CStdString checksum;
  if (m_hashes)
  {
    checksum = FetchChecksum(addon->Path()+".md5");
    size_t pos = checksum.find_first_of(" \n");
    if (pos != CStdString::npos)
      return checksum.Left(pos);
  }
  return checksum;
}

#define SET_IF_NOT_EMPTY(x,y) \
  { \
    if (!x.IsEmpty()) \
       x = y; \
  }

VECADDONS CRepository::Parse()
{
  CSingleLock lock(m_critSection);

  VECADDONS result;
  TiXmlDocument doc;

  CStdString file = m_info;
  if (m_compressed)
  {
    CURL url(m_info);
    CStdString opts = url.GetProtocolOptions();
    if (!opts.IsEmpty())
      opts += "&";
    url.SetProtocolOptions(opts+"Encoding=gzip");
    file = url.Get();
  }

  doc.LoadFile(file);
  if (doc.RootElement())
  {
    CAddonMgr::Get().AddonsFromRepoXML(doc.RootElement(), result);
    for (IVECADDONS i = result.begin(); i != result.end(); ++i)
    {
      AddonPtr addon = *i;
      if (m_zipped)
      {
        CStdString file;
        file.Format("%s/%s-%s.zip", addon->ID().c_str(), addon->ID().c_str(), addon->Version().c_str());
        addon->Props().path = URIUtils::AddFileToFolder(m_datadir,file);
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/icon.png"))
        file.Format("%s/changelog-%s.txt", addon->ID().c_str(), addon->Version().c_str());
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(m_datadir,file))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/fanart.jpg"))
      }
      else
      {
        addon->Props().path = URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/");
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/icon.png"))
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/changelog.txt"))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/fanart.jpg"))
      }
    }
  }

  return result;
}

CRepositoryUpdateJob::CRepositoryUpdateJob(const VECADDONS &repos)
  : m_repos(repos)
{
}

bool CRepositoryUpdateJob::DoWork()
{
  VECADDONS addons;
  for (VECADDONS::const_iterator i = m_repos.begin(); i != m_repos.end(); ++i)
  {
    RepositoryPtr repo = boost::dynamic_pointer_cast<CRepository>(*i);
    VECADDONS newAddons = GrabAddons(repo);
    addons.insert(addons.end(), newAddons.begin(), newAddons.end());
  }
  if (addons.empty())
    return false;

  // check for updates
  CAddonDatabase database;
  database.Open();
  for (unsigned int i=0;i<addons.size();++i)
  {
    // manager told us to feck off
    if (ShouldCancel(0,0))
      break;
    if (!CAddonInstaller::Get().CheckDependencies(addons[i]))
      addons[i]->Props().broken = g_localizeStrings.Get(24044);

    AddonPtr addon;
    CAddonMgr::Get().GetAddon(addons[i]->ID(),addon);
    if (addon && addons[i]->Version() > addon->Version() &&
        !database.IsAddonBlacklisted(addons[i]->ID(),addons[i]->Version().c_str()))
    {
      if (g_settings.m_bAddonAutoUpdate || addon->Type() >= ADDON_VIZ_LIBRARY)
      {
        CStdString referer;
        if (URIUtils::IsInternetStream(addons[i]->Path()))
          referer.Format("Referer=%s-%s.zip",addon->ID().c_str(),addon->Version().c_str());

        CAddonInstaller::Get().Install(addon->ID(), true, referer);
      }
      else if (g_settings.m_bAddonNotifications)
      {
        CGUIDialogKaiToast::QueueNotification(addon->Icon(),
                                              g_localizeStrings.Get(24061),
                                              addon->Name(),TOAST_DISPLAY_TIME,false,TOAST_DISPLAY_TIME);
      }
    }
    if (!addons[i]->Props().broken.IsEmpty())
    {
      if (database.IsAddonBroken(addons[i]->ID()).IsEmpty())
      {
        if (addon && CGUIDialogYesNo::ShowAndGetInput(addons[i]->Name(),
                                             g_localizeStrings.Get(24096),
                                             g_localizeStrings.Get(24097),
                                             ""))
          database.DisableAddon(addons[i]->ID());
      }
    }
    database.BreakAddon(addons[i]->ID(), addons[i]->Props().broken);
  }

  return true;
}

VECADDONS CRepositoryUpdateJob::GrabAddons(RepositoryPtr& repo)
{
  CAddonDatabase database;
  database.Open();
  CStdString checksum;
  int idRepo = database.GetRepoChecksum(repo->ID(),checksum);
  CStdString reposum = repo->Checksum();
  VECADDONS addons;
  if (idRepo == -1 || !checksum.Equals(reposum))
  {
    addons = repo->Parse();
    if (!addons.empty())
      database.AddRepository(repo->ID(),addons,reposum);
    else
      CLog::Log(LOGERROR,"Repository %s returned no add-ons, listing may have failed",repo->Name().c_str());
  }
  else
    database.GetRepository(repo->ID(),addons);
  database.SetRepoTimestamp(repo->ID(),CDateTime::GetCurrentDateTime().GetAsDBDateTime());

  return addons;
}

