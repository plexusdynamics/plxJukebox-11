

// XmlDocument.h: interface for the CXmlDocument class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLDOCUMENT_H__D68461F7_E0CE_4FA0_B1C9_0541610164E9__INCLUDED_)
#define AFX_XMLDOCUMENT_H__D68461F7_E0CE_4FA0_B1C9_0541610164E9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <d3d9.h>

#include <stdio.h>

#define XML_ROOT_NODE 0
#define XML_MAX_TAGNAME_SIZE	32
#define XML_MAX_INNERTEXT_SIZE	1024

typedef int    XmlNode;
typedef void (*XmlNodeCallback) (char* szTag, XmlNode node);      



class CXmlDocument  
{
public:
	CXmlDocument();
	virtual ~CXmlDocument();

	void	Create(char* szString);
	int     Load(char* szFile);
	void    Close();

	int		GetNodeCount(char* tag);

	void    EnumerateNodes(char* szTag, XmlNodeCallback pFunc);

	XmlNode GetChildNode(XmlNode node, char* szTag);
	XmlNode GetNextNode(XmlNode node);
	char*   GetNodeText(XmlNode node);
	char*   GetNodeTag(XmlNode node);

private:

	char*	m_doc;
	int		m_size;
	int		m_nodes;
	char	m_szTag[XML_MAX_TAGNAME_SIZE];
	char	m_szText[XML_MAX_INNERTEXT_SIZE];
};

class WriteXML
{
public:
  WriteXML() { m_file = NULL; m_rootTag = NULL; };
  ~WriteXML() { Close(); };

  bool Open(const char *szFile, const char *szOpeningTag)
  {
    remove(szFile);
    if (!szFile || !szOpeningTag) return false;
    m_file = fopen(szFile, "w");
    if (!m_file) return false;
    m_rootTag = new char[strlen(szOpeningTag) + 1];
    strcpy(m_rootTag, szOpeningTag);
    fprintf(m_file, "<%s>\n", m_rootTag);
    return true;
  };
  void Close()
  {
    if (m_file)
    {
      if (m_rootTag)
        fprintf(m_file, "</%s>\n", m_rootTag);
      fclose(m_file);
    }
    delete[] m_rootTag;
    m_rootTag = NULL;
    m_file = NULL;
  };
  void WriteTag(const char *szTag, const char *data)
  {
    if (!m_file || !szTag || !data) return;
    fprintf(m_file, "\t<%s>%s</%s>\n", szTag, data, szTag);
  };
  void WriteTag(const char *szTag, int data, const char *format = "%i")
  {
    char temp[10];
    sprintf(temp, format, data);
    WriteTag(szTag, temp);
  };
  void WriteTag(const char *szTag, float data)
  {
    if (!m_file || !szTag) return;
    fprintf(m_file, "\t<%s>%f</%s>\n", szTag, data, szTag);
  };
  void WriteTag(const char *szTag, bool data)
  {
    if (!m_file || !szTag) return;
    fprintf(m_file, "\t<%s>%s</%s>\n", szTag, data ? "true" : "false", szTag);
  };

private:
  char *m_rootTag;
  FILE *m_file;
};

#endif // !defined(AFX_XMLDOCUMENT_H__D68461F7_E0CE_4FA0_B1C9_0541610164E9__INCLUDED_)
