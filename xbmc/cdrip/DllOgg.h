#pragma once



#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include <ogg/ogg.h>
#include "utils/log.h"
#include "DynamicDll.h"

class DllOggInterface
{
public:
  virtual int ogg_page_eos(ogg_page *og)=0;
  virtual int ogg_stream_init(ogg_stream_state *os, int serialno)=0;
  virtual int ogg_stream_clear(ogg_stream_state *os)=0;
  virtual int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og)=0;
  virtual int ogg_stream_flush(ogg_stream_state *os, ogg_page *og)=0;
  virtual int ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op)=0;
  virtual ~DllOggInterface() {}
};

class DllOgg : public DllDynamic, DllOggInterface
{
  DECLARE_DLL_WRAPPER(DllOgg, DLL_PATH_OGG)
  DEFINE_METHOD1(int, ogg_page_eos, (ogg_page *p1))
  DEFINE_METHOD2(int, ogg_stream_init, (ogg_stream_state *p1, int p2))
  DEFINE_METHOD1(int, ogg_stream_clear, (ogg_stream_state *p1))
  DEFINE_METHOD2(int, ogg_stream_pageout, (ogg_stream_state *p1, ogg_page *p2))
  DEFINE_METHOD2(int, ogg_stream_flush, (ogg_stream_state *p1, ogg_page *p2))
  DEFINE_METHOD2(int, ogg_stream_packetin, (ogg_stream_state *p1, ogg_packet *p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(ogg_page_eos)
    RESOLVE_METHOD(ogg_stream_init)
    RESOLVE_METHOD(ogg_stream_clear)
    RESOLVE_METHOD(ogg_stream_pageout)
    RESOLVE_METHOD(ogg_stream_flush)
    RESOLVE_METHOD(ogg_stream_packetin)
  END_METHOD_RESOLVE()
};
