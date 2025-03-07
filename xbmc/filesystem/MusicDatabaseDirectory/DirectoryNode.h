#pragma once



#include "utils/StdString.h"

class CFileItemList;

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CQueryParams;

    typedef enum _NODE_TYPE
    {
      NODE_TYPE_NONE=0,
      NODE_TYPE_ROOT,
      NODE_TYPE_OVERVIEW,
      NODE_TYPE_TOP100,
      NODE_TYPE_GENRE,
      NODE_TYPE_ARTIST,
      NODE_TYPE_ALBUM,
      NODE_TYPE_ALBUM_RECENTLY_ADDED,
      NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS,
      NODE_TYPE_ALBUM_RECENTLY_PLAYED,
      NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS,
      NODE_TYPE_ALBUM_TOP100,
      NODE_TYPE_ALBUM_TOP100_SONGS,
      NODE_TYPE_ALBUM_COMPILATIONS,
      NODE_TYPE_ALBUM_COMPILATIONS_SONGS,
      NODE_TYPE_SONG,
      NODE_TYPE_SONG_TOP100,
      NODE_TYPE_YEAR,
      NODE_TYPE_YEAR_ALBUM,
      NODE_TYPE_YEAR_SONG,
      NODE_TYPE_SINGLES
    } NODE_TYPE;

    typedef struct {
      NODE_TYPE node;
      int       id;
      int       label;
    } Node;

    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const CStdString& strPath);
      static void GetDatabaseInfo(const CStdString& strPath, CQueryParams& params);
      virtual ~CDirectoryNode();

      NODE_TYPE GetType() const;

      bool GetChilds(CFileItemList& items);
      virtual NODE_TYPE GetChildType() const;
      virtual CStdString GetLocalizedName() const;

      CDirectoryNode* GetParent() const;
      bool CanCache() const;

    protected:
      CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);

      void CollectQueryParams(CQueryParams& params) const;

      const CStdString& GetName() const;
      int GetID() const;
      void RemoveParent();

      virtual bool GetContent(CFileItemList& items) const;

      CStdString BuildPath() const;

    private:
      void AddQueuingFolder(CFileItemList& items) const;

    private:
      NODE_TYPE m_Type;
      CStdString m_strName;
      CDirectoryNode* m_pParent;
    };
  }
}


