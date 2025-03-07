

#include "DVDOverlayCodecSSA.h"
#include "DVDOverlaySSA.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDClock.h"
#include "Util.h"
#include "utils/AutoPtrHandle.h"

using namespace AUTOPTR;
using namespace std;

CDVDOverlayCodecSSA::CDVDOverlayCodecSSA() : CDVDOverlayCodec("SSA Subtitle Decoder")
{
  m_pOverlay = NULL;
  m_libass   = NULL;
  m_order    = 0;
}

CDVDOverlayCodecSSA::~CDVDOverlayCodecSSA()
{
  Dispose();
}

bool CDVDOverlayCodecSSA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if(hints.codec != CODEC_ID_SSA)
    return false;

  Dispose();

  m_hints  = hints;
  m_libass = new CDVDSubtitlesLibass();
  return m_libass->DecodeHeader((char *)hints.extradata, hints.extrasize);
}

void CDVDOverlayCodecSSA::Dispose()
{
  if(m_libass)
    SAFE_RELEASE(m_libass);

  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

int CDVDOverlayCodecSSA::Decode(BYTE* data, int size, double pts, double duration)
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);

  if(strncmp((const char*)data, "Dialogue:", 9) == 0)
  {
    int    sh, sm, ss, sc, eh, em, es, ec;
    size_t pos;
    CStdString      line, line2;
    CStdStringArray lines;
    CUtil::Tokenize((const char*)data, lines, "\r\n");
    for(size_t i=0; i<lines.size(); i++)
    {
      line = lines[i];
      line.Trim();
      auto_aptr<char> layer(new char[line.length()+1]);

      if(sscanf(line.c_str(), "%*[^:]:%[^,],%d:%d:%d%*c%d,%d:%d:%d%*c%d"
                            , layer.get(), &sh, &sm, &ss, &sc, &eh,&em, &es, &ec) != 9)
        continue;

      duration  = (eh*360000.0)+(em*6000.0)+(es*100.0)+ec;
      if(pts == DVD_NOPTS_VALUE)
        pts = duration;
      duration -= (sh*360000.0)+(sm*6000.0)+(ss*100.0)+sc;
      duration *= 10000;

      pos = line.find_first_of(",", 0);
      pos = line.find_first_of(",", pos+1);
      pos = line.find_first_of(",", pos+1);
      if(pos == CStdString::npos)
        continue;

      line2.Format("%d,%s,%s", m_order++, layer.get(), line.Mid(pos+1));

      if(!m_libass->DecodeDemuxPkt((char*)line2.c_str(), line2.length(), pts, duration))
        continue;

      if(m_pOverlay == NULL)
      {
        m_pOverlay = new CDVDOverlaySSA(m_libass);
        m_pOverlay->iPTSStartTime = pts;
        m_pOverlay->iPTSStopTime  = pts + duration;
      }

      if(m_pOverlay->iPTSStopTime < pts + duration)
        m_pOverlay->iPTSStopTime  = pts + duration;
    }
  }
  else
  {
    if(m_libass->DecodeDemuxPkt((char*)data, size, pts, duration))
        m_pOverlay = new CDVDOverlaySSA(m_libass);
  }

  return OC_OVERLAY;
}
void CDVDOverlayCodecSSA::Reset()
{
  Dispose();
  m_order  = 0;
  m_libass = new CDVDSubtitlesLibass();
  m_libass->DecodeHeader((char *)m_hints.extradata, m_hints.extrasize);
}

void CDVDOverlayCodecSSA::Flush()
{
  Reset();
}

CDVDOverlay* CDVDOverlayCodecSSA::GetOverlay()
{
  if(m_pOverlay)
  {
    CDVDOverlay* overlay = m_pOverlay;
    m_pOverlay = NULL;
    return overlay;
  }
  return NULL;
}


