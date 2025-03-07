

// FileRar.h: interface for the CFileRar class.

#pragma once
#ifndef FILERAR_H_
#define FILERAR_H_

#include "File.h"
#include "threads/Thread.h"
#include "threads/Event.h"

class CmdExtract;
class CommandData;
class Archive;

namespace XFILE
{
#ifdef HAS_FILESYSTEM_RAR
  class CFileRarExtractThread : public CThread
  {
  public:
    CFileRarExtractThread();
    ~CFileRarExtractThread();

    void Start(Archive* pArc, CommandData* pCmd, CmdExtract* pExtract, int iSize);

    virtual void OnStartup();
    virtual void OnExit();
    virtual void Process();

    CEvent hRunning;
    CEvent hRestart;
    CEvent hQuit;

  protected:
    Archive* m_pArc;
    CommandData* m_pCmd;
    CmdExtract* m_pExtract;
    int m_iSize;
  };
#endif

  class CFileRar : public IFile
  {
  public:
    CFileRar();
    CFileRar(bool bSeekable); // used for caching files
    virtual ~CFileRar();
    virtual int64_t       GetPosition();
    virtual int64_t       GetLength();
    virtual bool          Open(const CURL& url);
    virtual bool          Exists(const CURL& url);
    virtual int           Stat(const CURL& url, struct __stat64* buffer);
    virtual unsigned int  Read(void* lpBuf, int64_t uiBufSize);
    virtual int           Write(const void* lpBuf, int64_t uiBufSize);
    virtual int64_t       Seek(int64_t iFilePosition, int iWhence=SEEK_SET);
    virtual void          Close();
    virtual void          Flush();

    virtual bool          OpenForWrite(const CURL& url);
    unsigned int          Write(void *lpBuf, int64_t uiBufSize);

  protected:
    CStdString m_strCacheDir;
    CStdString m_strRarPath;
    CStdString m_strPassword;
    CStdString m_strPathInRar;
    BYTE m_bFileOptions;
    void Init();
    void InitFromUrl(const CURL& url);
    bool OpenInArchive();
    void CleanUp();

    int64_t m_iFilePosition;
    int64_t m_iFileSize;
    // rar stuff
    bool m_bUseFile;
    bool m_bOpen;
    bool m_bSeekable;
    CFile m_File; // for packed source
#ifdef HAS_FILESYSTEM_RAR
    Archive* m_pArc;
    CommandData* m_pCmd;
    CmdExtract* m_pExtract;
    CFileRarExtractThread* m_pExtractThread;
#endif
    byte* m_szBuffer;
    byte* m_szStartOfBuffer;
    int64_t m_iDataInBuffer;
    int64_t m_iBufferStart;
  };

}

#endif  // FILERAR_H_

