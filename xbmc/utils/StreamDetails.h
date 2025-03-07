#pragma once


#include "Archive.h"
#include "ISerializable.h"
#include <vector>

class CStreamDetails;

class CStreamDetail : public IArchivable, public ISerializable
{
public:
  enum StreamType {
    VIDEO,
    AUDIO,
    SUBTITLE
  };

  CStreamDetail(StreamType type) : m_eType(type) {};
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual bool IsWorseThan(CStreamDetail *that) { return true; };

  const StreamType m_eType;

protected:
  CStreamDetails *m_pParent;
  friend class CStreamDetails;
};

class CStreamDetailVideo : public CStreamDetail
{
public:
  CStreamDetailVideo();
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual bool IsWorseThan(CStreamDetail *that);

  int m_iWidth;
  int m_iHeight;
  float m_fAspect;
  int m_iDuration;
  CStdString m_strCodec;
};

class CStreamDetailAudio : public CStreamDetail
{
public:
  CStreamDetailAudio();
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual bool IsWorseThan(CStreamDetail *that);

  int m_iChannels;
  CStdString m_strCodec;
  CStdString m_strLanguage;
};

class CStreamDetailSubtitle : public CStreamDetail
{
public:
  CStreamDetailSubtitle();
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual bool IsWorseThan(CStreamDetail *that);

  CStdString m_strLanguage;
};

class CStreamDetails : public IArchivable, public ISerializable
{
public:
  CStreamDetails() { Reset(); };
  CStreamDetails(const CStreamDetails &that);
  ~CStreamDetails() { Reset(); };
  CStreamDetails& operator=(const CStreamDetails &that);

  static CStdString VideoDimsToResolutionDescription(int iWidth, int iHeight);
  static CStdString VideoAspectToAspectDescription(float fAspect);

  bool HasItems(void) const { return m_vecItems.size() > 0; };
  int GetStreamCount(CStreamDetail::StreamType type) const;
  int GetVideoStreamCount(void) const;
  int GetAudioStreamCount(void) const;
  int GetSubtitleStreamCount(void) const;
  const CStreamDetail* GetNthStream(CStreamDetail::StreamType type, int idx) const;

  CStdString GetVideoCodec(int idx = 0) const;
  float GetVideoAspect(int idx = 0) const;
  int GetVideoWidth(int idx = 0) const;
  int GetVideoHeight(int idx = 0) const;
  int GetVideoDuration(int idx = 0) const;

  CStdString GetAudioCodec(int idx = 0) const;
  CStdString GetAudioLanguage(int idx = 0) const;
  int GetAudioChannels(int idx = 0) const;

  CStdString GetSubtitleLanguage(int idx = 0) const;

  void AddStream(CStreamDetail *item);
  void Reset(void);
  void DetermineBestStreams(void);

  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);

  // Language to use for "best" subtitle stream
  CStdString m_strLanguage;

private:
  CStreamDetail *NewStream(CStreamDetail::StreamType type);
  std::vector<CStreamDetail *> m_vecItems;
  CStreamDetailVideo *m_pBestVideo;
  CStreamDetailAudio *m_pBestAudio;
  CStreamDetailSubtitle *m_pBestSubtitle;
};

class IStreamDetailsObserver
{
public:
  virtual ~IStreamDetailsObserver() {}
  virtual void OnStreamDetails(const CStreamDetails &details, const CStdString &strFileName, long lFileId) = 0;
};
