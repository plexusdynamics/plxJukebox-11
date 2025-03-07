#pragma once


#include "DirectoryNode.h"

namespace XFILE
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeCountry : public CDirectoryNode
    {
    public:
      CDirectoryNodeCountry(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType() const;
      virtual bool GetContent(CFileItemList& items) const;
      virtual CStdString GetLocalizedName() const;
    };
  }
}


