#pragma once



#include "system.h"
#ifdef HAS_FILESYSTEM_SFTP
#include "IFile.h"
#include "URL.h"
#include "FileItem.h"
#include "threads/CriticalSection.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0,3,2)
#define ssh_session SSH_SESSION
#endif

#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0,4,0)
#define sftp_file SFTP_FILE*
#define sftp_session SFTP_SESSION*
#define sftp_attributes SFTP_ATTRIBUTES*
#define sftp_dir SFTP_DIR*
#define ssh_session ssh_session*
#endif

//five secs timeout for SFTP
#define SFTP_TIMEOUT 5

class CSFTPSession
{
public:
  CSFTPSession(const CStdString &host, unsigned int port, const CStdString &username, const CStdString &password);
  virtual ~CSFTPSession();

  sftp_file CreateFileHande(const CStdString &file);
  void CloseFileHandle(sftp_file handle);
  bool GetDirectory(const CStdString &base, const CStdString &folder, CFileItemList &items);
  bool Exists(const char *path);
  int Stat(const char *path, struct __stat64* buffer);
  int Seek(sftp_file handle, uint64_t position);
  int Read(sftp_file handle, void *buffer, size_t length);
  int64_t GetPosition(sftp_file handle);
  bool IsIdle();
private:
  bool VerifyKnownHost(ssh_session session);
  bool Connect(const CStdString &host, unsigned int port, const CStdString &username, const CStdString &password);
  void Disconnect();
  CCriticalSection m_critSect;

  bool m_connected;
  ssh_session  m_session;
  sftp_session m_sftp_session;
  int m_LastActive;
};

typedef boost::shared_ptr<CSFTPSession> CSFTPSessionPtr;

class CSFTPSessionManager
{
public:
  static CSFTPSessionPtr CreateSession(const CURL &url);
  static CSFTPSessionPtr CreateSession(const CStdString &host, unsigned int port, const CStdString &username, const CStdString &password);
  static void ClearOutIdleSessions();
  static void DisconnectAllSessions();
private:
  static CCriticalSection m_critSect;
  static std::map<CStdString, CSFTPSessionPtr> sessions;
};

namespace XFILE
{
  class CFileSFTP : public IFile
  {
  public:
    CFileSFTP();
    virtual ~CFileSFTP();
    virtual void Close();
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual int Stat(struct __stat64* buffer);
    virtual int64_t GetLength();
    virtual int64_t GetPosition();
    virtual int     GetChunkSize() {return 1;};
    virtual int     IoControl(EIoControl request, void* param);
  private:
    CStdString m_file;
    CSFTPSessionPtr m_session;
    sftp_file m_sftp_handle;
  };
}
#endif
