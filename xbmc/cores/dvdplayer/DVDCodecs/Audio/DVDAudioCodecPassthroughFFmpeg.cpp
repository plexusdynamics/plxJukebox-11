

#include "DVDAudioCodecPassthroughFFmpeg.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include "Encoders/DVDAudioEncoderFFmpeg.h"

//These values are forced to allow spdif out
#define OUT_SAMPLESIZE 16
#define OUT_CHANNELS   2
#define OUT_SAMPLERATE 48000

/* Lookup tables */
static const uint16_t AC3Bitrates[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod   [] = {48000, 44100, 32000, 0};

static const uint16_t DTSFSCod   [] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0};

#define NULL_MUXER(muxer) \
  muxer.m_pFormat    = NULL; \
  muxer.m_pStream    = NULL; \
  muxer.m_OutputSize = 0; \
  muxer.m_Consumed   = 0; \
  muxer.m_Buffer     = NULL; \
  muxer.m_BufferSize = 0;

CDVDAudioCodecPassthroughFFmpeg::CDVDAudioCodecPassthroughFFmpeg(void)
{
  NULL_MUXER(m_SPDIF);
  NULL_MUXER(m_ADTS );

  m_pSyncFrame   = NULL;
  m_Needed       = 0;
  m_NeededUsed   = 0;
  m_SampleRate   = 0;

  m_Codec        = NULL;
  m_Encoder      = NULL;
  m_InitEncoder  = true;
  
  /* make enough room for at-least two audio frames */
  m_DecodeSize   = 0;
  m_DecodeBuffer = NULL;
}

CDVDAudioCodecPassthroughFFmpeg::~CDVDAudioCodecPassthroughFFmpeg(void)
{
  Dispose();
}

/*===================== MUXER FUNCTIONS ========================*/
bool CDVDAudioCodecPassthroughFFmpeg::SetupMuxer(CDVDStreamInfo &hints, CStdString muxerName, Muxer &muxer)
{
  CLog::Log(LOGINFO, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - Trying to setup %s muxer", muxerName.c_str());

  /* get the muxer */
  AVOutputFormat *fOut = NULL;

#if LIBAVFORMAT_VERSION_MAJOR < 52
  fOut = m_dllAvFormat.guess_format(muxerName.c_str(), NULL, NULL);
#else
  fOut = m_dllAvFormat.av_guess_format(muxerName.c_str(), NULL, NULL);
#endif
  if (!fOut)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - Failed to get the FFmpeg %s muxer", muxerName.c_str());
    Dispose();
    return false;
  }

  /* allocate a the format context */
  muxer.m_pFormat = m_dllAvFormat.avformat_alloc_context();
  if (!muxer.m_pFormat)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - Failed to allocate AVFormat context");
    Dispose();
    return false;
  }

  muxer.m_pFormat->oformat = fOut;

  /* allocate a put_byte struct so we can grab the output */
  muxer.m_pFormat->pb = m_dllAvFormat.av_alloc_put_byte(muxer.m_BCBuffer, sizeof(muxer.m_BCBuffer), URL_RDONLY, &muxer,  NULL, MuxerReadPacket, NULL);
  if (!muxer.m_pFormat->pb)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - Failed to allocate ByteIOContext");
    Dispose();
    return false;
  }

  /* this is streamed, no file, and ignore the index */
  muxer.m_pFormat->pb->is_streamed   = 1;
  muxer.m_pFormat->flags            |= AVFMT_NOFILE | AVFMT_FLAG_IGNIDX;
  muxer.m_pFormat->bit_rate          = hints.bitrate;

  /* setup the muxer */
  if (m_dllAvFormat.av_set_parameters(muxer.m_pFormat, NULL) != 0)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - Failed to set the %s muxer parameters", muxerName.c_str());
    Dispose();
    return false;
  }

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,92,0)
  // API added on: 2011-01-02

  /* While this is strictly only needed on big-endian systems, we do it on
   * both to avoid as much dead code as possible.
   * CoreAudio (at least on the cases we've seen) wants IEC 61937 in
   * little-endian format even on big-endian systems. */
#if defined(WORDS_BIGENDIAN) && !defined(__APPLE__)
  const char *spdifFlags = "+be";
#else
  const char *spdifFlags = "-be";
