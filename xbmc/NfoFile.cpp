
// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "NfoFile.h"
#include "video/VideoInfoDownloader.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "Util.h"
#include "FileItem.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "settings/GUISettings.h"
#include "LangInfo.h"
#include "utils/log.h"

#include <vector>

using namespace std;
using namespace XFILE;
using namespace ADDON;

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, ADDON::TYPE type, int episode, const CStdString& strPath2)
{
//  m_info = info; // assume we can use these settings
//  m_type = ScraperTypeFromContent(info->Content());
  m_type = type;
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CFileItemList items;
  bool bNfo=false;

  AddonPtr addon;
  ScraperPtr defaultScraper;
  if (CAddonMgr::Get().GetDefault(m_type, addon))
    defaultScraper = boost::dynamic_pointer_cast<CScraper>(addon);

  if (m_type == ADDON_SCRAPER_ALBUMS)
  {
    CAlbum album;
    bNfo = GetDetails(album);
  }
  else if (m_type == ADDON_SCRAPER_ARTISTS)
  {
    CArtist artist;
    bNfo = GetDetails(artist);
  }
  else if (m_type == ADDON_SCRAPER_TVSHOWS || m_type == ADDON_SCRAPER_MOVIES || m_type == ADDON_SCRAPER_MUSICVIDEOS)
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (episode > -1 && bNfo && m_type == ADDON_SCRAPER_TVSHOWS)
    {
      int infos=0;
      while (m_headofdoc && details.m_iEpisode != episode)
      {
        m_headofdoc = strstr(m_headofdoc+1,"<episodedetails");
        bNfo  = GetDetails(details);
        infos++;
      }
      if (details.m_iEpisode != episode)
      {
        bNfo = false;
        details.Reset();
        m_headofdoc = m_doc;
        if (infos == 1) // still allow differing nfo/file numbers for single ep nfo's
          bNfo = GetDetails(details);
      }
    }
  }

  vector<ScraperPtr> vecScrapers;

  // add selected scraper
  if (m_info)
    vecScrapers.push_back(m_info);

  VECADDONS addons;
  CAddonMgr::Get().GetAddons(m_type,addons);
  // first pass - add language based scrapers
  if (m_info && g_guiSettings.GetBool("scrapers.langfallback"))
    AddScrapers(addons,vecScrapers);

  // add default scraper
  if (defaultScraper && m_info && m_info->ID() != defaultScraper->ID())
    vecScrapers.push_back(defaultScraper);

  // search ..
  int res = -1;
  for (unsigned int i=0;i<vecScrapers.size();++i)
    if ((res = Scrape(vecScrapers[i])) == 0 || res == 2)
      break;

  if (res == 2)
    return ERROR_NFO;
  if (bNfo)
    return m_scurl.m_url.empty() ? FULL_NFO : COMBINED_NFO;
  return m_scurl.m_url.empty() ? NO_NFO : URL_NFO;
}

// return value: 0 - success; 1 - no result; skip; 2 - error
int CNfoFile::Scrape(ScraperPtr& scraper)
{
  if (scraper->Type() != m_type)
    return 1;
  scraper->ClearCache();

  try
  {
    m_scurl = scraper->NfoUrl(m_doc);
  }
  catch (const CScraperError &sce)
  {
    CVideoInfoDownloader::ShowErrorDialog(sce);
    if (!sce.FAborted())
      return 2;
  }

  if (!m_scurl.m_url.empty())
    SetScraperInfo(scraper);
  return m_scurl.m_url.empty() ? 1 : 0;
}

int CNfoFile::Load(const CStdString& strFile)
{
  Close();
  XFILE::CFile file;
  if (file.Open(strFile))
  {
    int size = (int)file.GetLength();
    try
    {
      m_doc = new char[size+1];
      m_headofdoc = m_doc;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s: Exception while creating file buffer",__FUNCTION__);
      return 1;
    }
    if (!m_doc)
    {
      file.Close();
      return 1;
    }
    file.Read(m_doc, size);
    m_doc[size] = 0;
    file.Close();
    return 0;
  }
  return 1;
}

void CNfoFile::Close()
{
  delete m_doc;
  m_doc = NULL;
  m_scurl.Clear();
}

void CNfoFile::AddScrapers(VECADDONS& addons,
                           vector<ScraperPtr>& vecScrapers)
{
  for (unsigned i=0;i<addons.size();++i)
  {
    ScraperPtr scraper = boost::dynamic_pointer_cast<CScraper>(addons[i]);

    // skip if scraper requires settings and there's nothing set yet
    if (scraper->RequiresSettings() && !scraper->HasUserSettings())
      continue;

    // add same language and multi-language
    if (scraper->Language() == m_info->Language() || scraper->Language().Equals("multi"))
      vecScrapers.push_back(scraper);
  }
}
