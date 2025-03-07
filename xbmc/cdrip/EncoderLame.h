#ifndef _ENCODERLAME_H
#define _ENCODERLAME_H



#include "Encoder.h"
#include "DllLameenc.h"

class CEncoderLame : public CEncoder
{
public:
  CEncoderLame();
  virtual ~CEncoderLame() {}
  bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  int Encode(int nNumBytesRead, BYTE* pbtStream);
  bool Close();
  void AddTag(int key, const char* value);

protected:
  lame_global_flags* m_pGlobalFlags;

  unsigned char m_buffer[48160]; // mp3buf_size in bytes = 1.25*(chunk size / 4) + 7200
  char m_inPath[1024 + 1];
  char m_outPath[1024 + 1];

  DllLameEnc m_dll;
};

#endif // _ENCODERLAME_H