#endif

  /* request output of wanted endianness */
  if (!fOut->priv_class || m_dllAvUtil.av_set_string3(muxer.m_pFormat->priv_data, "spdif_flags", spdifFlags, 0, NULL) != 0)
#endif
  {
#if defined(WORDS_BIGENDIAN) && !defined(__APPLE__)
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - Unable to set big-endian stream mode (FFmpeg too old?), disabling passthrough");
    Dispose();
    return false;
#endif
  }

  /* add a stream to it */
  muxer.m_pStream = m_dllAvFormat.av_new_stream(muxer.m_pFormat, 1);
  if (!muxer.m_pStream)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - Failed to allocate AVStream context");
    Dispose();
    return false;
  }


  /* set the stream's parameters */
  muxer.m_pStream->stream_copy           = 1;

  m_SampleRate = hints.samplerate;
  if(!m_SampleRate && hints.codec == CODEC_ID_AC3)
    m_SampleRate = 48000;

  AVCodecContext *codec = muxer.m_pStream->codec;
  codec->codec_type     = AVMEDIA_TYPE_AUDIO;
  codec->codec_id       = hints.codec;
  codec->sample_rate    = m_SampleRate;
  codec->sample_fmt     = AV_SAMPLE_FMT_S16;
  codec->channels       = hints.channels;
  codec->bit_rate       = hints.bitrate;
  codec->extradata      = new uint8_t[hints.extrasize];
  codec->extradata_size = hints.extrasize;
  memcpy(codec->extradata, hints.extradata, hints.extrasize);

  muxer.m_WroteHeader = m_dllAvFormat.av_write_header(muxer.m_pFormat) == 0;
  if (!muxer.m_WroteHeader)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::SetupMuxer - Failed to write the frame header");
    return false;
  }

  CLog::Log(LOGINFO, "CDVDAudioCodecPassthroughFFmpeg::SetupMuxer - %s muxer ready", muxerName.c_str());
  return true;
}

int CDVDAudioCodecPassthroughFFmpeg::MuxerReadPacket(void *opaque, uint8_t *buf, int buf_size)
{
  /* create a new packet and push it into our output buffer */
  DataPacket *packet = new DataPacket();
  packet->size       = buf_size;
  packet->data       = new uint8_t[buf_size];
  memcpy(packet->data, buf, buf_size);

  Muxer *muxer = (Muxer*)opaque;
  muxer->m_OutputBuffer.push_back(packet);
  muxer->m_OutputSize += buf_size;

  /* return how much we wrote to our buffer */
  return buf_size;
}

void CDVDAudioCodecPassthroughFFmpeg::WriteFrame(Muxer &muxer, uint8_t *pData, int iSize)
{
  AVPacket pkt;
  m_dllAvCodec.av_init_packet(&pkt);
  pkt.data = pData;
  pkt.size = iSize;

  muxer.m_Consumed += iSize;
  if (m_dllAvFormat.av_write_frame(muxer.m_pFormat, &pkt) < 0)
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::WriteFrame - Failed to write the frame data");
}

int CDVDAudioCodecPassthroughFFmpeg::GetMuxerData(Muxer &muxer, uint8_t** dst)
{
  int size;
  if(muxer.m_OutputSize)
  {
    /* check if the buffer is allocated */
    if (muxer.m_Buffer)
    {
      /* only re-allocate the buffer it is too small */
      if (muxer.m_BufferSize < muxer.m_OutputSize)
      {
        delete[] muxer.m_Buffer;
        muxer.m_Buffer = new uint8_t[muxer.m_OutputSize];
        muxer.m_BufferSize = muxer.m_OutputSize;
      }
    }
    else
    {
      /* allocate the buffer */
      muxer.m_Buffer     = new uint8_t[muxer.m_OutputSize];
      muxer.m_BufferSize = muxer.m_OutputSize;
    }

    /* fill the buffer with the output data */
    uint8_t *offset;
    offset = muxer.m_Buffer;
    while(!muxer.m_OutputBuffer.empty())
    {
      DataPacket* packet = muxer.m_OutputBuffer.front();
      muxer.m_OutputBuffer.pop_front();

      memcpy(offset, packet->data, packet->size);
      offset += packet->size;

      delete[] packet->data;
      delete   packet;
    }

    *dst = muxer.m_Buffer;
    size = muxer.m_OutputSize;
    muxer.m_OutputSize = 0;
    muxer.m_Consumed = 0;
    return size;
  }
  else
    return 0;
}

