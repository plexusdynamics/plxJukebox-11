#ifndef SPC_CODEC_H_
#define SPC_CODEC_H_



#include "ICodec.h"
#include "snesapu/Types.h"
#include "../DllLoader/LibraryLoader.h"

class SPCCodec : public ICodec
{
public:
  SPCCodec();
  virtual ~SPCCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
#ifdef _LINUX
  typedef void  (__cdecl *LoadMethod) ( const void* p1);
  typedef void* (__cdecl *EmuMethod) ( void *p1, u32 p2, u32 p3);
  typedef void  (__cdecl *SeekMethod) ( u32 p1, b8 p2 );
  typedef u32 (__cdecl *InitMethod)(void);
  typedef void (__cdecl *DeInitMethod)(void);
#else
  typedef void  (__stdcall* LoadMethod) ( const void* p1);
  typedef void* (__stdcall * EmuMethod) ( void *p1, u32 p2, u32 p3);
  typedef void  (__stdcall * SeekMethod) ( u32 p1, b8 p2 );
#endif
  struct
  {
    LoadMethod LoadSPCFile;
    EmuMethod EmuAPU;
    SeekMethod SeekAPU;
#ifdef _LINUX
    InitMethod InitAPU;
    DeInitMethod ResetAPU;
#endif
  } m_dll;

  LibraryLoader* m_loader;
  CStdString m_loader_name;

  char* m_szBuffer;
  u8* m_pApuRAM;
  __int64 m_iDataPos;
};

#endif
