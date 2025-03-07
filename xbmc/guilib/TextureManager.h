

/*!
\file TextureManager.h
\brief
*/

#ifndef GUILIB_TEXTUREMANAGER_H
#define GUILIB_TEXTUREMANAGER_H

#include <vector>
#include "TextureBundle.h"
#include "threads/CriticalSection.h"

#pragma once

/************************************************************************/
/*                                                                      */
/************************************************************************/
class CTextureArray
{
public:
  CTextureArray(int width, int height, int loops, bool texCoordsArePixels = false);
  CTextureArray();

  virtual ~CTextureArray();

  void Reset();

  void Add(CBaseTexture *texture, int delay);
  void Set(CBaseTexture *texture, int width, int height);
  void Free();
  unsigned int size() const;

  std::vector<CBaseTexture* > m_textures;
  std::vector<int> m_delays;
  int m_width;
  int m_height;
  int m_orientation;
  int m_loops;
  int m_texWidth;
  int m_texHeight;
  bool m_texCoordsArePixels;
};

/*!
 \ingroup textures
 \brief
 */
/************************************************************************/
/*                                                                      */
/************************************************************************/
class CTextureMap
{
public:
  CTextureMap();
  CTextureMap(const CStdString& textureName, int width, int height, int loops);
  virtual ~CTextureMap();

  void Add(CBaseTexture* texture, int delay);
  bool Release();

  const CStdString& GetName() const;
  const CTextureArray& GetTexture();
  void Dump() const;
  uint32_t GetMemoryUsage() const;
  void Flush();
  bool IsEmpty() const;
protected:
  void FreeTexture();

  CStdString m_textureName;
  CTextureArray m_texture;
  unsigned int m_referenceCount;
  uint32_t m_memUsage;
};

/*!
 \ingroup textures
 \brief
 */
/************************************************************************/
/*                                                                      */
/************************************************************************/
class CGUITextureManager
{
public:
  CGUITextureManager(void);
  virtual ~CGUITextureManager(void);

  bool HasTexture(const CStdString &textureName, CStdString *path = NULL, int *bundle = NULL, int *size = NULL);
  bool CanLoad(const CStdString &texturePath) const; ///< Returns true if the texture manager can load this texture
  int Load(const CStdString& strTextureName, bool checkBundleOnly = false);
  const CTextureArray& GetTexture(const CStdString& strTextureName);
  void ReleaseTexture(const CStdString& strTextureName);
  void Cleanup();
  void Dump() const;
  uint32_t GetMemoryUsage() const;
  void Flush();
  CStdString GetTexturePath(const CStdString& textureName, bool directory = false);
  void GetBundledTexturesFromPath(const CStdString& texturePath, std::vector<CStdString> &items);

  void AddTexturePath(const CStdString &texturePath);    ///< Add a new path to the paths to check when loading media
  void SetTexturePath(const CStdString &texturePath);    ///< Set a single path as the path to check when loading media (clear then add)
  void RemoveTexturePath(const CStdString &texturePath); ///< Remove a path from the paths to check when loading media

  void FreeUnusedTextures(); ///< Free textures (called from app thread only)
protected:
  std::vector<CTextureMap*> m_vecTextures;
  std::vector<CTextureMap*> m_unusedTextures;
  typedef std::vector<CTextureMap*>::iterator ivecTextures;
  // we have 2 texture bundles (one for the base textures, one for the theme)
  CTextureBundle m_TexBundle[2];

  std::vector<CStdString> m_texturePaths;
  CCriticalSection m_section;
};

/*!
 \ingroup textures
 \brief
 */
extern CGUITextureManager g_TextureManager;
#endif
