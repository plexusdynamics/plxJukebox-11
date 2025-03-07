

#include "URL.h"
#include "PlayerSelectionRule.h"
#include "video/VideoInfoTag.h"
#include "utils/StreamDetails.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "tinyXML/tinyxml.h"

CPlayerSelectionRule::CPlayerSelectionRule(TiXmlElement* pRule)
{
  Initialize(pRule);
}

CPlayerSelectionRule::~CPlayerSelectionRule()
{}

void CPlayerSelectionRule::Initialize(TiXmlElement* pRule)
{
  m_name = pRule->Attribute("name");
  if (!m_name || m_name.IsEmpty())
    m_name = "un-named";

  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::Initialize: creating rule: %s", m_name.c_str());

  m_tInternetStream = GetTristate(pRule->Attribute("internetstream"));
  m_tAudio = GetTristate(pRule->Attribute("audio"));
  m_tVideo = GetTristate(pRule->Attribute("video"));

  m_tDVD = GetTristate(pRule->Attribute("dvd"));
  m_tDVDFile = GetTristate(pRule->Attribute("dvdfile"));
  m_tDVDImage = GetTristate(pRule->Attribute("dvdimage"));

  m_protocols = pRule->Attribute("protocols");
  m_fileTypes = pRule->Attribute("filetypes");
  m_mimeTypes = pRule->Attribute("mimetypes");
  m_fileName = pRule->Attribute("filename");

  m_audioCodec = pRule->Attribute("audiocodec");
  m_audioChannels = pRule->Attribute("audiochannels");
  m_videoCodec = pRule->Attribute("videocodec");
  m_videoResolution = pRule->Attribute("videoresolution");
  m_videoAspect = pRule->Attribute("videoaspect");

  m_bStreamDetails = m_audioCodec.length() > 0 || m_audioChannels.length() > 0 ||
    m_videoCodec.length() > 0 || m_videoResolution.length() > 0 || m_videoAspect.length() > 0;

  if (m_bStreamDetails && !g_guiSettings.GetBool("myvideos.extractflags"))
  {
      CLog::Log(LOGWARNING, "CPlayerSelectionRule::Initialize: rule: %s needs media flagging, which is disabled", m_name.c_str());
  }

  m_playerName = pRule->Attribute("player");
  m_playerCoreId = 0;

  TiXmlElement* pSubRule = pRule->FirstChildElement("rule");
  while (pSubRule)
  {
    vecSubRules.push_back(new CPlayerSelectionRule(pSubRule));
    pSubRule = pSubRule->NextSiblingElement("rule");
  }
}

int CPlayerSelectionRule::GetTristate(const char* szValue) const
{
  if (szValue)
  {
    if (stricmp(szValue, "true") == 0) return 1;
    if (stricmp(szValue, "false") == 0) return 0;
  }
  return -1;
}

bool CPlayerSelectionRule::CompileRegExp(const CStdString& str, CRegExp& regExp) const
{
  return str.length() > 0 && regExp.RegComp(str.c_str());
}

bool CPlayerSelectionRule::MatchesRegExp(const CStdString& str, CRegExp& regExp) const
{
  return regExp.RegFind(str, 0) == 0;
}

void CPlayerSelectionRule::GetPlayers(const CFileItem& item, VECPLAYERCORES &vecCores)
{
  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: considering rule: %s", m_name.c_str());

  if (m_bStreamDetails && !item.HasVideoInfoTag()) return;
  if (m_tAudio >= 0 && (m_tAudio > 0) != item.IsAudio()) return;
  if (m_tVideo >= 0 && (m_tVideo > 0) != item.IsVideo()) return;
  if (m_tInternetStream >= 0 && (m_tInternetStream > 0) != item.IsInternetStream()) return;

  if (m_tDVD >= 0 && (m_tDVD > 0) != item.IsDVD()) return;
  if (m_tDVDFile >= 0 && (m_tDVDFile > 0) != item.IsDVDFile()) return;
  if (m_tDVDImage >= 0 && (m_tDVDImage > 0) != item.IsDVDImage()) return;

  CRegExp regExp;

  if (m_bStreamDetails)
  {
    if (!item.GetVideoInfoTag()->HasStreamDetails())
    {
      CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: cannot check rule: %s, no StreamDetails", m_name.c_str());
      return;
    }

    CStreamDetails streamDetails = item.GetVideoInfoTag()->m_streamDetails;

    if (CompileRegExp(m_audioCodec, regExp) && !MatchesRegExp(streamDetails.GetAudioCodec(), regExp)) return;

    if (CompileRegExp(m_videoCodec, regExp) && !MatchesRegExp(streamDetails.GetVideoCodec(), regExp)) return;

    if (CompileRegExp(m_videoResolution, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoDimsToResolutionDescription(streamDetails.GetVideoWidth(), streamDetails.GetVideoHeight()), regExp)) return;

    if (CompileRegExp(m_videoAspect, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoAspectToAspectDescription(streamDetails.GetVideoAspect()),  regExp)) return;
  }

  CURL url(item.GetPath());

  if (CompileRegExp(m_fileTypes, regExp) && !MatchesRegExp(url.GetFileType(), regExp)) return;

  if (CompileRegExp(m_protocols, regExp) && !MatchesRegExp(url.GetProtocol(), regExp)) return;

  if (CompileRegExp(m_mimeTypes, regExp) && !MatchesRegExp(item.GetMimeType(), regExp)) return;

  if (CompileRegExp(m_fileName, regExp) && !MatchesRegExp(item.GetPath(), regExp)) return;

  CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: matches rule: %s", m_name.c_str());

  for (unsigned int i = 0; i < vecSubRules.size(); i++)
    vecSubRules[i]->GetPlayers(item, vecCores);

  PLAYERCOREID playerCoreId = GetPlayerCore();
  if (playerCoreId != EPC_NONE)
  {
    CLog::Log(LOGDEBUG, "CPlayerSelectionRule::GetPlayers: adding player: %s (%d) for rule: %s", m_playerName.c_str(), playerCoreId, m_name.c_str());
    vecCores.push_back(GetPlayerCore());
  }
}

PLAYERCOREID CPlayerSelectionRule::GetPlayerCore()
{
  if (!m_playerCoreId)
  {
    m_playerCoreId = CPlayerCoreFactory::GetPlayerCore(m_playerName);
  }
  return m_playerCoreId;
}

