#pragma once



#include "cores/IPlayer.h"
#include "threads/Thread.h"

#include "IDVDPlayer.h"

#include "DVDMessageQueue.h"
#include "DVDClock.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayerVideo.h"
#include "DVDPlayerSubtitle.h"
#include "DVDPlayerTeletext.h"

//#include "DVDChapterReader.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "utils/BitstreamStats.h"

#include "Edl.h"
#include "FileItem.h"
#include "threads/SingleLock.h"


class CDVDInputStream;

class CDVDDemux;
class CDemuxStreamVideo;
class CDemuxStreamAudio;
class CStreamInfo;

#define DVDSTATE_NORMAL           0x00000001 // normal dvd state
#define DVDSTATE_STILL            0x00000002 // currently displaying a still frame
#define DVDSTATE_WAIT             0x00000003 // waiting for demuxer read error
#define DVDSTATE_SEEK             0x00000004 // we are finishing a seek request

class CCurrentStream
{
public:
  int              id;     // demuxerid of current playing stream
  int              source;
  double           dts;    // last dts from demuxer, used to find disncontinuities
  CDVDStreamInfo   hint;   // stream hints, used to notice stream changes
  void*            stream; // pointer or integer, identifying stream playing. if it changes stream changed
  bool             inited;
  bool             started; // has the player started
  const StreamType type;
  // stuff to handle starting after seek
  double   startpts;
  CDVDMsg* startsync;

  CCurrentStream(StreamType t)
    : type(t)
  {
    startsync = NULL;
    Clear();
  }

  void Clear()
  {
    id     = -1;
    source = STREAM_SOURCE_NONE;
    dts    = DVD_NOPTS_VALUE;
    hint.Clear();
    stream = NULL;
    inited = false;
    started = false;
    if(startsync)
      startsync->Release();
    startsync = NULL;
    startpts  = DVD_NOPTS_VALUE;
  }
};

typedef struct
{
  StreamType   type;
  std::string  filename;
  std::string  filename2;  // for vobsub subtitles, 2 files are necessary (idx/sub) 
  std::string  language;
  std::string  name;
  CDemuxStream::EFlags flags;
  int          source;
  int          id;
  std::string  codec;
  int          channels;
} SelectionStream;

class CSelectionStreams
{
  CCriticalSection m_section;
  SelectionStream  m_invalid;
public:
  CSelectionStreams()
  {
    m_invalid.id = -1;
    m_invalid.source = STREAM_SOURCE_NONE;
    m_invalid.type = STREAM_NONE;
  }
  std::vector<SelectionStream> m_Streams;

  int              IndexOf (StreamType type, int source, int id) const;
  int              IndexOf (StreamType type, CDVDPlayer& p) const;
  int              Count   (StreamType type) const { return IndexOf(type, STREAM_SOURCE_NONE, -1) + 1; }
  SelectionStream& Get     (StreamType type, int index);
  bool             Get     (StreamType type, CDemuxStream::EFlags flag, SelectionStream& out);

  void             Clear   (StreamType type, StreamSource source);
  int              Source  (StreamSource source, std::string filename);

  void             Update  (SelectionStream& s);
  void             Update  (CDVDInputStream* input, CDVDDemux* demuxer);
};


#define DVDPLAYER_AUDIO    1
#define DVDPLAYER_VIDEO    2
#define DVDPLAYER_SUBTITLE 3
#define DVDPLAYER_TELETEXT 4

