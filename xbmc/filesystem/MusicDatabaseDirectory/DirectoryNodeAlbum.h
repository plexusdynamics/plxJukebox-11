#pragma once


#include "DirectoryNode.h"

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeAlbum : public CDirectoryNode
    {
    public:
      CDirectoryNodeAlbum(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType() const;
      virtual bool GetContent(CFileItemList& items) const;
      virtual CStdString GetLocalizedName() const;
    };
  }
}


