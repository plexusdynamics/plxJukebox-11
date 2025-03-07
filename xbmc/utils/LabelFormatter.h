#pragma once



#include "StdString.h"
#include <vector>

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

class CFileItem;  // forward

struct LABEL_MASKS
{
  LABEL_MASKS(const CStdString& strLabelFile="", const CStdString& strLabel2File="", const CStdString& strLabelFolder="", const CStdString& strLabel2Folder="")
  {
    m_strLabelFile=strLabelFile;
    m_strLabel2File=strLabel2File;
    m_strLabelFolder=strLabelFolder;
    m_strLabel2Folder=strLabel2Folder;
  }
  CStdString m_strLabelFile;
  CStdString m_strLabel2File;
  CStdString m_strLabelFolder;
  CStdString m_strLabel2Folder;
};

class CLabelFormatter
{
public:
  CLabelFormatter(const CStdString &mask, const CStdString &mask2);

  void FormatLabel(CFileItem *item) const;
  void FormatLabel2(CFileItem *item) const;
  void FormatLabels(CFileItem *item) const // convenient shorthand
  {
    FormatLabel(item);
    FormatLabel2(item);
  }

  bool FillMusicTag(const CStdString &fileName, MUSIC_INFO::CMusicInfoTag *tag) const;

private:
  class CMaskString
  {
  public:
    CMaskString(const CStdString &prefix, char content, const CStdString &postfix)
    {
      m_prefix = prefix; m_content = content; m_postfix = postfix;
    };
    CStdString m_prefix;
    CStdString m_postfix;
    char m_content;
  };

  // functions for assembling the mask vectors
  void AssembleMask(unsigned int label, const CStdString &mask);
  void SplitMask(unsigned int label, const CStdString &mask);

  // functions for retrieving content based on our mask vectors
  CStdString GetContent(unsigned int label, const CFileItem *item) const;
  CStdString GetMaskContent(const CMaskString &mask, const CFileItem *item) const;
  void FillMusicMaskContent(const char mask, const CStdString &value, MUSIC_INFO::CMusicInfoTag *tag) const;

  std::vector<CStdString>   m_staticContent[2];
  std::vector<CMaskString>  m_dynamicContent[2];
  bool                 m_hideFileExtensions;
};