void CDVDAudioCodecPassthroughFFmpeg::ResetMuxer(Muxer &muxer)
{
  muxer.m_OutputSize = 0;
  muxer.m_Consumed   = 0;
  while(!muxer.m_OutputBuffer.empty())
  {
    DataPacket* packet = muxer.m_OutputBuffer.front();
    muxer.m_OutputBuffer.pop_front();
    delete[] packet->data;
    delete   packet;
  }
}

void CDVDAudioCodecPassthroughFFmpeg::DisposeMuxer(Muxer &muxer)
{
  ResetMuxer(muxer);
  delete[] muxer.m_Buffer;
  muxer.m_Buffer     = NULL;
  muxer.m_BufferSize = 0;
  if (muxer.m_pFormat)
  {
    if (muxer.m_WroteHeader)
      m_dllAvFormat.av_write_trailer(muxer.m_pFormat);
    muxer.m_WroteHeader = false;
    if (muxer.m_pStream)
      delete[] muxer.m_pStream->codec->extradata;
    m_dllAvUtil.av_freep(&muxer.m_pFormat->pb);
    m_dllAvUtil.av_freep(&muxer.m_pFormat);
    m_dllAvUtil.av_freep(&muxer.m_pStream);
  }
}
/*===================== END MUXER FUNCTIONS ========================*/

bool CDVDAudioCodecPassthroughFFmpeg::SupportsFormat(CDVDStreamInfo &hints)
{
  m_pSyncFrame = NULL;

       if (m_bSupportsAC3Out && hints.codec == CODEC_ID_AC3) m_pSyncFrame = &CDVDAudioCodecPassthroughFFmpeg::SyncAC3;
  else if (m_bSupportsDTSOut && hints.codec == CODEC_ID_DTS) m_pSyncFrame = &CDVDAudioCodecPassthroughFFmpeg::SyncDTS;
  else if (m_bSupportsAACOut && hints.codec == CODEC_ID_AAC) m_pSyncFrame = &CDVDAudioCodecPassthroughFFmpeg::SyncAAC;
  else if (m_bSupportsMP1Out && hints.codec == CODEC_ID_MP1);
  else if (m_bSupportsMP2Out && hints.codec == CODEC_ID_MP2);
  else if (m_bSupportsMP3Out && hints.codec == CODEC_ID_MP3);
  else return false;

  return true;
}

bool CDVDAudioCodecPassthroughFFmpeg::SetupEncoder(CDVDStreamInfo &hints)
{
  /* there is no point encoding <= 2 channel sources, and we dont support anything but AC3 at the moment */
  if (hints.channels <= 2 || !m_bSupportsAC3Out)
    return false;

  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughFFmpeg::SetupEncoder - Setting up encoder for on the fly transcode");

  /* we need to decode the incoming audio data, so we can re-encode it as we need it */
  CDVDStreamInfo h(hints);
  m_Codec = CDVDFactoryCodec::CreateAudioCodec(h, false);
  if (!m_Codec)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupEncoder - Unable to create a decoder for transcode");
    return false;
  }

  /* create and setup the encoder */
  m_Encoder     = new CDVDAudioEncoderFFmpeg();
  m_InitEncoder = true;

  /* adjust the hints according to the encorders output */
  hints.codec    = m_Encoder->GetCodecID();
  hints.bitrate  = m_Encoder->GetBitRate();
  hints.channels = OUT_CHANNELS;

  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughFFmpeg::SetupEncoder - Ready to transcode");
  return true;
}

bool CDVDAudioCodecPassthroughFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  int audioMode = g_guiSettings.GetInt("audiooutput.mode");

  // TODO - move this stuff somewhere else
  if (AUDIO_IS_BITSTREAM(audioMode))
  {
    m_bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    m_bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
    m_bSupportsAACOut = g_guiSettings.GetBool("audiooutput.passthroughaac");
    m_bSupportsMP1Out = g_guiSettings.GetBool("audiooutput.passthroughmp1");
    m_bSupportsMP2Out = g_guiSettings.GetBool("audiooutput.passthroughmp2");
    m_bSupportsMP3Out = g_guiSettings.GetBool("audiooutput.passthroughmp3");
  }
  else
    return false;

  // TODO - this is only valid for video files, and should be moved somewhere else
  if( hints.channels == 2 && g_settings.m_currentVideoSettings.m_OutputToAllSpeakers )
  {
    CLog::Log(LOGINFO, "CDVDAudioCodecPassthroughFFmpeg::Open - disabled passthrough due to video OTAS");
    return false;
  }

  // TODO - some soundcards do support other sample rates, but they are quite uncommon
  if( hints.samplerate > 0 && hints.samplerate != 48000 )
  {
    CLog::Log(LOGINFO, "CDVDAudioCodecPassthrough::Open - disabled passthrough due to sample rate not being 48000");
    return false;
  }

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load())
    return false;

  m_dllAvFormat.av_register_all();

  /* see if the muxer supports our codec (see spdif.c for supported formats) */
  if (!SupportsFormat(hints))
  {
    /* HDMI can do multichannel LPCM, transcoding would just be silly */
    if (audioMode == AUDIO_HDMI)
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecPassthroughFFmpeg::Open - Won't transcode for HDMI");
      Dispose();
      return false;
    }

    if (!SetupEncoder(hints) || !SupportsFormat(hints))
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - FFmpeg SPDIF muxer does not support this codec");
      Dispose();
      return false;
    }
  }
  else
  {
    /* aac needs to be wrapped into ADTS frames */
    if (hints.codec == CODEC_ID_AAC)
      if (!SetupMuxer(hints, "adts", m_ADTS))
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - Unable to setup ADTS muxer");
        Dispose();
        return false;
      }

    m_Codec   = NULL;
    m_Encoder = NULL;
  }

  if (!SetupMuxer(hints, "spdif", m_SPDIF))
    return false;

  Reset();

  /* we will check the first packet's crc */
  m_LostSync = true;
  return true;
}

void CDVDAudioCodecPassthroughFFmpeg::Dispose()
{
  Reset();

  DisposeMuxer(m_SPDIF);
  DisposeMuxer(m_ADTS );

  if (m_DecodeBuffer)
  {
    _aligned_free(m_DecodeBuffer);
    m_DecodeBuffer = NULL;
  }

  delete m_Encoder;
  m_Encoder = NULL;

  delete m_Codec;
  m_Codec   = NULL;
}

