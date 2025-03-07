#pragma once


#include "DirectoryNode.h"

namespace XFILE
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeTitleMusicVideos : public CDirectoryNode
    {
    public:
      CDirectoryNodeTitleMusicVideos(const CStdString& strEntryName, CDirectoryNode* pParent);
    protected:
      virtual bool GetContent(CFileItemList& item) const;
    };
  }
}