class CDVDPlayer : public IPlayer, public CThread, public IDVDPlayer
{
public:
  CDVDPlayer(IPlayerCallback& callback);
  virtual ~CDVDPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdPlayerAudio.RegisterAudioCallback(pCallback); }
  virtual void UnRegisterAudioCallback()                        { m_dvdPlayerAudio.UnRegisterAudioCallback(); }
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo() const;
  virtual bool HasAudio() const;
  virtual bool IsPassthrough() const;
  virtual bool CanSeek();
  virtual void Seek(bool bPlus, bool bLargeStep);
  virtual bool SeekScene(bool bPlus = true);
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage();
  virtual float GetCachePercentage();

  virtual void SetVolume(long nVolume)                          { m_dvdPlayerAudio.SetVolume(nVolume); }
  virtual void SetDynamicRangeCompression(long drc)             { m_dvdPlayerAudio.SetDynamicRangeCompression(drc); }
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing)                       { m_dvdPlayerVideo.Update(bPauseDrawing); }
  virtual void GetVideoRect(CRect& SrcRect, CRect& DestRect)    { m_dvdPlayerVideo.GetVideoRect(SrcRect, DestRect); }
  virtual void GetVideoAspectRatio(float& fAR)                  { fAR = m_dvdPlayerVideo.GetAspectRatio(); }
  virtual bool CanRecord();
  virtual bool IsRecording();
  virtual bool Record(bool bOnOff);
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();

  virtual void SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();
  virtual int GetSubtitleCount();
  virtual int GetSubtitle();
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void GetSubtitleLanguage(int iStream, CStdString &strStreamLang);
  virtual void SetSubtitle(int iStream);
  virtual bool GetSubtitleVisible();
  virtual void SetSubtitleVisible(bool bVisible);
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension) { return false; }
  virtual int  AddSubtitle(const CStdString& strSubPath);

  virtual int GetAudioStreamCount();
  virtual int GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName);
  virtual void SetAudioStream(int iStream);
  virtual void GetAudioStreamLanguage(int iStream, CStdString &strLanguage);

  virtual TextCacheStruct_t* GetTeletextCache();
  virtual void LoadPage(int p, int sp, unsigned char* buffer);

  virtual int  GetChapterCount();
  virtual int  GetChapter();
  virtual void GetChapterName(CStdString& strChapterName);
  virtual int  SeekChapter(int iChapter);

  virtual void SeekTime(__int64 iTime);
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual bool OnAction(const CAction &action);
  virtual bool HasMenu();
  virtual int GetAudioBitrate();
  virtual int GetVideoBitrate();
  virtual int GetSourceBitrate();
  virtual int GetChannels();
  virtual CStdString GetAudioCodecName();
  virtual CStdString GetVideoCodecName();
  virtual int GetPictureWidth();
  virtual int GetPictureHeight();
  virtual bool GetStreamDetails(CStreamDetails &details);

  virtual bool GetCurrentSubtitle(CStdString& strSubtitle);

  virtual CStdString GetPlayerState();
  virtual bool SetPlayerState(CStdString state);

  virtual CStdString GetPlayingTitle();

  enum ECacheState
  { CACHESTATE_DONE = 0
  , CACHESTATE_FULL     // player is filling up the demux queue
  , CACHESTATE_INIT     // player is waiting for first packet of each stream
  , CACHESTATE_PLAY     // player is waiting for players to not be stalled
  , CACHESTATE_FLUSH    // temporary state player will choose startup between init or full
  };

  virtual bool IsCaching() const { return m_caching == CACHESTATE_FULL; }
  virtual int GetCacheLevel() const ;

  virtual int OnDVDNavResult(void* pData, int iMessage);
