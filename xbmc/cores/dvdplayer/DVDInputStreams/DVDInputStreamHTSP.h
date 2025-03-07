

#pragma once
#include "DVDInputStream.h"
#include "filesystem/HTSPSession.h"

class CDVDInputStreamHTSP
  : public CDVDInputStream
  , public CDVDInputStream::IChannel
  , public CDVDInputStream::IDisplayTime
{
public:
  CDVDInputStreamHTSP();
  virtual ~CDVDInputStreamHTSP();
  virtual bool    Open(const char* file, const std::string &content);
  virtual void    Close();
  virtual int     Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence) { return -1; }
  virtual bool Pause(double dTime) { return false; };
  virtual bool    IsEOF();
  virtual __int64 GetLength()                      { return -1; }

  virtual bool    NextStream()                     { return m_startup; }

  virtual void    Abort();

  bool            NextChannel();
  bool            PrevChannel();
  bool            SelectChannel(unsigned int channel);
  bool            UpdateItem(CFileItem& item);

  int             GetTotalTime();
  int             GetTime();

  htsmsg_t* ReadStream();

private:
  typedef std::vector<HTSP::SChannel> SChannelV;
  typedef HTSP::const_circular_iter<SChannelV::iterator> SChannelC;
  bool      GetChannels(SChannelV &channels, SChannelV::iterator &it);
  bool      SetChannel(int channel);
  unsigned           m_subs;
  bool               m_startup;
  HTSP::CHTSPSession m_session;
  int                m_channel;
  int                m_tag;
  HTSP::SChannels    m_channels;
  HTSP::SEvent       m_event;

  struct SRead
  {
    SRead() { buf = NULL; Clear(); }
   ~SRead() { Clear(); }

    int  Size() { return end - cur; }
    void Clear()
    {
      free(buf);
      buf = NULL;
      cur = NULL;
      end = NULL;
    }

    uint8_t* buf;
    uint8_t* cur;
    uint8_t* end;
  } m_read;
};
