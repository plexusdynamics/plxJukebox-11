

#include "threads/SingleLock.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayer.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDPerformanceCounter.h"
#include "settings/GUISettings.h"
#include "video/VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/MathUtils.h"

#include <sstream>
#include <iomanip>

using namespace std;

CPTSOutputQueue::CPTSOutputQueue()
{
  Flush();
}

void CPTSOutputQueue::Add(double pts, double delay, double duration)
{
  CSingleLock lock(m_sync);

  TPTSItem item;
  item.pts = pts;
  item.timestamp = CDVDClock::GetAbsoluteClock() + delay;
  item.duration = duration;

  // first one is applied directly
  if(m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
    m_current = item;
  else
    m_queue.push(item);

  // call function to make sure the queue
  // doesn't grow should nobody call it
  Current();
}
void CPTSOutputQueue::Flush()
{
  CSingleLock lock(m_sync);

  while( !m_queue.empty() ) m_queue.pop();
  m_current.pts = DVD_NOPTS_VALUE;
  m_current.timestamp = 0.0;
  m_current.duration = 0.0;
}

double CPTSOutputQueue::Current()
{
  CSingleLock lock(m_sync);

  if(!m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  while( !m_queue.empty() && CDVDClock::GetAbsoluteClock() >= m_queue.front().timestamp )
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  if( m_current.timestamp == 0 ) return m_current.pts;

  return m_current.pts + min(m_current.duration, (CDVDClock::GetAbsoluteClock() - m_current.timestamp));
}

void CPTSInputQueue::Add(__int64 bytes, double pts)
{
  CSingleLock lock(m_sync);

  m_list.insert(m_list.begin(), make_pair(bytes, pts));
}

void CPTSInputQueue::Flush()
{
  CSingleLock lock(m_sync);

  m_list.clear();
}
double CPTSInputQueue::Get(__int64 bytes, bool consume)
{
  CSingleLock lock(m_sync);

  IT it = m_list.begin();
  for(; it != m_list.end(); it++)
  {
    if(bytes <= it->first)
    {
      double pts = it->second;
      if(consume)
      {
        it->second = DVD_NOPTS_VALUE;
        m_list.erase(++it, m_list.end());
      }
      return pts;
    }
    bytes -= it->first;
  }
  return DVD_NOPTS_VALUE;
}


class CDVDMsgAudioCodecChange : public CDVDMsg
{
public:
  CDVDMsgAudioCodecChange(const CDVDStreamInfo &hints, CDVDAudioCodec* codec)
    : CDVDMsg(GENERAL_STREAMCHANGE)
    , m_codec(codec)
    , m_hints(hints)
  {}
 ~CDVDMsgAudioCodecChange()
  {
    delete m_codec;
  }
  CDVDAudioCodec* m_codec;
  CDVDStreamInfo  m_hints;
};


CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent)
: CThread("CDVDPlayerAudio")
, m_messageQueue("audio")
, m_messageParent(parent)
, m_dvdAudio((bool&)m_bStop)
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;
  m_audioClock = 0;
  m_droptime = 0;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_stalled = true;
  m_started = false;
  m_duration = 0.0;
  m_resampleratio = 1.0;

  m_freq = CurrentHostFrequency();

  m_messageQueue.SetMaxDataSize(6 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_messageQueue);
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  StopThread();
  g_dvdPerformanceCounter.DisableAudioQueue();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
}

bool CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints )
{
  bool passthrough = AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode"));

  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", hints.codec);
  CDVDAudioCodec* codec = CDVDFactoryCodec::CreateAudioCodec(hints, passthrough);
  if( !codec )
  {
    CLog::Log(LOGERROR, "Unsupported audio codec");
    return false;
  }

  if(m_messageQueue.IsInited())
    m_messageQueue.Put(new CDVDMsgAudioCodecChange(hints, codec), 0);
  else
  {
    OpenStream(hints, codec);
    m_messageQueue.Init();
    CLog::Log(LOGNOTICE, "Creating audio thread");
    Create();
  }
  return true;
}

void CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints, CDVDAudioCodec* codec )
{
  SAFE_DELETE(m_pAudioCodec);
  m_pAudioCodec = codec;

  /* store our stream hints */
  m_streaminfo = hints;

  /* update codec information from what codec gave ut */
  m_streaminfo.channels = m_pAudioCodec->GetChannels();
  m_streaminfo.samplerate = m_pAudioCodec->GetSampleRate();

  /* check if we only just got sample rate, in which case the previous call
   * to CreateAudioCodec() couldn't have started passthrough */
  if (hints.samplerate != m_streaminfo.samplerate)
    SwitchCodecIfNeeded();

  m_droptime = 0;
  m_audioClock = 0;
  m_stalled = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_started = false;

  m_synctype = SYNC_DISCON;
  m_setsynctype = g_guiSettings.GetInt("videoplayer.synctype");
  m_prevsynctype = -1;
  m_resampler.SetQuality(g_guiSettings.GetInt("videoplayer.resamplequality"));

  m_error = 0;
  m_errorbuff = 0;
  m_errorcount = 0;
  m_integral = 0;
  m_skipdupcount = 0;
  m_prevskipped = false;
  m_syncclock = true;
  m_errortime = CurrentHostCounter();
  m_silence = false;

  m_maxspeedadjust = g_guiSettings.GetFloat("videoplayer.maxspeedadjust");
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  // send abort message to the audio queue
  m_messageQueue.Abort();

  CLog::Log(LOGNOTICE, "Waiting for audio thread to exit");

  // shut down the adio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true

  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  if (bWaitForBuffers && m_speed > 0)
  {
    m_bStop = false;
    m_dvdAudio.Drain();
    m_bStop = true;
  }
  m_dvdAudio.Destroy();

  // uninit queue
  m_messageQueue.End();

  CLog::Log(LOGNOTICE, "Deleting audio codec");
  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }

  // flush any remaining pts values
  m_ptsOutput.Flush();
  m_resampler.Flush();
}

