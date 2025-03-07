

#include "system.h"

#if defined(HAS_GL) || HAS_GLES == 2
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"
#include "FrameBufferObject.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

#if HAS_GLES == 2
// For OpenGL ES2.0, FBO are not extensions but part of the API.
#define glGenFramebuffersEXT		glGenFramebuffers
#define glDeleteFramebuffersEXT		glDeleteFramebuffers
#define glFramebufferTexture2DEXT	glFramebufferTexture2D
#define glCheckFramebufferStatusEXT	glCheckFramebufferStatus
#define GL_COLOR_ATTACHMENT0_EXT	GL_COLOR_ATTACHMENT0
#define GL_FRAMEBUFFER_COMPLETE_EXT	GL_FRAMEBUFFER_COMPLETE
#endif

//////////////////////////////////////////////////////////////////////
// CFrameBufferObject
//////////////////////////////////////////////////////////////////////

CFrameBufferObject::CFrameBufferObject()
{
  m_fbo = 0;
  m_valid = false;
  m_supported = false;
  m_bound = false;
  m_texid = 0;
}

bool CFrameBufferObject::IsSupported()
{
  if(g_Windowing.IsExtSupported("GL_EXT_framebuffer_object"))
    m_supported = true;
  else
    m_supported = false;
  return m_supported;
}

bool CFrameBufferObject::Initialize()
{
  if (!IsSupported())
    return false;

  Cleanup();

  glGenFramebuffersEXT(1, &m_fbo);
  VerifyGLState();

  if (!m_fbo)
    return false;

  m_valid = true;
  return true;
}

void CFrameBufferObject::Cleanup()
{
  if (!IsValid())
    return;

  if (m_fbo)
    glDeleteFramebuffersEXT(1, &m_fbo);

  if (m_texid)
    glDeleteTextures(1, &m_texid);

  m_texid = 0;
  m_fbo = 0;
  m_valid = false;
  m_bound = false;
}

bool CFrameBufferObject::CreateAndBindToTexture(GLenum target, int width, int height, GLenum format,
                                                GLenum filter, GLenum clampmode)
{
  if (!IsValid())
    return false;

  if (m_texid)
    glDeleteTextures(1, &m_texid);

  glGenTextures(1, &m_texid);
  glBindTexture(target, m_texid);
  glTexImage2D(target, 0, format,  width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, clampmode);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, clampmode);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
  VerifyGLState();
  return BindToTexture(target, m_texid);
}

void CFrameBufferObject::SetFiltering(GLenum target, GLenum mode)
{
  glBindTexture(target, m_texid);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mode);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mode);
}

bool CFrameBufferObject::BindToTexture(GLenum target, GLuint texid)
{
  if (!IsValid())
    return false;

  m_bound = false;
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
  glBindTexture(target, texid);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, target, texid, 0);
  VerifyGLState();
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
  {
    VerifyGLState();
    return false;
  }
  m_bound = true;
  return true;
}

#endif
