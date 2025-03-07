#pragma once



#include "DVDOverlayContainer.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "DVDStreamInfo.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxSPU.h"

class CDVDInputStream;
class CDVDSubtitleStream;
class CDVDSubtitleParser;
class CDVDInputStreamNavigator;
class CDVDOverlayCodec;

class CDVDPlayerSubtitle
{
public:
  CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer);
  ~CDVDPlayerSubtitle();

  void Process(double pts);
  void Flush();
  void FindSubtitles(const char* strFilename);
  void GetCurrentSubtitle(CStdString& strSubtitle, double pts);
  int GetSubtitleCount();

  void UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, int iAction) { m_pOverlayContainer->UpdateOverlayInfo(pStream, &m_dvdspus, iAction); }

  bool AcceptsData();
  void SendMessage(CDVDMsg* pMsg);
  bool OpenStream(CDVDStreamInfo &hints, std::string& filename);
  void CloseStream(bool flush);

  bool IsStalled() { return m_pOverlayContainer->GetSize() == 0; }
private:
  CDVDOverlayContainer* m_pOverlayContainer;

  CDVDSubtitleStream* m_pSubtitleStream;
  CDVDSubtitleParser* m_pSubtitleFileParser;
  CDVDOverlayCodec*   m_pOverlayCodec;
  CDVDDemuxSPU        m_dvdspus;

  CDVDStreamInfo      m_streaminfo;
  double              m_lastPts;
};


//typedef struct SubtitleInfo
//{

//
//} SubtitleInfo;