// decode one audio frame and returns its uncompressed size
int CDVDPlayerAudio::DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket)
{
  int result = 0;

  // make sure the sent frame is clean
  memset(&audioframe, 0, sizeof(DVDAudioFrame));

  while (!m_bStop)
  {
    bool switched = false;
    /* NOTE: the audio packet can contain several frames */
    while( !m_bStop && m_decode.size > 0 )
    {
      if( !m_pAudioCodec )
        return DECODE_FLAG_ERROR;

      /* the packet dts refers to the first audioframe that starts in the packet */
      double dts = m_ptsInput.Get(m_decode.size + m_pAudioCodec->GetBufferSize(), true);
      if (dts != DVD_NOPTS_VALUE)
        m_audioClock = dts;

      int len = m_pAudioCodec->Decode(m_decode.data, m_decode.size);
      m_audioStats.AddSampleBytes(m_decode.size);
      if (len < 0)
      {
        /* if error, we skip the packet */
        CLog::Log(LOGERROR, "CDVDPlayerAudio::DecodeFrame - Decode Error. Skipping audio packet");
        m_decode.Release();
        m_pAudioCodec->Reset();
        return DECODE_FLAG_ERROR;
      }

      // fix for fucked up decoders
      if( len > m_decode.size )
      {
        CLog::Log(LOGERROR, "CDVDPlayerAudio:DecodeFrame - Codec tried to consume more data than available. Potential memory corruption");
        m_decode.Release();
        m_pAudioCodec->Reset();
        assert(0);
      }

      m_decode.data += len;
      m_decode.size -= len;


      // get decoded data and the size of it
      audioframe.size = m_pAudioCodec->GetData(&audioframe.data);
      audioframe.pts = m_audioClock;
      audioframe.channel_map = m_pAudioCodec->GetChannelMap();
      audioframe.channels = m_pAudioCodec->GetChannels(); /* get channels AFTER map so that it can be corrected if bad */
      audioframe.bits_per_sample = m_pAudioCodec->GetBitsPerSample();
      audioframe.sample_rate = m_pAudioCodec->GetSampleRate();
      audioframe.passthrough = m_pAudioCodec->NeedPassthrough();

      if (audioframe.size <= 0)
        continue;

      if (m_streaminfo.samplerate != audioframe.sample_rate)
      {
        // The sample rate has changed or we just got it for the first time
        // for this stream. See if we should enable/disable passthrough due
        // to it.
        m_streaminfo.samplerate = audioframe.sample_rate;
        if (!switched && SwitchCodecIfNeeded()) {
          // passthrough has been enabled/disabled, reprocess the packet
          m_decode.data -= len;
          m_decode.size += len;
          switched = true;
          continue;
        }
      }

      // compute duration.
      int n = (audioframe.channels * audioframe.bits_per_sample * audioframe.sample_rate)>>3;
      if (n > 0)
      {
        // safety check, if channels == 0, n will result in 0, and that will result in a nice devide exception
        audioframe.duration = ((double)audioframe.size * DVD_TIME_BASE) / n;

        // increase audioclock to after the packet
        m_audioClock += audioframe.duration;
      }

      if(audioframe.duration > 0)
        m_duration = audioframe.duration;

      // if demux source want's us to not display this, continue
      if(m_decode.msg->GetPacketDrop())
        continue;

      //If we are asked to drop this packet, return a size of zero. then it won't be played
      //we currently still decode the audio.. this is needed since we still need to know it's
      //duration to make sure clock is updated correctly.
      if( bDropPacket )
        result |= DECODE_FLAG_DROP;

      return result;
    }
    // free the current packet
    m_decode.Release();

    if (m_messageQueue.ReceivedAbortRequest()) return DECODE_FLAG_ABORT;

    CDVDMsg* pMsg;
    int priority = (m_speed == DVD_PLAYSPEED_PAUSE && m_started) ? 1 : 0;

    int timeout;
    if(m_duration > 0)
      timeout = (int)(1000 * (m_duration / DVD_TIME_BASE + m_dvdAudio.GetCacheTime()));
    else
      timeout = 1000;

    // read next packet and return -1 on error
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, timeout, priority);

    if (ret == MSGQ_TIMEOUT)
      return DECODE_FLAG_TIMEOUT;

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT)
      return DECODE_FLAG_ABORT;

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      m_decode.Attach((CDVDMsgDemuxerPacket*)pMsg);
      m_ptsInput.Add( m_decode.size, m_decode.dts );
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      ((CDVDMsgGeneralSynchronize*)pMsg)->Wait( &m_bStop, SYNCSOURCE_AUDIO );
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;

      if (pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        m_audioClock = pMsgGeneralResync->m_timestamp;

      m_ptsOutput.Add(m_audioClock, m_dvdAudio.GetDelay(), 0);
      if (pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 1)", m_audioClock);
        m_pClock->Discontinuity(m_ptsOutput.Current());
      }
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 0)", m_audioClock);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      if (m_pAudioCodec)
        m_pAudioCodec->Reset();
      m_decode.Release();
      m_started = false;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      m_dvdAudio.Flush();
      m_ptsOutput.Flush();
      m_ptsInput.Flush();
      m_resampler.Flush();
      m_syncclock = true;
      m_stalled   = true;
      m_started   = false;

      if (m_pAudioCodec)
        m_pAudioCodec->Reset();

      m_decode.Release();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      if(m_started)
        m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_EOF");
      m_dvdAudio.Finish();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      if (m_speed != DVD_PLAYSPEED_PAUSE)
      {
        double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_DELAY(%f)", timeout);

        timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
        timeout += CDVDClock::GetAbsoluteClock();

        while(!m_bStop && CDVDClock::GetAbsoluteClock() < timeout)
          Sleep(1);
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;

      if (m_speed == DVD_PLAYSPEED_NORMAL)
      {
        m_dvdAudio.Resume();
      }
      else
      {
        m_ptsOutput.Flush();
        m_resampler.Flush();
        m_syncclock = true;
        if (m_speed != DVD_PLAYSPEED_PAUSE)
          m_dvdAudio.Flush();
        m_dvdAudio.Pause();
      }
    }
    else if (pMsg->IsType(CDVDMsg::AUDIO_SILENCE))
    {
      m_silence = static_cast<CDVDMsgBool*>(pMsg)->m_value;
      if (m_silence)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::AUDIO_SILENCE(%f, 1)", m_audioClock);
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::AUDIO_SILENCE(%f, 0)", m_audioClock);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      CDVDMsgAudioCodecChange* msg(static_cast<CDVDMsgAudioCodecChange*>(pMsg));
      OpenStream(msg->m_hints, msg->m_codec);
      msg->m_codec = NULL;
    }

    pMsg->Release();
  }
  return 0;
}

