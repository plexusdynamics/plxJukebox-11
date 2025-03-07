

#include "PasswordManager.h"
#include "settings/GUIDialogLockSettings.h"
#include "URL.h"
#include "settings/Settings.h"
#include "utils/XMLUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "filesystem/File.h"

using namespace std;

CPasswordManager &CPasswordManager::GetInstance()
{
  static CPasswordManager sPasswordManager;
  return sPasswordManager;
}

CPasswordManager::CPasswordManager()
{
  m_loaded = false;
}

bool CPasswordManager::AuthenticateURL(CURL &url)
{
  CSingleLock lock(m_critSection);

  if (!m_loaded)
    Load();
  CStdString lookup(GetLookupPath(url));
  map<CStdString, CStdString>::const_iterator it = m_temporaryCache.find(lookup);
  if (it == m_temporaryCache.end())
  { // second step, try something that doesn't quite match
    it = m_temporaryCache.find(GetServerLookup(lookup));
  }
  if (it != m_temporaryCache.end())
  {
    CURL auth(it->second);
    url.SetPassword(auth.GetPassWord());
    url.SetUserName(auth.GetUserName());
    return true;
  }
  return false;
}

bool CPasswordManager::PromptToAuthenticateURL(CURL &url)
{
  CSingleLock lock(m_critSection);

  CStdString passcode;
  CStdString username = url.GetUserName();
  CStdString path = GetLookupPath(url);

  bool saveDetails = false;
  if (!CGUIDialogLockSettings::ShowAndGetUserAndPassword(username, passcode, url.GetWithoutUserDetails(), &saveDetails))
    return false;

  url.SetPassword(passcode);
  url.SetUserName(username);

  // save the information for later
  CStdString authenticatedPath = url.Get();

  if (!m_loaded)
    Load();

  if (saveDetails)
  { // write to some random XML file...
    m_permanentCache[path] = authenticatedPath;
    Save();
  }

  // save for both this path and more generally the server as a whole.
  m_temporaryCache[path] = authenticatedPath;
  m_temporaryCache[GetServerLookup(path)] = authenticatedPath;
  return true;
}

void CPasswordManager::Clear()
{
  m_temporaryCache.clear();
  m_permanentCache.clear();
  m_loaded = false;
}

void CPasswordManager::Load()
{
  Clear();
  CStdString passwordsFile = g_settings.GetUserDataItem("passwords.xml");
  if (XFILE::CFile::Exists(passwordsFile))
  {
    TiXmlDocument doc;
    if (!doc.LoadFile(passwordsFile))
    {
      CLog::Log(LOGERROR, "%s - Unable to load: %s, Line %d\n%s", 
        __FUNCTION__, passwordsFile.c_str(), doc.ErrorRow(), doc.ErrorDesc());
      return;
    }
    const TiXmlElement *root = doc.RootElement();
    if (root->ValueStr() != "passwords")
      return;
    // read in our passwords
    const TiXmlElement *path = root->FirstChildElement("path");
    while (path)
    {
      CStdString from, to;
      if (XMLUtils::GetPath(path, "from", from) && XMLUtils::GetPath(path, "to", to))
      {
        m_permanentCache[from] = to;
        m_temporaryCache[from] = to;
        m_temporaryCache[GetServerLookup(from)] = to;
      }
      path = path->NextSiblingElement("path");
    }
  }
  m_loaded = true;
}

void CPasswordManager::Save() const
{
  if (!m_permanentCache.size())
    return;

  TiXmlDocument doc;
  TiXmlElement rootElement("passwords");
  TiXmlNode *root = doc.InsertEndChild(rootElement);
  if (!root)
    return;

  for (map<CStdString, CStdString>::const_iterator i = m_permanentCache.begin(); i != m_permanentCache.end(); ++i)
  {
    TiXmlElement pathElement("path");
    TiXmlNode *path = root->InsertEndChild(pathElement);
    XMLUtils::SetPath(path, "from", i->first);
    XMLUtils::SetPath(path, "to", i->second);
  }

  doc.SaveFile(g_settings.GetUserDataItem("passwords.xml"));
}

CStdString CPasswordManager::GetLookupPath(const CURL &url) const
{
  return "smb://" + url.GetHostName() + "/" + url.GetShareName();
}

CStdString CPasswordManager::GetServerLookup(const CStdString &path) const
{
  CURL url(path);
  return "smb://" + url.GetHostName() + "/";
}
