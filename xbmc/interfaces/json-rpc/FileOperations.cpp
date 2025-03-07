

#include "FileOperations.h"
#include "VideoLibrary.h"
#include "AudioLibrary.h"
#include "settings/Settings.h"
#include "MediaSource.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "Util.h"
#include "URL.h"

using namespace XFILE;
using namespace JSONRPC;

static const unsigned int SourcesSize = 5;
static CStdString SourceNames[] = { "programs", "files", "video", "music", "pictures" };

JSON_STATUS CFileOperations::GetRootDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString media = parameterObject["media"].asString();
  media = media.ToLower();

  VECSOURCES *sources = g_settings.GetSourcesFromType(media);
  if (sources)
  {
    CFileItemList items;
    for (unsigned int i = 0; i < (unsigned int)sources->size(); i++)
    {
      // Do not show sources which are locked
      if (sources->at(i).m_iHasLock == 2)
        continue;

      items.Add(CFileItemPtr(new CFileItem(sources->at(i))));
    }

    for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    {
      if (items[i]->IsSmb())
      {
        CURL url(items[i]->GetPath());
        items[i]->SetPath(url.GetWithoutUserDetails());
      }
    }

    CVariant param(CVariant::VariantTypeObject);
    param["properties"] = CVariant(CVariant::VariantTypeArray);
    param["properties"].append("file");

    HandleFileItemList(NULL, true, "sources", items, param, result);
  }

  return OK;
}

JSON_STATUS CFileOperations::GetDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString media = parameterObject["media"].asString();
  media = media.ToLower();

  CDirectory directory;
  CFileItemList items;
  CStdString strPath = parameterObject["directory"].asString();

  // Check if this directory is part of a source and whether it's locked
  VECSOURCES *sources;
  bool isSource;
  for (unsigned int index = 0; index < SourcesSize; index++)
  {
    sources = g_settings.GetSourcesFromType(SourceNames[index]);
    int sourceIndex = CUtil::GetMatchingSource(strPath, *sources, isSource);
    if (sourceIndex >= 0 && sourceIndex < (int)sources->size() && sources->at(sourceIndex).m_iHasLock == 2)
      return InvalidParams;
  }

  CStdStringArray regexps;
  CStdString extensions = "";
  if (media.Equals("video"))
  {
    regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
    extensions = g_settings.m_videoExtensions;
  }
  else if (media.Equals("music"))
  {
    regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
    extensions = g_settings.m_musicExtensions;
  }
  else if (media.Equals("pictures"))
  {
    regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;
    extensions = g_settings.m_pictureExtensions;
  }

  if (directory.GetDirectory(strPath, items, extensions))
  {
    CFileItemList filteredDirectories, filteredFiles;
    for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    {
      if (CUtil::ExcludeFileOrFolder(items[i]->GetPath(), regexps))
        continue;

      if (items[i]->IsSmb())
      {
        CURL url(items[i]->GetPath());
        items[i]->SetPath(url.GetWithoutUserDetails());
      }

      if ((media == "video" && items[i]->HasVideoInfoTag()) ||
          (media == "music" && items[i]->HasMusicInfoTag()))
      {
        if (items[i]->m_bIsFolder)
          filteredDirectories.Add(items[i]);
        else 
          filteredFiles.Add(items[i]);
      }
      else
      {
        CFileItem fileItem;
        if (FillFileItem(items[i], fileItem, media))
        {
          if (items[i]->m_bIsFolder)
            filteredDirectories.Add(CFileItemPtr(new CFileItem(fileItem)));
          else
            filteredFiles.Add(CFileItemPtr(new CFileItem(fileItem)));
        }
        else
        {
          if (items[i]->m_bIsFolder)
            filteredDirectories.Add(items[i]);
          else
            filteredFiles.Add(items[i]);
        }
      }
    }

    // Check if the "properties" list exists
    // and make sure it contains the "file"
    // field
    CVariant param = parameterObject;
    if (!param.isMember("properties"))
      param["properties"] = CVariant(CVariant::VariantTypeArray);

    bool hasFileField = false;
    for (CVariant::const_iterator_array itr = param["properties"].begin_array(); itr != param["properties"].end_array(); itr++)
    {
      if (*itr == CVariant("file"))
      {
        hasFileField = true;
        break;
      }
    }

    if (!hasFileField)
      param["properties"].append("file");

    HandleFileItemList("id", true, "files", filteredDirectories, param, result);
    for (unsigned int index = 0; index < result["files"].size(); index++)
    {
      result["files"][index]["filetype"] = "directory";
    }
    int count = (int)result["limits"]["total"].asInteger();

    HandleFileItemList("id", true, "files", filteredFiles, param, result);
    for (unsigned int index = count; index < result["files"].size(); index++)
    {
      result["files"][index]["filetype"] = "file";
    }
    count += (int)result["limits"]["total"].asInteger();

    result["limits"]["end"] = count;
    result["limits"]["total"] = count;

    return OK;
  }

  return InvalidParams;
}