void CDVDPlayerAudio::OnStartup()
{
  m_decode.msg = NULL;
  m_decode.Release();

  g_dvdPerformanceCounter.EnableAudioDecodePerformance(ThreadHandle());

#ifdef _WIN32
  CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CDVDPlayerAudio::Process()");

  int result;
  bool packetadded(false);

  DVDAudioFrame audioframe;
  m_audioStats.Start();

  while (!m_bStop)
  {
    //Don't let anybody mess with our global variables
    result = DecodeFrame(audioframe, m_speed > DVD_PLAYSPEED_NORMAL || m_speed < 0); // blocks if no audio is available, but leaves critical section before doing so

    if( result & DECODE_FLAG_ERROR )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Decode Error");
      continue;
    }

    if( result & DECODE_FLAG_TIMEOUT )
    {
      m_stalled = true;

      // Flush as the audio output may keep looping if we don't
      if(m_speed == DVD_PLAYSPEED_NORMAL)
      {
        m_dvdAudio.Drain();
        m_dvdAudio.Flush();
      }

      continue;
    }

    if( result & DECODE_FLAG_ABORT )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Abort received, exiting thread");
      break;
    }

#ifdef PROFILE /* during profiling we just drop all packets, after having decoded */
    m_pClock->Discontinuity(audioframe.pts);
    continue;
#endif

    if( audioframe.size == 0 )
      continue;

    packetadded = true;

    // we have succesfully decoded an audio frame, setup renderer to match
    if (!m_dvdAudio.IsValidFormat(audioframe))
    {
      if(m_speed)
        m_dvdAudio.Drain();

      m_dvdAudio.Destroy();

      if(m_speed)
        m_dvdAudio.Resume();
      else
        m_dvdAudio.Pause();

      if(!m_dvdAudio.Create(audioframe, m_streaminfo.codec))
        CLog::Log(LOGERROR, "%s - failed to create audio renderer", __FUNCTION__);
    }

    // Zero out the frame data if we are supposed to silence the audio
    if (m_silence)
      memset(audioframe.data, 0, audioframe.size);

    if( result & DECODE_FLAG_DROP )
    {
      //frame should be dropped. Don't let audio move ahead of the current time thou
      //we need to be able to start playing at any time
      //when playing backwords, we try to keep as small buffers as possible

      if(m_droptime == 0.0)
        m_droptime = m_pClock->GetAbsoluteClock();
      if(m_speed > 0)
        m_droptime += audioframe.duration * DVD_PLAYSPEED_NORMAL / m_speed;
      while( !m_bStop && m_droptime > m_pClock->GetAbsoluteClock() ) Sleep(1);

      m_stalled = false;
    }
    else
    {
      m_droptime = 0.0;

      SetSyncType(audioframe.passthrough);

      // add any packets play
      packetadded = OutputPacket(audioframe);

      // we are not running until something is cached in output device
      if(m_stalled && m_dvdAudio.GetCacheTime() > 0.0)
        m_stalled = false;
    }

    // store the delay for this pts value so we can calculate the current playing
    if(packetadded)
    {
      if(m_speed == DVD_PLAYSPEED_PAUSE)
        m_ptsOutput.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration, 0);
      else
        m_ptsOutput.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration, audioframe.duration);
    }

    // signal to our parent that we have initialized
    if(m_started == false)
    {
      m_started = true;
      m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
    }

    if( m_ptsOutput.Current() == DVD_NOPTS_VALUE )
      continue;

    if( m_speed != DVD_PLAYSPEED_NORMAL )
      continue;

    if (packetadded)
      HandleSyncError(audioframe.duration);
  }
}

