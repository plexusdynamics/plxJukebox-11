

// AsyncAudioRenderer.h: interface for the CAsyncDirectSound class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __ALSA_DIRECT_SOUND_H__
#define __ALSA_DIRECT_SOUND_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IAudioRenderer.h"
#include "cores/IAudioCallback.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include "../../utils/PCMAmplifier.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CALSADirectSound : public IAudioRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual unsigned int GetChunkLen();
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();
  CALSADirectSound();
  virtual bool Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic=false, EEncoded encoded = IAudioRenderer::ENCODED_NONE);
  virtual ~CALSADirectSound();

  virtual unsigned int AddPackets(const void* data, unsigned int len);
  virtual unsigned int GetSpace();
  virtual bool Deinitialize();
  virtual bool Pause();
  virtual bool Stop();
  virtual bool Resume();

  virtual long GetCurrentVolume() const;
  virtual void Mute(bool bMute);
  virtual bool SetCurrentVolume(long nVolume);
  virtual void SetDynamicRangeCompression(long drc) { m_drc = drc; }
  virtual int SetPlaySpeed(int iSpeed);
  virtual void WaitCompletion();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);

  virtual void Flush();
  static void EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough);
private:
  unsigned int GetSpaceFrames();
  static void GenSoundLabel(AudioSinkList& vAudioSinks, CStdString sink, CStdString card, int dev, CStdString readableCard);
  snd_pcm_t 		*m_pPlayHandle;

  IAudioCallback* m_pCallback;
  CPCMAmplifier 	m_amp;
  long m_nCurrentVolume;
  long m_drc;
  snd_pcm_uframes_t m_dwFrameCount;
  snd_pcm_uframes_t m_uiBufferSize;
  unsigned int      m_dwNumPackets;
  bool m_bPause;
  bool m_bIsAllocated;
  bool m_bCanPause;

  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiDataChannels;
  unsigned int m_uiChannels;

  bool m_bPassthrough;
};

#endif

