

#include "CharsetConverter.h"
#include "HTMLUtil.h"

using namespace std;
using namespace HTML;


CHTMLUtil::CHTMLUtil(void)
{}

CHTMLUtil::~CHTMLUtil(void)
{}

int CHTMLUtil::FindTag(const CStdString& strHTML, const CStdString& strTag, CStdString& strtagFound, int iPos) const
{
  CStdString strHTMLLow = strHTML;
  CStdString strTagLow = strTag;
  strHTMLLow.ToLower();
  strTagLow.ToLower();
  strtagFound = "";
  int iStart = strHTMLLow.Find(strTag, iPos);
  if (iStart < 0) return -1;
  int iEnd = strHTMLLow.Find(">", iStart);
  if (iEnd < 0) iEnd = (int)strHTMLLow.size();
  strtagFound = strHTMLLow.Mid(iStart, (iEnd + 1) - iStart);
  return iStart;
}

int CHTMLUtil::FindClosingTag(const CStdString& strHTML, const CStdString& strTag, CStdString& strtagFound, int iPos) const
{
  CStdString strHTMLLow = strHTML;
  CStdString strTagLow = strTag;
  strHTMLLow.ToLower();
  strTagLow.ToLower();
  strtagFound = "";
  int iStart = strHTMLLow.Find("</" + strTag, iPos);
  if (iStart < 0) return -1;
  int iOpenStart = strHTMLLow.Find("<" + strTag, iPos);
  while (iOpenStart < iStart && iOpenStart != -1)
  {
    iStart = strHTMLLow.Find("</" + strTag, iStart + 1);
    iOpenStart = strHTMLLow.Find("<" + strTag, iOpenStart + 1);
  }

  int iEnd = strHTMLLow.Find(">", iStart);
  if (iEnd < 0) iEnd = (int)strHTMLLow.size();
  strtagFound = strHTMLLow.Mid(iStart, (iEnd + 1) - iStart);
  return iStart;
}

void CHTMLUtil::getValueOfTag(const CStdString& strTagAndValue, CStdString& strValue)
{
  // strTagAndValue contains:
  // like <a href=blablabla.....>value</a>
  strValue = strTagAndValue;
  int iStart = strTagAndValue.Find(">");
  int iEnd = strTagAndValue.Find("<", iStart + 1);
  if (iStart >= 0 && iEnd >= 0)
  {
    iStart++;
    strValue = strTagAndValue.Mid(iStart, iEnd - iStart);
  }
}

void CHTMLUtil::getAttributeOfTag(const CStdString& strTagAndValue, const CStdString& strTag, CStdString& strValue)
{
  // strTagAndValue contains:
  // like <a href=""value".....
  strValue = strTagAndValue;
  int iStart = strTagAndValue.Find(strTag);
  if (iStart < 0) return ;
  iStart += (int)strTag.size();
  while (strTagAndValue[iStart + 1] == 0x20 || strTagAndValue[iStart + 1] == 0x27 || strTagAndValue[iStart + 1] == 34) iStart++;
  int iEnd = iStart + 1;
  while (strTagAndValue[iEnd] != 0x27 && strTagAndValue[iEnd] != 0x20 && strTagAndValue[iEnd] != 34 && strTagAndValue[iEnd] != '>') iEnd++;
  if (iStart >= 0 && iEnd >= 0)
  {
    strValue = strTagAndValue.Mid(iStart, iEnd - iStart);
  }
}

void CHTMLUtil::RemoveTags(CStdString& strHTML)
{
  int iNested = 0;
  CStdString strReturn = "";
  for (int i = 0; i < (int) strHTML.size(); ++i)
  {
    if (strHTML[i] == '<') iNested++;
    else if (strHTML[i] == '>') iNested--;
    else
    {
      if (!iNested)
      {
        strReturn += strHTML[i];
      }
    }
  }

  strHTML = strReturn;
}

typedef struct
{
  const wchar_t* html;
  const wchar_t  w;
} HTMLMapping;

