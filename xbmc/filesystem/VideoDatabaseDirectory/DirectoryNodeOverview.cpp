

#include "video/VideoDatabase.h"
#include "DirectoryNodeOverview.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;
using namespace std;


Node OverviewChildren[] = {
                            { NODE_TYPE_MOVIES_OVERVIEW,            1, 342 },
                            { NODE_TYPE_TVSHOWS_OVERVIEW,           2, 20343 },
                            { NODE_TYPE_MUSICVIDEOS_OVERVIEW,       3, 20389 },
                            { NODE_TYPE_RECENTLY_ADDED_MOVIES,      4, 20386 },
                            { NODE_TYPE_RECENTLY_ADDED_EPISODES,    5, 20387 },
                            { NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS, 6, 20390 },
                          };

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetID() == OverviewChildren[i].id)
      return OverviewChildren[i].node;

  return NODE_TYPE_NONE;
}

CStdString CDirectoryNodeOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetID() == OverviewChildren[i].id)
      return g_localizeStrings.Get(OverviewChildren[i].label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  CVideoDatabase database;
  database.Open();
  bool hasMovies = database.HasContent(VIDEODB_CONTENT_MOVIES);
  bool hasTvShows = database.HasContent(VIDEODB_CONTENT_TVSHOWS);
  bool hasMusicVideos = database.HasContent(VIDEODB_CONTENT_MUSICVIDEOS);
  vector<pair<const char*, int> > vec;
  if (hasMovies)
  {
    if (g_settings.m_bMyVideoNavFlatten)
      vec.push_back(make_pair("1/2", 342));
    else
      vec.push_back(make_pair("1", 342));   // Movies
  }
  if (hasTvShows)
  {
    if (g_settings.m_bMyVideoNavFlatten)
      vec.push_back(make_pair("2/2", 20343));
    else
      vec.push_back(make_pair("2", 20343)); // TV Shows
  }
  if (hasMusicVideos)
  {
    if (g_settings.m_bMyVideoNavFlatten)
      vec.push_back(make_pair("3/2", 20389));
    else
      vec.push_back(make_pair("3", 20389)); // Music Videos
  }
  if (!g_advancedSettings.m_bVideoLibraryHideRecentlyAddedItems)
  {
    if (hasMovies)
      vec.push_back(make_pair("4", 20386));  // Recently Added Movies
    if (hasTvShows)
      vec.push_back(make_pair("5", 20387)); // Recently Added Episodes
    if (hasMusicVideos)
      vec.push_back(make_pair("6", 20390)); // Recently Added Music Videos
  }
  CStdString path = BuildPath();
  for (unsigned int i = 0; i < vec.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(path + vec[i].first + "/", true));
    pItem->SetLabel(g_localizeStrings.Get(vec[i].second));
    pItem->SetLabelPreformated(true);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
