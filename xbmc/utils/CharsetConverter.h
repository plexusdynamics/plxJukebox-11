#ifndef CCHARSET_CONVERTER
#define CCHARSET_CONVERTER



#include "threads/CriticalSection.h"
#include "StdString.h"
#include "utils/GlobalsHandling.h"

#include <vector>

class CCharsetConverter 
{
public:
  CCharsetConverter();

  void reset();

  void clear();

  void utf8ToW(const CStdStringA& utf8String, CStdStringW &utf16String, bool bVisualBiDiFlip=true, bool forceLTRReadingOrder=false, bool* bWasFlipped=NULL);

  void utf16LEtoW(const CStdString16& utf16String, CStdStringW &wString);

  void subtitleCharsetToW(const CStdStringA& strSource, CStdStringW& strDest);

  void utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest);

  void utf8ToStringCharset(CStdStringA& strSourceDest);
  void utf8ToSystem(CStdStringA& strSourceDest);

  void utf8To(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdStringA& strDest);
  void utf8To(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdString16& strDest);
  void utf8To(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdString32& strDest);

  void stringCharsetToUtf8(const CStdStringA& strSourceCharset, const CStdStringA& strSource, CStdStringA& strDest);

  bool isValidUtf8(const CStdString& str);

  bool isValidUtf8(const char *buf, unsigned int len);

  void ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest, bool swap = false);

  void wToUTF8(const CStdStringW& strSource, CStdStringA &strDest);
  void utf16BEtoUTF8(const CStdString16& strSource, CStdStringA &strDest);
  void utf16LEtoUTF8(const CStdString16& strSource, CStdStringA &strDest);
  void ucs2ToUTF8(const CStdString16& strSource, CStdStringA& strDest);

  void utf8logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest);

  void utf32ToStringCharset(const unsigned long* strSource, CStdStringA& strDest);

  std::vector<CStdString> getCharsetLabels();
  CStdString getCharsetLabelByName(const CStdString& charsetName);
  CStdString getCharsetNameByLabel(const CStdString& charsetLabel);
  bool isBidiCharset(const CStdString& charset);

  void unknownToUTF8(CStdStringA &sourceDest);
  void unknownToUTF8(const CStdStringA &source, CStdStringA &dest);

  void toW(const CStdStringA& source, CStdStringW& dest, const CStdStringA& enc);
  void fromW(const CStdStringW& source, CStdStringA& dest, const CStdStringA& enc);
};

XBMC_GLOBAL(CCharsetConverter,g_charsetConverter);

size_t iconv_const (void* cd, const char** inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);

#endif