JSON_STATUS CFileOperations::PrepareDownload(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string protocol;
  if (transport->PrepareDownload(parameterObject["path"].asString().c_str(), result["details"], protocol))
  {
    result["protocol"] = protocol;

    if ((transport->GetCapabilities() & FileDownloadDirect) == FileDownloadDirect)
      result["mode"] = "direct";
    else
      result["mode"] = "redirect";

    return OK;
  }
  
  return InvalidParams;
}

JSON_STATUS CFileOperations::Download(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return transport->Download(parameterObject["path"].asString().c_str(), result) ? OK : InvalidParams;
}

bool CFileOperations::FillFileItem(const CFileItemPtr &originalItem, CFileItem &item, CStdString media /* = "" */)
{
  if (originalItem.get() == NULL)
    return false;

  bool status = false;
  CStdString strFilename = originalItem->GetPath();
  if (!strFilename.empty() && (CDirectory::Exists(strFilename) || CFile::Exists(strFilename)))
  {
    if (media.Equals("video"))
      status = CVideoLibrary::FillFileItem(strFilename, item);
    else if (media.Equals("music"))
      status = CAudioLibrary::FillFileItem(strFilename, item);

    if (!status && originalItem->GetLabel().empty())
    {
      bool isDir = CDirectory::Exists(strFilename);
      CStdString label = CUtil::GetTitleFromPath(strFilename, isDir);
      if (!label.empty())
      {
        item = CFileItem(strFilename, isDir);
        item.SetLabel(label);

        status = true;
      }
    }
  }

  return status;
}

bool CFileOperations::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  if (parameterObject.isMember("directory"))
  {
    CStdString media =  parameterObject["media"].asString();
    media = media.ToLower();

    CStdString strPath = parameterObject["directory"].asString();
    if (!strPath.empty())
    {
      CFileItemList items;
      CStdString extensions = "";
      CStdStringArray regexps;

      if (media.Equals("video"))
      {
        regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
        extensions = g_settings.m_videoExtensions;
      }
      else if (media.Equals("music"))
      {
        regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
        extensions = g_settings.m_musicExtensions;
      }
      else if (media.Equals("pictures"))
      {
        regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;
        extensions = g_settings.m_pictureExtensions;
      }

      CDirectory directory;
      if (directory.GetDirectory(strPath, items, extensions))
      {
        items.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);
        CFileItemList filteredDirectories;
        for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
        {
          if (CUtil::ExcludeFileOrFolder(items[i]->GetPath(), regexps))
            continue;

          if (items[i]->m_bIsFolder)
            filteredDirectories.Add(items[i]);
          else if ((media == "video" && items[i]->HasVideoInfoTag()) ||
                   (media == "music" && items[i]->HasMusicInfoTag()))
            list.Add(items[i]);
          else
          {
            CFileItem fileItem;
            if (FillFileItem(items[i], fileItem, media))
              list.Add(CFileItemPtr(new CFileItem(fileItem)));
            else if (media == "files")
              list.Add(items[i]);
          }
        }

        if (parameterObject.isMember("recursive") && parameterObject["recursive"].isBoolean())
        {
          for (int i = 0; i < filteredDirectories.Size(); i++)
          {
            CVariant val = parameterObject;
            val["directory"] = filteredDirectories[i]->GetPath();
            FillFileItemList(val, list);
          }
        }

        return true;
      }
    }
  }

  return false;
}