protected:
  friend class CSelectionStreams;

  class StreamLock : public CSingleLock
  {
  public:
    inline StreamLock(CDVDPlayer* cdvdplayer) : CSingleLock(cdvdplayer->m_critStreamSection) {}
  };

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  bool OpenAudioStream(int iStream, int source);
  bool OpenVideoStream(int iStream, int source);
  bool OpenSubtitleStream(int iStream, int source);
  bool OpenTeletextStream(int iStream, int source);
  bool CloseAudioStream(bool bWaitForBuffers);
  bool CloseVideoStream(bool bWaitForBuffers);
  bool CloseSubtitleStream(bool bKeepOverlays);
  bool CloseTeletextStream(bool bWaitForBuffers);

  void ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessTeletextData(CDemuxStream* pStream, DemuxPacket* pPacket);

  int  AddSubtitleFile(const std::string& filename, const std::string& subfilename = "", CDemuxStream::EFlags flags = CDemuxStream::FLAG_NONE);

  /**
   * one of the DVD_PLAYSPEED defines
   */
  void SetPlaySpeed(int iSpeed);
  int GetPlaySpeed()                                                { return m_playSpeed; }
  void SetCaching(ECacheState state);

  __int64 GetTotalTimeInMsec();

  double GetQueueTime();
  bool GetCachingTimes(double& play_left, double& cache_left, double& file_offset);


  void FlushBuffers(bool queued, double pts = DVD_NOPTS_VALUE, bool accurate = true);

  void HandleMessages();
  void HandlePlaySpeed();
  bool IsInMenu() const;

  void SynchronizePlayers(DWORD sources);
  void SynchronizeDemuxer(DWORD timeout);
  void CheckAutoSceneSkip();
  void CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket);
  bool CheckSceneSkip(CCurrentStream& current);
  bool CheckPlayerInit(CCurrentStream& current, unsigned int source);
  bool CheckStartCaching(CCurrentStream& current);
  void UpdateTimestamps(CCurrentStream& current, DemuxPacket* pPacket);
  void SendPlayerMessage(CDVDMsg* pMsg, unsigned int target);

  bool ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);
  bool IsValidStream(CCurrentStream& stream);
  bool IsBetterStream(CCurrentStream& current, CDemuxStream* stream);

  bool OpenInputStream();
  bool OpenDemuxStream();
  void OpenDefaultStreams();

  void UpdateApplication(double timeout);
  void UpdatePlayState(double timeout);
  double m_UpdateApplication;

  bool m_bAbortRequest;

  std::string m_filename; // holds the actual filename
  std::string m_mimetype;  // hold a hint to what content file contains (mime type)
  ECacheState m_caching;
  CFileItem   m_item;


  CCurrentStream m_CurrentAudio;
  CCurrentStream m_CurrentVideo;
  CCurrentStream m_CurrentSubtitle;
  CCurrentStream m_CurrentTeletext;

  CSelectionStreams m_SelectionStreams;

  int m_playSpeed;
  struct SSpeedState
  {
    double lastpts;  // holds last display pts during ff/rw operations
    double lasttime;
  } m_SpeedState;

  int m_errorCount;

  CDVDMessageQueue m_messenger;     // thread messenger

  CDVDPlayerVideo m_dvdPlayerVideo; // video part
  CDVDPlayerAudio m_dvdPlayerAudio; // audio part
  CDVDPlayerSubtitle m_dvdPlayerSubtitle; // subtitle part
  CDVDTeletextData m_dvdPlayerTeletext; // teletext part

  CDVDClock m_clock;                // master clock
  CDVDOverlayContainer m_overlayContainer;

  CDVDInputStream* m_pInputStream;  // input stream for current playing file
  CDVDDemux* m_pDemuxer;            // demuxer for current playing file
  CDVDDemux* m_pSubtitleDemuxer;

  CStdString m_lastSub;
  
  struct SDVDInfo
  {
    void Clear()
    {
      state                =  DVDSTATE_NORMAL;
      iSelectedSPUStream   = -1;
      iSelectedAudioStream = -1;
      iDVDStillTime        =  0;
      iDVDStillStartTime   =  0;
    }

    int state;                // current dvdstate
    unsigned int iDVDStillTime;      // total time in ticks we should display the still before continuing
    unsigned int iDVDStillStartTime; // time in ticks when we started the still
    int iSelectedSPUStream;   // mpeg stream id, or -1 if disabled
    int iSelectedAudioStream; // mpeg stream id, or -1 if disabled
  } m_dvd;

  struct SPlayerState
  {
    SPlayerState() { Clear(); }
    void Clear()
    {
      timestamp     = 0;
      time          = 0;
      time_total    = 0;
      time_offset   = 0;
      dts           = DVD_NOPTS_VALUE;
      player_state  = "";
      chapter       = 0;
      chapter_name  = "";
      chapter_count = 0;
      canrecord     = false;
      recording     = false;
      demux_video   = "";
      demux_audio   = "";
      cache_bytes   = 0;
      cache_level   = 0.0;
      cache_delay   = 0.0;
      cache_offset  = 0.0;
    }

    double timestamp;         // last time of update
    double time_offset;       // difference between time and pts

    double time;              // current playback time
    double time_total;        // total playback time
    double dts;               // last known dts

    std::string player_state;  // full player state

    int         chapter;      // current chapter
    std::string chapter_name; // name of current chapter
    int         chapter_count;// number of chapter

    bool canrecord;           // can input stream record
    bool recording;           // are we currently recording

    std::string demux_video;
    std::string demux_audio;

    __int64 cache_bytes;   // number of bytes current's cached
    double  cache_level;   // current estimated required cache level
    double  cache_delay;   // time until cache is expected to reach estimated level
    double  cache_offset;  // percentage of file ahead of current position
  } m_State;
  CCriticalSection m_StateSection;

  CEvent m_ready;
  CCriticalSection m_critStreamSection; // need to have this lock when switching streams (audio / video)

  CEdl m_Edl;

  struct SEdlAutoSkipMarkers {

    void Clear()
    {
      cut = -1;
      commbreak_start = -1;
      commbreak_end = -1;
      seek_to_start = false;
      reset = 0;
      mute = false;
    }

    void ResetCutMarker(double timeout)
    {
      if(reset != 0
      && reset + DVD_MSEC_TO_TIME(timeout) > CDVDClock::GetAbsoluteClock())
        return;

      /*
       * Reset the automatic EDL skip marker for a cut so automatic seeking can happen again if,
       * for example, the initial automatic skip ended up back in the cut due to seeking
       * inaccuracies.
       */
      cut = -1;

      reset = CDVDClock::GetAbsoluteClock();
    }

    int cut;              // last automatically skipped EDL cut seek position
    int commbreak_start;  // start time of the last commercial break automatically skipped
    int commbreak_end;    // end time of the last commercial break automatically skipped
    bool seek_to_start;   // whether seeking can go back to the start of a previously skipped break
    double reset;         // last actual reset time
    bool mute;            // whether EDL mute is on

  } m_EdlAutoSkipMarkers;

  CPlayerOptions m_PlayerOptions;
};