void CDVDPlayerAudio::SetSyncType(bool passthrough)
{
  //set the synctype from the gui
  //use skip/duplicate when resample is selected and passthrough is on
  m_synctype = m_setsynctype;
  if (passthrough && m_synctype == SYNC_RESAMPLE)
    m_synctype = SYNC_SKIPDUP;

  //tell dvdplayervideo how much it can change the speed
  //if SetMaxSpeedAdjust returns false, it means no video is played and we need to use clock feedback
  double maxspeedadjust = 0.0;
  if (m_synctype == SYNC_RESAMPLE)
    maxspeedadjust = m_maxspeedadjust;

  if (!m_pClock->SetMaxSpeedAdjust(maxspeedadjust))
    m_synctype = SYNC_DISCON;

  if (m_synctype != m_prevsynctype)
  {
    const char *synctypes[] = {"clock feedback", "skip/duplicate", "resample", "invalid"};
    int synctype = (m_synctype >= 0 && m_synctype <= 2) ? m_synctype : 3;
    CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: synctype set to %i: %s", m_synctype, synctypes[synctype]);
    m_prevsynctype = m_synctype;
  }

  CDVDClock::SetMasterClock(false);
}

void CDVDPlayerAudio::HandleSyncError(double duration)
{
  double clock = m_pClock->GetClock();
  double error = m_ptsOutput.Current() - clock;
  int64_t now;

  if( fabs(error) > DVD_MSEC_TO_TIME(100) || m_syncclock )
  {
    m_pClock->Discontinuity(clock+error);
    if(m_speed == DVD_PLAYSPEED_NORMAL)
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuity - was:%f, should be:%f, error:%f", clock, clock+error, error);

    m_errorbuff = 0;
    m_errorcount = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_syncclock = false;
    m_errortime = CurrentHostCounter();

    return;
  }

  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_errorbuff = 0;
    m_errorcount = 0;
    m_integral = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_resampler.Flush();
    m_errortime = CurrentHostCounter();
    return;
  }

  m_errorbuff += error;
  m_errorcount++;

  //check if measured error for 1 second
  now = CurrentHostCounter();
  if ((now - m_errortime) >= m_freq)
  {
    m_errortime = now;
    m_error = m_errorbuff / m_errorcount;

    m_errorbuff = 0;
    m_errorcount = 0;

    if (m_synctype == SYNC_DISCON)
    {
      double limit, error;
      if (g_VideoReferenceClock.GetRefreshRate(&limit) > 0)
      {
        //when the videoreferenceclock is running, the discontinuity limit is one vblank period
        limit *= DVD_TIME_BASE;

        //make error a multiple of limit, rounded towards zero,
        //so it won't interfere with the sync methods in CXBMCRenderManager::WaitPresentTime
        if (m_error > 0.0)
          error = limit * floor(m_error / limit);
        else
          error = limit * ceil(m_error / limit);
      }
      else
      {
        limit = DVD_MSEC_TO_TIME(10);
        error = m_error;
      }

      if (fabs(error) > limit - 0.001)
      {
        m_pClock->Discontinuity(clock+error);
        if(m_speed == DVD_PLAYSPEED_NORMAL)
          CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuity - was:%f, should be:%f, error:%f", clock, clock+error, error);
      }
    }
    else if (m_synctype == SYNC_SKIPDUP && m_skipdupcount == 0 && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    {
      //check how many packets to skip/duplicate
      m_skipdupcount = (int)(m_error / duration);
      //if less than one frame off, see if it's more than two thirds of a frame, so we can get better in sync
      if (m_skipdupcount == 0 && fabs(m_error) > duration / 3 * 2)
        m_skipdupcount = (int)(m_error / (duration / 3 * 2));

      if (m_skipdupcount > 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Duplicating %i packet(s) of %.2f ms duration",
                  m_skipdupcount, duration / DVD_TIME_BASE * 1000.0);
      else if (m_skipdupcount < 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Skipping %i packet(s) of %.2f ms duration ",
                  m_skipdupcount * -1,  duration / DVD_TIME_BASE * 1000.0);
    }
    else if (m_synctype == SYNC_RESAMPLE)
    {
      //reset the integral on big errors, failsafe
      if (fabs(m_error) > DVD_TIME_BASE)
        m_integral = 0;
      else if (fabs(m_error) > DVD_MSEC_TO_TIME(5))
        m_integral += m_error / DVD_TIME_BASE / INTEGRAL;
    }
  }
}

