

#include "ZeroconfDirectory.h"
#include <stdexcept>

#include "URL.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "network/ZeroconfBrowser.h"
#include "Directory.h"
#include "utils/log.h"

using namespace XFILE;

CZeroconfDirectory::CZeroconfDirectory()
{
  CZeroconfBrowser::GetInstance()->Start();
}

CZeroconfDirectory::~CZeroconfDirectory()
{
}

namespace
{
  CStdString GetHumanReadableProtocol(std::string const& fcr_service_type)
  {
    if(fcr_service_type == "_smb._tcp.")
      return "SAMBA";
    else if(fcr_service_type == "_ftp._tcp.")
      return "FTP";
    else if(fcr_service_type == "_htsp._tcp.")
      return "HTS";
    else if(fcr_service_type == "_daap._tcp.")
      return "iTunes Music Sharing";
    else if(fcr_service_type == "_webdav._tcp.")
      return "WebDAV";   
    else if(fcr_service_type == "_nfs._tcp.")
      return "NFS";   
    else if(fcr_service_type == "_afpovertcp._tcp.")
      return "AFP";   
    else if(fcr_service_type == "_sftp-ssh._tcp.")
      return "SFTP";
    //fallback, just return the received type
    return fcr_service_type;
  }
  bool GetXBMCProtocol(std::string const& fcr_service_type, CStdString& fr_protocol)
  {
    if(fcr_service_type == "_smb._tcp.")
      fr_protocol = "smb";
    else if(fcr_service_type == "_ftp._tcp.")
      fr_protocol = "ftp";
    else if(fcr_service_type == "_htsp._tcp.")
      fr_protocol = "htsp";
    else if(fcr_service_type == "_daap._tcp.")
      fr_protocol = "daap";
    else if(fcr_service_type == "_webdav._tcp.")
      fr_protocol = "dav";
    else if(fcr_service_type == "_nfs._tcp.")
      fr_protocol = "nfs";      
    else if(fcr_service_type == "_afpovertcp._tcp.")
      fr_protocol = "afp";      
    else if(fcr_service_type == "_sftp-ssh._tcp.")
      fr_protocol = "sftp";
    else
      return false;
    return true;
  }
}

bool GetDirectoryFromTxtRecords(CZeroconfBrowser::ZeroconfService zeroconf_service, CURL& url, CFileItemList &items)
{
  bool ret = false;

  //get the txt-records from this service
  CZeroconfBrowser::ZeroconfService::tTxtRecordMap txtRecords=zeroconf_service.GetTxtRecords();

  //if we have some records
  if(!txtRecords.empty())
  {
    CStdString path;
    CStdString username;
    CStdString password;
  
    //search for a path key entry
    CZeroconfBrowser::ZeroconfService::tTxtRecordMap::iterator it = txtRecords.find(TXT_RECORD_PATH_KEY);

    //if we found the key - be sure there is a value there
    if( it != txtRecords.end() && !it->second.empty() )
    {
      //from now on we treat the value as a path - everything else would mean
      //a missconfigured zeroconf server.
      path=it->second;
    }
    
    //search for a username key entry
    it = txtRecords.find(TXT_RECORD_USERNAME_KEY);

    //if we found the key - be sure there is a value there
    if( it != txtRecords.end() && !it->second.empty() )
    {
      username=it->second;
      url.SetUserName(username);
    }
    
    //search for a password key entry
    it = txtRecords.find(TXT_RECORD_PASSWORD_KEY);

    //if we found the key - be sure there is a value there
    if( it != txtRecords.end() && !it->second.empty() )
    {
      password=it->second;
      url.SetPassword(password);
    }
    
    //if we got a path - add a item - else at least we maybe have set username and password to theurl
    if( !path.empty())
    {
      CFileItemPtr item(new CFileItem("", true));
      CStdString urlStr(url.Get());
      //if path has a leading slash (sure it should have one)
      if( path.at(0) == '/' )
      {
        URIUtils::RemoveSlashAtEnd(urlStr);//we don't need the slash at and of url then
      }
      else//path doesn't start with slash - 
      {//this is some kind of missconfiguration - we fix it by adding a slash to the url
        URIUtils::AddSlashAtEnd(urlStr);
      }
      
      //add slash at end of path since it has to be a folder
      URIUtils::AddSlashAtEnd(path);
      //this is the full path includeing remote stuff (e.x. nfs://ip/path
      item->SetPath(urlStr + path);
      //remove the slash at the end of the path or GetFileName will not give the last dir
      URIUtils::RemoveSlashAtEnd(path);
      //set the label to the last directory in path
      if( !URIUtils::GetFileName(path).empty() )
        item->SetLabel(URIUtils::GetFileName(path));
      else
        item->SetLabel("/");

      item->SetLabelPreformated(true);
      //just set the default folder icon
      item->FillInDefaultIcon();
      item->m_bIsShareOrDrive=true;
      items.Add(item);
      ret = true;
    }
  }
  return ret;
}