int CDVDAudioCodecPassthroughFFmpeg::Decode(BYTE* pData, int iSize)
{
  unsigned int used, fSize;
  fSize = iSize;

  /* if we are transcoding */
  if (m_Encoder)
  {
    uint8_t *decData;
    uint8_t *encData;

    /* if we are decoding */
    used  = m_Codec->Decode (pData, iSize);
    fSize = m_Codec->GetData(&decData);

    /* we may not get any data for a few frames, this is expected */
    if (fSize == 0)
      return used;

    /* now we have data, it is safe to initialize the encoder, as we should now have a channel map */
    if (m_InitEncoder)
    {
      if (m_Encoder->Initialize(m_Codec->GetChannels(), m_Codec->GetChannelMap(), m_Codec->GetBitsPerSample(), m_Codec->GetSampleRate()))
      {
        m_InitEncoder   = false;
        m_EncPacketSize = m_Encoder->GetPacketSize();
        /* allocate enough room for two packets of data */
        m_DecodeBuffer  = (uint8_t*)_aligned_malloc(m_EncPacketSize * 2, 16);
        m_DecodeSize    = 0;
      }
      else
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Encode - Unable to initialize the encoder for transcode");
        return -1;
      }
    }

    unsigned int avail, eUsed, eCoded = 0;
    avail = fSize + m_DecodeSize;

    while(avail >= m_EncPacketSize)
    {
      /* append up to one packet of data to the buffer */
      if (m_DecodeSize < m_EncPacketSize)
      {
        unsigned int copy = (fSize > m_EncPacketSize) ? m_EncPacketSize : fSize;
        if (copy)
        {
          memcpy(m_DecodeBuffer + m_DecodeSize, decData, copy);
          m_DecodeSize += copy;
          decData      += copy;
          fSize        -= copy;
        }
      }

      /* encode the data and advance our data pointer */
      eUsed  = m_Encoder->Encode(m_DecodeBuffer, m_EncPacketSize);
      avail -= eUsed;

      /* shift buffered data along with memmove as the data can overlap */
      m_DecodeSize -= eUsed;
      memmove(m_DecodeBuffer, m_DecodeBuffer + eUsed, m_DecodeSize);

      /* output the frame of data */
      while((eCoded = m_Encoder->GetData(&encData)))
        WriteFrame(m_SPDIF, encData, eCoded);
    }

    /* append any leftover data to the buffer */
    memcpy(m_DecodeBuffer + m_DecodeSize, decData, fSize);
    m_DecodeSize += fSize;

    return used;
  }

  /* if we are muxing into ADTS (AAC) */
  int adts_used = 0;
  if (m_ADTS.m_pFormat)
  {
    adts_used = iSize;
    WriteFrame(m_ADTS, pData, iSize);
    iSize = GetMuxerData(m_ADTS, &pData);
  }

  used = 0;
  while(iSize > 0)
  {
    /* skip data until we can sync and know how much we need */
    if (m_Needed == 0)
    {
      /* if we have a sync function for this codec */
      if (m_pSyncFrame)
      {
	int skip = (this->*m_pSyncFrame)(pData, iSize, &m_Needed);
	if (skip > 0)
	{
	  /* we lost sync, so invalidate our buffer */
	  m_NeededUsed = 0;
	  return used + skip;
	}
      }
      else
        m_Needed = iSize;
    }

    if(m_SPDIF.m_pStream->codec->sample_rate != m_SampleRate)
    {
     CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughFFmpeg::Decode - stream changed sample rate from %d to %d"
                       , m_SPDIF.m_pStream->codec->sample_rate
                       , m_SampleRate);
     m_SPDIF.m_pStream->codec->sample_rate = m_SampleRate;
    }

    /* check for bad parsing */
    assert(m_Needed > 0);

    /* append one block or less of data */
    int copy;
    int room = sizeof(m_NeededBuffer) - m_NeededUsed;
    int need = m_Needed - m_NeededUsed;
    copy = room  > need ? need : room;
    copy = iSize > copy ? copy : iSize;
    memcpy(m_NeededBuffer + m_NeededUsed, pData, copy);

    m_NeededUsed += copy;
    used         += copy;
    iSize        -= copy;
    pData        += copy;

    /* if we have enough data in the buffer, write it out */
    if (m_NeededUsed == m_Needed)
    {
      WriteFrame(m_SPDIF, m_NeededBuffer, m_Needed);
      m_NeededUsed = 0;
      m_Needed     = 0;
    }
  }

  /* return how much data we copied */
  if (m_ADTS.m_pFormat)
    return adts_used;
  else
    return used;
}

int CDVDAudioCodecPassthroughFFmpeg::GetData(BYTE** dst)
{
  return GetMuxerData(m_SPDIF, dst);
}

void CDVDAudioCodecPassthroughFFmpeg::Reset()
{
  m_DecodeSize = 0;
  m_LostSync   = true;
  m_Needed     = 0;
  m_NeededUsed = 0;

  ResetMuxer(m_SPDIF);
  ResetMuxer(m_ADTS );

  if (m_Encoder)
    m_Encoder->Reset();
}

int CDVDAudioCodecPassthroughFFmpeg::GetChannels()
{
  //Can't return correct channels here as this is used to keep sync.
  //should probably have some other way to find out this
  return OUT_CHANNELS;
}

int CDVDAudioCodecPassthroughFFmpeg::GetSampleRate()
{
  return m_SPDIF.m_pStream->codec->sample_rate;
}

int CDVDAudioCodecPassthroughFFmpeg::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

int CDVDAudioCodecPassthroughFFmpeg::GetBufferSize()
{
  if (m_Codec)
    return m_Codec->GetBufferSize();
  else
    return m_SPDIF.m_Consumed + m_ADTS.m_Consumed + m_NeededUsed;
}

