#pragma once


#include "IFile.h"
#include "ILiveTV.h"
#include "MythSession.h"
#include "XBDateTime.h"
#include "video/VideoInfoTag.h"
#include <queue>

extern "C" {
#include "cmyth/include/cmyth/cmyth.h"
}

class DllLibCMyth;

namespace XFILE
{


class CMythFile
  : public  IFile
  ,         ILiveTVInterface
  ,         IRecordable
  , private CMythSession::IEventListener
{
public:
  CMythFile();
  virtual ~CMythFile();
  virtual bool          Open(const CURL& url);
  virtual int64_t       Seek(int64_t pos, int whence=SEEK_SET);
  virtual int64_t       GetPosition();
  virtual int64_t       GetLength();
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual unsigned int  Read(void* buffer, int64_t size);
  virtual CStdString    GetContent() { return ""; }
  virtual bool          SkipNext();

  virtual bool          Delete(const CURL& url);
  virtual bool          Exists(const CURL& url);
  virtual int           GetChunkSize() {return 1;};

  virtual ILiveTVInterface* GetLiveTV() {return (ILiveTVInterface*)this;}

  virtual bool           NextChannel();
  virtual bool           PrevChannel();
  virtual bool           SelectChannel(unsigned int channel);

  virtual int            GetTotalTime();
  virtual int            GetStartTime();

  virtual bool           UpdateItem(CFileItem& item);

  virtual IRecordable*   GetRecordable() {return (IRecordable*)this;}

  virtual bool           CanRecord();
  virtual bool           IsRecording();
  virtual bool           Record(bool bOnOff);

  virtual bool           GetCommBreakList(cmyth_commbreaklist_t& commbreaklist);
  virtual bool           GetCutList(cmyth_commbreaklist_t& commbreaklist);

  virtual int            IoControl(EIoControl request, void* param);
protected:
  virtual void OnEvent(int event, const std::string& data);

  bool HandleEvents();
  bool ChangeChannel(int direction, const CStdString &channel);

  bool SetupConnection(const CURL& url, bool control, bool event, bool database);
  bool SetupRecording(const CURL& url);
  bool SetupLiveTV(const CURL& url);
  bool SetupFile(const CURL& url);

  CStdString GetValue(char* str) { return m_session->GetValue(str); }
  CDateTime  GetValue(const cmyth_timestamp_t t) { return m_session->GetValue(t); }

  CMythSession*     m_session;
  DllLibCMyth*      m_dll;
  cmyth_conn_t      m_control;
  cmyth_database_t  m_database;
  cmyth_recorder_t  m_recorder;
  cmyth_proginfo_t  m_program;
  cmyth_file_t      m_file;
  cmyth_timestamp_t m_starttime;
  CStdString        m_filename;
  CVideoInfoTag     m_infotag;

  CCriticalSection  m_section;
  std::queue<std::pair<int, std::string> > m_events;

  bool              m_recording;

  unsigned int      m_timestamp;
};

}