bool CZeroconfDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  assert(strPath.substr(0, 11) == "zeroconf://");
  CStdString path = strPath.substr(11, strPath.length());
  URIUtils::RemoveSlashAtEnd(path);
  if(path.empty())
  {
    std::vector<CZeroconfBrowser::ZeroconfService> found_services = CZeroconfBrowser::GetInstance()->GetFoundServices();
    for(std::vector<CZeroconfBrowser::ZeroconfService>::iterator it = found_services.begin(); it != found_services.end(); ++it)
    {
      //only use discovered services we can connect to through directory
      CStdString tmp;
      if(GetXBMCProtocol(it->GetType(), tmp))
      {
        CFileItemPtr item(new CFileItem("", true));
        CURL url;
        url.SetProtocol("zeroconf");
        CStdString service_path = CZeroconfBrowser::ZeroconfService::toPath(*it);
        CURL::Encode(service_path);
        url.SetFileName(service_path);
        item->SetPath(url.Get());

        //now do the formatting
        CStdString protocol = GetHumanReadableProtocol(it->GetType());
        item->SetLabel(it->GetName() + " (" + protocol  + ")");
        item->SetLabelPreformated(true);
        //just set the default folder icon
        item->FillInDefaultIcon();
        items.Add(item);
      }
    }
    return true;
  } 
  else
  {
    //decode the path first
    CStdString decoded = path;
    CURL::Decode(decoded);
    try
    {
      CZeroconfBrowser::ZeroconfService zeroconf_service = CZeroconfBrowser::ZeroconfService::fromPath(decoded);

      if(!CZeroconfBrowser::GetInstance()->ResolveService(zeroconf_service))
      {
        CLog::Log(LOGINFO, "CZeroconfDirectory::GetDirectory service ( %s ) could not be resolved in time", zeroconf_service.GetName().c_str());
        return false;
      }
      else
      {
        assert(!zeroconf_service.GetIP().empty());
        CURL service;
        service.SetPort(zeroconf_service.GetPort());
        service.SetHostName(zeroconf_service.GetIP());
        //do protocol conversion (_smb._tcp -> smb)
        //ToDo: try automatic conversion -> remove leading '_' and '._tcp'?
        CStdString protocol;
        if(!GetXBMCProtocol(zeroconf_service.GetType(), protocol))
        {
          CLog::Log(LOGERROR, "CZeroconfDirectory::GetDirectory Unknown service type (%s), skipping; ", zeroconf_service.GetType().c_str());
          return false;
        }
        
        service.SetProtocol(protocol);
        
        //first try to show the txt-record defined path if any
        if(GetDirectoryFromTxtRecords(zeroconf_service, service, items))
        {
          return true;
        }
        else//no txt record path - so let the CDirectory handler show the folders
        {          
          return CDirectory::GetDirectory(service.Get(), items, "", true, true); 
        }
      }
    } catch (std::runtime_error& e) {
      CLog::Log(LOGERROR, "CZeroconfDirectory::GetDirectory failed getting directory: '%s'. Error: '%s'", decoded.c_str(), e.what());
      return false;
    }
  }
}