static const HTMLMapping mappings[] =
  {{L"&amp;",     0x0026},
   {L"&apos;",    0x0027},
   {L"&acute;",   0x00B4},
   {L"&agrave;",  0x00E0},
   {L"&aacute;",  0x00E1},
   {L"&acirc;",   0x00E2},
   {L"&atilde;",  0x00E3},
   {L"&auml;",    0x00E4},
   {L"&aring;",   0x00E5},
   {L"&aelig;",   0x00E6},
   {L"&Agrave;",  0x00C0},
   {L"&Aacute;",  0x00C1},
   {L"&Acirc;",   0x00C2},
   {L"&Atilde;",  0x00C3},
   {L"&Auml;",    0x00C4},
   {L"&Aring;",   0x00C5},
   {L"&AElig;",   0x00C6},
   {L"&bdquo;",   0x201E},
   {L"&brvbar;",  0x00A6},
   {L"&bull;",    0x2022},
   {L"&bullet;",  0x2022},
   {L"&cent;",    0x00A2},
   {L"&circ;",    0x02C6},
   {L"&curren;",  0x00A4},
   {L"&copy;",    0x00A9},
   {L"&cedil;",   0x00B8},
   {L"&Ccedil;",  0x00C7},
   {L"&ccedil;",  0x00E7},
   {L"&dagger;",  0x2020},
   {L"&deg;",     0x00B0},
   {L"&divide;",  0x00F7},
   {L"&Dagger;",  0x2021},
   {L"&egrave;",  0x00E8},
   {L"&eacute;",  0x00E9},
   {L"&ecirc;",   0x00EA},
   {L"&emsp;",    0x2003},
   {L"&ensp;",    0x2002},
   {L"&euml;",    0x00EB},
   {L"&eth;",     0x00F0},
   {L"&euro;",    0x20AC},
   {L"&Egrave;",  0x00C8},
   {L"&Eacute;",  0x00C9},
   {L"&Ecirc;",   0x00CA},
   {L"&Euml;",    0x00CB},
   {L"&ETH;",     0x00D0},
   {L"&quot;",    0x0022},
   {L"&frasl;",   0x2044},
   {L"&frac14;",  0x00BC},
   {L"&frac12;",  0x00BD},
   {L"&frac34;",  0x00BE},
   {L"&gt;",      0x003E},
   {L"&hellip;",  0x2026},
   {L"&iexcl;",   0x00A1},
   {L"&iquest;",  0x00BF},
   {L"&igrave;",  0x00EC},
   {L"&iacute;",  0x00ED},
   {L"&icirc;",   0x00EE},
   {L"&iuml;",    0x00EF},
   {L"&Igrave;",  0x00CC},
   {L"&Iacute;",  0x00CD},
   {L"&Icirc;",   0x00CE},
   {L"&Iuml;",    0x00CF},
   {L"&lrm;",     0x200E},
   {L"&lt;",      0x003C},
   {L"&laquo;",   0x00AB},
   {L"&ldquo;",   0x201C},
   {L"&lsaquo;",  0x2039},
   {L"&lsquo;",   0x2018},
   {L"&macr;",    0x00AF},
   {L"&micro;",   0x00B5},
   {L"&middot;",  0x00B7},
   {L"&mdash;",   0x2014},
   {L"&nbsp;",    0x00A0},
   {L"&ndash;",   0x2013},
   {L"&ntilde;",  0x00F1},
   {L"&not;",     0x00AC},
   {L"&Ntilde;",  0x00D1},
   {L"&ordf;",    0x00AA},
   {L"&ordm;",    0x00BA},
   {L"&oelig;",   0x0153},
   {L"&ograve;",  0x00F2},
   {L"&oacute;",  0x00F3},
   {L"&ocirc;",   0x00F4},
   {L"&otilde;",  0x00F5},
   {L"&ouml;",    0x00F6},
   {L"&oslash;",  0x00F8},
   {L"&OElig;",   0x0152},
   {L"&Ograve;",  0x00D2},
   {L"&Oacute;",  0x00D3},
   {L"&Ocirc;",   0x00D4},
   {L"&Otilde;",  0x00D5},
   {L"&Ouml;",    0x00D6},
   {L"&Oslash;",  0x00D8},
   {L"&para;",    0x00B6},
   {L"&permil;",  0x2030},
   {L"&plusmn;",  0x00B1},
   {L"&pound;",   0x00A3},
   {L"&raquo;",   0x00BB},
   {L"&rdquo;",   0x201D},
   {L"&reg;",     0x00AE},
   {L"&rlm;",     0x200F},
   {L"&rsaquo;",  0x203A},
   {L"&rsquo;",   0x2019},
   {L"&sbquo;",   0x201A},
   {L"&scaron;",  0x0161},
   {L"&sect;",    0x00A7},
   {L"&shy;",     0x00AD},
   {L"&sup1;",    0x00B9},
   {L"&sup2;",    0x00B2},
   {L"&sup3;",    0x00B3},
   {L"&szlig;",   0x00DF},
   {L"&Scaron;",  0x0160},
   {L"&thinsp;",  0x2009},
   {L"&thorn;",   0x00FE},
   {L"&tilde;",   0x02DC},
   {L"&times;",   0x00D7},
   {L"&trade;",   0x2122},
   {L"&THORN;",   0x00DE},
   {L"&uml;",     0x00A8},
   {L"&ugrave;",  0x00F9},
   {L"&uacute;",  0x00FA},
   {L"&ucirc;",   0x00FB},
   {L"&uuml;",    0x00FC},
   {L"&Ugrave;",  0x00D9},
   {L"&Uacute;",  0x00DA},
   {L"&Ucirc;",   0x00DB},
   {L"&Uuml;",    0x00DC},
   {L"&yen;",     0x00A5},
   {L"&yuml;",    0x00FF},
   {L"&yacute;",  0x00FD},
   {L"&Yacute;",  0x00DD},
   {L"&Yuml;",    0x0178},
   {L"&zwj;",     0x200D},
   {L"&zwnj;",    0x200C},
   {NULL,         L'\0'}};

void CHTMLUtil::ConvertHTMLToW(const CStdStringW& strHTML, CStdStringW& strStripped)
{
  if (strHTML.size() == 0)
  {
    strStripped.Empty();
    return ;
  }
  int iPos = 0;
  strStripped = strHTML;
  while (mappings[iPos].html)
  {
    strStripped.Replace(mappings[iPos].html,CStdStringW(1, mappings[iPos].w));
    iPos++;
  }

  iPos = strStripped.Find(L"&#");
  while (iPos > 0 && iPos < (int)strStripped.size()-4)
  {
    int iStart = iPos + 1;
    iPos += 2;
    CStdStringW num;
    int base = 10;
    if (strStripped[iPos+1] == L'x')
    {
      base = 16;
      iPos++;
    }

    int i=iPos;
    while ( iPos < (int)strStripped.size() && 
           (base==16?iswxdigit(strStripped[iPos]):iswdigit(strStripped[iPos])))
      iPos++; 

    num = strStripped.Mid(i,iPos-i);
    wchar_t val = (wchar_t)wcstol(num.c_str(),NULL,base);
    if (base == 10)
      num.Format(L"&#%ls;",num.c_str());
    else
      num.Format(L"&#x%ls;",num.c_str());

    strStripped.Replace(num,CStdStringW(1,val));
    iPos = strStripped.Find(L"&#", iStart);
  }
}