bool CDVDPlayerAudio::OutputPacket(DVDAudioFrame &audioframe)
{
  if (m_synctype == SYNC_DISCON)
  {
    m_dvdAudio.AddPackets(audioframe);
  }
  else if (m_synctype == SYNC_SKIPDUP)
  {
    if (m_skipdupcount < 0)
    {
      m_prevskipped = !m_prevskipped;
      if (!m_prevskipped)
      {
        m_dvdAudio.AddPackets(audioframe);
        m_skipdupcount++;
      }
    }
    else if (m_skipdupcount > 0)
    {
      m_dvdAudio.AddPackets(audioframe);
      m_dvdAudio.AddPackets(audioframe);
      m_skipdupcount--;
    }
    else if (m_skipdupcount == 0)
    {
      m_dvdAudio.AddPackets(audioframe);
    }
  }
  else if (m_synctype == SYNC_RESAMPLE)
  {
    double proportional = 0.0, proportionaldiv;

    //on big errors use more proportional
    if (fabs(m_error / DVD_TIME_BASE) > 0.0)
    {
      proportionaldiv = PROPORTIONAL * (PROPREF / fabs(m_error / DVD_TIME_BASE));
      if (proportionaldiv < PROPDIVMIN) proportionaldiv = PROPDIVMIN;
      else if (proportionaldiv > PROPDIVMAX) proportionaldiv = PROPDIVMAX;

      proportional = m_error / DVD_TIME_BASE / proportionaldiv;
    }
    m_resampleratio = 1.0 / g_VideoReferenceClock.GetSpeed() + proportional + m_integral;
    m_resampler.SetRatio(m_resampleratio);

    //add to the resampler
    m_resampler.Add(audioframe, audioframe.pts);
    //give any packets from the resampler to the audiorenderer
    bool packetadded = false;
    while(m_resampler.Retrieve(audioframe, audioframe.pts))
    {
      m_dvdAudio.AddPackets(audioframe);
      packetadded = true;
    }
    return packetadded;
  }

  return true;
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();

#ifdef _WIN32
  CoUninitialize();
#endif

  CLog::Log(LOGNOTICE, "thread end: CDVDPlayerAudio::OnExit()");
}

void CDVDPlayerAudio::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

void CDVDPlayerAudio::Flush()
{
  m_messageQueue.Flush();
  m_messageQueue.Put( new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

void CDVDPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();

  // make sure almost all has been rendered
  // leave 500ms to avound buffer underruns
  double delay = m_dvdAudio.GetCacheTime();
  if(delay > 0.5)
    Sleep((int)(1000 * (delay - 0.5)));
}

bool CDVDPlayerAudio::SwitchCodecIfNeeded()
{
  // check if passthrough is disabled
  if (!AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode")))
    return false;

  CLog::Log(LOGDEBUG, "CDVDPlayerAudio: Sample rate changed, checking for passthrough");
  CDVDAudioCodec *codec = CDVDFactoryCodec::CreateAudioCodec(m_streaminfo, true);
  if (!codec || codec->NeedPassthrough() == m_pAudioCodec->NeedPassthrough()) {
    // passthrough state has not changed
    delete codec;
    return false;
  }

  delete m_pAudioCodec;
  m_pAudioCodec = codec;
  return true;
}

string CDVDPlayerAudio::GetPlayerInfo()
{
  std::ostringstream s;
  s << "aq:"     << setw(2) << min(99,m_messageQueue.GetLevel() + MathUtils::round_int(100.0/8.0*m_dvdAudio.GetCacheTime())) << "%";
  s << ", kB/s:" << fixed << setprecision(2) << (double)GetAudioBitrate() / 1024.0;

  //print the inverse of the resample ratio, since that makes more sense
  //if the resample ratio is 0.5, then we're playing twice as fast
  if (m_synctype == SYNC_RESAMPLE)
    s << ", rr:" << fixed << setprecision(5) << 1.0 / m_resampleratio;

  s << ", att:" << fixed << setprecision(1) << log(GetCurrentAttenuation()) * 20.0f << " dB";

  return s.str();
}

int CDVDPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}

bool CDVDPlayerAudio::IsPassthrough() const
{
  return m_pAudioCodec && m_pAudioCodec->NeedPassthrough();
}
