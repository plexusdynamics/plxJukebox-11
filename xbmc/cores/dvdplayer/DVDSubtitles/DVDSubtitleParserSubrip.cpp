

#include "DVDSubtitleParserSubrip.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDClock.h"
#include "utils/StdString.h"
#include "DVDSubtitleTagSami.h"

using namespace std;

CDVDSubtitleParserSubrip::CDVDSubtitleParserSubrip(CDVDSubtitleStream* pStream, const string& strFile)
    : CDVDSubtitleParserText(pStream, strFile)
{
}

CDVDSubtitleParserSubrip::~CDVDSubtitleParserSubrip()
{
  Dispose();
}

bool CDVDSubtitleParserSubrip::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  CDVDSubtitleTagSami TagConv;
  if (!TagConv.Init())
    return false;

  char line[1024];
  CStdString strLine;

  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    strLine = line;
    strLine.Trim();

    if (strLine.length() > 0)
    {
      char sep;
      int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;
      int c = sscanf(strLine.c_str(), "%d%c%d%c%d%c%d --> %d%c%d%c%d%c%d\n",
                     &hh1, &sep, &mm1, &sep, &ss1, &sep, &ms1,
                     &hh2, &sep, &mm2, &sep, &ss2, &sep, &ms2);

      if (c == 1)
      {
        // numbering, skip it
      }
      else if (c == 14) // time info
      {
        CDVDOverlayText* pOverlay = new CDVDOverlayText();
        pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

        pOverlay->iPTSStartTime = ((double)(((hh1 * 60 + mm1) * 60) + ss1) * 1000 + ms1) * (DVD_TIME_BASE / 1000);
        pOverlay->iPTSStopTime  = ((double)(((hh2 * 60 + mm2) * 60) + ss2) * 1000 + ms2) * (DVD_TIME_BASE / 1000);

        while (m_pStream->ReadLine(line, sizeof(line)))
        {
          strLine = line;
          strLine.Trim();

          // empty line, next subtitle is about to start
          if (strLine.length() <= 0) break;

          TagConv.ConvertLine(pOverlay, strLine.c_str(), strLine.length());
        }
        TagConv.CloseTag(pOverlay);
        m_collection.Add(pOverlay);
      }
    }
  }
  m_collection.Sort();
  return true;
}