/* ========================== SYNC FUNCTIONS ========================== */
unsigned int CDVDAudioCodecPassthroughFFmpeg::SyncAC3(BYTE* pData, unsigned int iSize, unsigned int *fSize)
{
  unsigned int skip = 0;
  for(skip = 0; iSize - skip > 6; ++skip, ++pData)
  {
    /* search for an ac3 sync word */
    if(pData[0] != 0x0b || pData[1] != 0x77)
      continue;
 
    uint8_t fscod      = pData[4] >> 6;
    uint8_t frmsizecod = pData[4] & 0x3F;
    uint8_t bsid       = pData[5] >> 3;

    /* sanity checks on the header */
    if (
        fscod      ==   3 ||
        frmsizecod >   37 ||
        bsid       > 0x11
    ) continue;

    /* get the details we need to check crc1 and framesize */
    uint16_t     bitrate   = AC3Bitrates[frmsizecod >> 1];
    unsigned int framesize = 0;
    switch(fscod)
    {
      case 0: framesize = bitrate * 2; break;
      case 1: framesize = (320 * bitrate / 147 + (frmsizecod & 1 ? 1 : 0)); break;
      case 2: framesize = bitrate * 4; break;
    }

    *fSize = framesize * 2;
    m_SampleRate = AC3FSCod[fscod];

    /* dont do extensive testing if we have not lost sync */
    if (!m_LostSync && skip == 0)
      return 0;

    unsigned int crc_size;
    /* if we have enough data, validate the entire packet, else try to validate crc2 (5/8 of the packet) */
    if (framesize <= iSize - skip)
         crc_size = framesize - 1;
    else crc_size = (framesize >> 1) + (framesize >> 3) - 1;

    if (crc_size <= iSize - skip)
      if(m_dllAvUtil.av_crc(m_dllAvUtil.av_crc_get_table(AV_CRC_16_ANSI), 0, &pData[2], crc_size * 2))
        continue;

    /* if we get here, we can sync */
    m_LostSync = false;
    return skip;
  }

  /* if we get here, the entire packet is invalid and we have lost sync */
  m_LostSync = true;
  return iSize;
}

unsigned int CDVDAudioCodecPassthroughFFmpeg::SyncDTS(BYTE* pData, unsigned int iSize, unsigned int *fSize)
{
  unsigned int skip;
  unsigned int srCode;
  bool littleEndian;

  for(skip = 0; iSize - skip > 8; ++skip, ++pData)
  {
    /* 16bit le */ if (pData[0] == 0x7F && pData[1] == 0xFE && pData[2] == 0x80 && pData[3] == 0x01                                                 ) littleEndian = true ; else
    /* 14bit le */ if (pData[0] == 0x1F && pData[1] == 0xFF && pData[2] == 0xE8 && pData[3] == 0x00 && pData[4] == 0x07 && (pData[5] & 0xF0) == 0xF0) littleEndian = true ; else
    /* 16bit be */ if (pData[1] == 0x7F && pData[0] == 0xFE && pData[3] == 0x80 && pData[2] == 0x01                                                 ) littleEndian = false; else
    /* 14bit be */ if (pData[1] == 0x1F && pData[0] == 0xFF && pData[3] == 0xE8 && pData[2] == 0x00 && pData[5] == 0x07 && (pData[4] & 0xF0) == 0xF0) littleEndian = false; else
      continue;

    if (littleEndian)
    {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((pData[4] & 0x80) == 0x80 && (pData[4] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      *fSize = ((((pData[5] & 0x3) << 8 | pData[6]) << 4) | ((pData[7] & 0xF0) >> 4)) + 1;
      srCode = (pData[8] & 0x3C) >> 2;
   }
   else
   {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((pData[5] & 0x80) == 0x80 && (pData[5] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      *fSize = ((((pData[4] & 0x3) << 8 | pData[7]) << 4) | ((pData[6] & 0xF0) >> 4)) + 1;
      srCode = (pData[9] & 0x3C) >> 2;
   }

    /* make sure the framesize is sane */
    if (*fSize < 96 || *fSize > 16384)
      continue;

    m_SampleRate = DTSFSCod[srCode];
    m_LostSync = false;
    return skip;
  }

  m_LostSync = true;
  return iSize;
}

unsigned int CDVDAudioCodecPassthroughFFmpeg::SyncAAC(BYTE* pData, unsigned int iSize, unsigned int *fSize)
{
  unsigned int skip;
  for(skip = 0; iSize - skip > 5; ++skip, ++pData)
  {
    if (pData[0] != 0xFF || (pData[1] & 0xF0) != 0xF0)
      continue;

    *fSize = (pData[3] & 0x03) << 11 | pData[4] << 3 | (pData[5] & 0xE0) >> 5;
    if (*fSize < 7)
      continue;

    m_LostSync = false;
    return skip;
  }

  m_LostSync = true;
  return iSize;
}
/* ========================== END SYNC FUNCTIONS ========================== */
