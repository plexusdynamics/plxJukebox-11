

#ifndef GUILIB_GUICONTROLPROFILER_H__
#define GUILIB_GUICONTROLPROFILER_H__
#pragma once

#include "GUIControl.h"

class CGUIControlProfiler;
class TiXmlElement;

class CGUIControlProfilerItem
{
public:
  CGUIControlProfiler *m_pProfiler;
  CGUIControlProfilerItem * m_pParent;
  CGUIControl *m_pControl;
  std::vector<CGUIControlProfilerItem *> m_vecChildren;
  CStdString m_strDescription;
  int m_controlID;
  CGUIControl::GUICONTROLTYPES m_ControlType;
  unsigned int m_visTime;
  unsigned int m_renderTime;
  int64_t m_i64VisStart;
  int64_t m_i64RenderStart;

  CGUIControlProfilerItem(CGUIControlProfiler *pProfiler, CGUIControlProfilerItem *pParent, CGUIControl *pControl);
  ~CGUIControlProfilerItem(void);

  void Reset(CGUIControlProfiler *pProfiler);
  void BeginVisibility(void);
  void EndVisibility(void);
  void BeginRender(void);
  void EndRender(void);
  void SaveToXML(TiXmlElement *parent);
  unsigned int GetTotalTime(void) const { return m_visTime + m_renderTime; };

  CGUIControlProfilerItem *AddControl(CGUIControl *pControl);
  CGUIControlProfilerItem *FindOrAddControl(CGUIControl *pControl, bool recurse);
};

class CGUIControlProfiler
{
public:
  static CGUIControlProfiler &Instance(void);
  static bool IsRunning(void);

  void Start(void);
  void EndFrame(void);
  void BeginVisibility(CGUIControl *pControl);
  void EndVisibility(CGUIControl *pControl);
  void BeginRender(CGUIControl *pControl);
  void EndRender(CGUIControl *pControl);
  int GetMaxFrameCount(void) const { return m_iMaxFrameCount; };
  void SetMaxFrameCount(int iMaxFrameCount) { m_iMaxFrameCount = iMaxFrameCount; };
  void SetOutputFile(const CStdString &strOutputFile) { m_strOutputFile = strOutputFile; };
  const CStdString &GetOutputFile(void) const { return m_strOutputFile; };
  bool SaveResults(void);
  unsigned int GetTotalTime(void) const { return m_ItemHead.GetTotalTime(); };

  float m_fPerfScale;
private:
  CGUIControlProfiler(void);
  ~CGUIControlProfiler(void) {};
  CGUIControlProfiler(const CGUIControlProfiler &that);
  CGUIControlProfiler &operator=(const CGUIControlProfiler &that);

  CGUIControlProfilerItem m_ItemHead;
  CGUIControlProfilerItem *m_pLastItem;
  CGUIControlProfilerItem *FindOrAddControl(CGUIControl *pControl);

  static bool m_bIsRunning;
  CStdString m_strOutputFile;
  int m_iMaxFrameCount;
  int m_iFrameCount;
};

#define GUIPROFILER_VISIBILITY_BEGIN(x) { if (CGUIControlProfiler::IsRunning()) CGUIControlProfiler::Instance().BeginVisibility(x); }
#define GUIPROFILER_VISIBILITY_END(x) { if (CGUIControlProfiler::IsRunning()) CGUIControlProfiler::Instance().EndVisibility(x); }
#define GUIPROFILER_RENDER_BEGIN(x) { if (CGUIControlProfiler::IsRunning()) CGUIControlProfiler::Instance().BeginRender(x); }
#define GUIPROFILER_RENDER_END(x) { if (CGUIControlProfiler::IsRunning()) CGUIControlProfiler::Instance().EndRender(x); }

#endif
