

#ifndef RENDER_SYSTEM_GL_H
#define RENDER_SYSTEM_GL_H

#pragma once

#include "system.h"
#include "rendering/RenderSystem.h"

class CRenderSystemGL : public CRenderSystemBase
{
public:
  CRenderSystemGL();
  virtual ~CRenderSystemGL();
  virtual void CheckOpenGLQuirks();
  virtual bool InitRenderSystem();
  virtual bool DestroyRenderSystem();
  virtual bool ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate);

  virtual bool BeginRender();
  virtual bool EndRender();
  virtual bool PresentRender(const CDirtyRegionList& dirty);
  virtual bool ClearBuffers(color_t color);
  virtual bool IsExtSupported(const char* extension);

  virtual void SetVSync(bool vsync);

  virtual void SetViewPort(CRect& viewPort);
  virtual void GetViewPort(CRect& viewPort);

  virtual void SetScissors(const CRect &rect);
  virtual void ResetScissors();

  virtual void CaptureStateBlock();
  virtual void ApplyStateBlock();

  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight);

  virtual void ApplyHardwareTransform(const TransformMatrix &matrix);
  virtual void RestoreHardwareTransform();

  virtual bool TestRender();

  virtual void Project(float &x, float &y, float &z);

  virtual void GetGLSLVersion(int& major, int& minor);

  virtual void ResetGLErrors();

protected:
  virtual void SetVSyncImpl(bool enable) = 0;
  virtual bool PresentRenderImpl(const CDirtyRegionList& dirty) = 0;
  void CalculateMaxTexturesize();

  int        m_iVSyncMode;
  int        m_iVSyncErrors;
  int64_t    m_iSwapStamp;
  int64_t    m_iSwapRate;
  int64_t    m_iSwapTime;
  bool       m_bVsyncInit;
  int        m_width;
  int        m_height;

  CStdString m_RenderExtensions;

  int        m_glslMajor;
  int        m_glslMinor;
  
#ifdef HAS_GL
  GLdouble   m_view[16];
  GLdouble   m_projection[16];
  GLint      m_viewPort[4];
#endif
};

#endif // RENDER_SYSTEM_H
