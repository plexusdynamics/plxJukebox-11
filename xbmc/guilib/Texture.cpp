

#include "Texture.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "pictures/DllImageLib.h"
#include "DDSImage.h"
#include "filesystem/SpecialProtocol.h"
#include "JpegIO.h"
#if defined(__APPLE__) && defined(__arm__)
#include <ImageIO/ImageIO.h>
#include "filesystem/File.h"
#include "osx/DarwinUtils.h"
#endif

/************************************************************************/
/*                                                                      */
/************************************************************************/
CBaseTexture::CBaseTexture(unsigned int width, unsigned int height, unsigned int format)
 : m_hasAlpha( true )
{
#ifndef HAS_DX 
  m_texture = 0; 
#endif
  m_pixels = NULL;
  m_loadedToGPU = false;
  Allocate(width, height, format);
}

CBaseTexture::~CBaseTexture()
{
  delete[] m_pixels;
}

void CBaseTexture::Allocate(unsigned int width, unsigned int height, unsigned int format)
{
  m_imageWidth = width;
  m_imageHeight = height;
  m_format = format;
  m_orientation = 0;

  m_textureWidth = m_imageWidth;
  m_textureHeight = m_imageHeight;

  if (m_format & XB_FMT_DXT_MASK)
    while (GetPitch() < g_Windowing.GetMinDXTPitch())
      m_textureWidth += GetBlockSize();

  if (!g_Windowing.SupportsNPOT((m_format & XB_FMT_DXT_MASK) != 0))
  {
    m_textureWidth = PadPow2(m_textureWidth);
    m_textureHeight = PadPow2(m_textureHeight);
  }
  if (m_format & XB_FMT_DXT_MASK)
  { // DXT textures must be a multiple of 4 in width and height
    m_textureWidth = ((m_textureWidth + 3) / 4) * 4;
    m_textureHeight = ((m_textureHeight + 3) / 4) * 4;
  }

  // check for max texture size
  #define CLAMP(x, y) { if (x > y) x = y; }
  CLAMP(m_textureWidth, g_Windowing.GetMaxTextureSize());
  CLAMP(m_textureHeight, g_Windowing.GetMaxTextureSize());
  CLAMP(m_imageWidth, m_textureWidth);
  CLAMP(m_imageHeight, m_textureHeight);
  delete[] m_pixels;
  m_pixels = new unsigned char[GetPitch() * GetRows()];
}

void CBaseTexture::Update(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU)
{
  if (pixels == NULL)
    return;

  if (format & XB_FMT_DXT_MASK && !g_Windowing.SupportsDXT())
  { // compressed format that we don't support
    Allocate(width, height, XB_FMT_A8R8G8B8);
    CDDSImage::Decompress(m_pixels, std::min(width, m_textureWidth), std::min(height, m_textureHeight), GetPitch(m_textureWidth), pixels, format);
  }
  else
  {
    Allocate(width, height, format);

    unsigned int srcPitch = pitch ? pitch : GetPitch(width);
    unsigned int srcRows = GetRows(height);
    unsigned int dstPitch = GetPitch(m_textureWidth);
    unsigned int dstRows = GetRows(m_textureHeight);

    if (srcPitch == dstPitch)
      memcpy(m_pixels, pixels, srcPitch * std::min(srcRows, dstRows));
    else
    {
      const unsigned char *src = pixels;
      unsigned char* dst = m_pixels;
      for (unsigned int y = 0; y < srcRows && y < dstRows; y++)
      {
        memcpy(dst, src, std::min(srcPitch, dstPitch));
        src += srcPitch;
        dst += dstPitch;
      }
    }
  }
  ClampToEdge();

  if (loadToGPU)
    LoadToGPU();
}

void CBaseTexture::ClampToEdge()
{
  unsigned int imagePitch = GetPitch(m_imageWidth);
  unsigned int imageRows = GetRows(m_imageHeight);
  unsigned int texturePitch = GetPitch(m_textureWidth);
  unsigned int textureRows = GetRows(m_textureHeight);
  if (imagePitch < texturePitch)
  {
    unsigned int blockSize = GetBlockSize();
    unsigned char *src = m_pixels + imagePitch - blockSize;
    unsigned char *dst = m_pixels;
    for (unsigned int y = 0; y < imageRows; y++)
    {
      for (unsigned int x = imagePitch; x < texturePitch; x += blockSize)
        memcpy(dst + x, src, blockSize);
      dst += texturePitch;
    }
  }

  if (imageRows < textureRows)
  {
    unsigned char *dst = m_pixels + imageRows * texturePitch;
    for (unsigned int y = imageRows; y < textureRows; y++)
    {
      memcpy(dst, dst - texturePitch, texturePitch);
      dst += texturePitch;
    }
  }
}

bool CBaseTexture::LoadFromFile(const CStdString& texturePath, unsigned int maxWidth, unsigned int maxHeight,
                                bool autoRotate, unsigned int *originalWidth, unsigned int *originalHeight)
{
  if (URIUtils::GetExtension(texturePath).Equals(".dds"))
  { // special case for DDS images
    CDDSImage image;
    if (image.ReadFile(texturePath))
    {
      Update(image.GetWidth(), image.GetHeight(), 0, image.GetFormat(), image.GetData(), false);
      return true;
    }
    return false;
  }

  //ImageLib is sooo sloow for jpegs. Try our own decoder first. If it fails, fall back to ImageLib.
  if (URIUtils::GetExtension(texturePath).Equals(".jpg") || URIUtils::GetExtension(texturePath).Equals(".tbn"))
  {
    CJpegIO jpegfile;
    if (jpegfile.Open(texturePath))
    {
      if (jpegfile.Width() > 0 && jpegfile.Height() > 0)
      {
        Allocate(jpegfile.Width(), jpegfile.Height(), XB_FMT_A8R8G8B8);
        if (jpegfile.Decode(m_pixels, GetPitch(), XB_FMT_A8R8G8B8))
        {
          if (autoRotate && jpegfile.Orientation())
            m_orientation = jpegfile.Orientation() - 1;
          m_hasAlpha=false;
          return true;
        }
      }
    }
  }

  DllImageLib dll;
  if (!dll.Load())
    return false;

  ImageInfo image;
  memset(&image, 0, sizeof(image));

  unsigned int width = maxWidth ? std::min(maxWidth, g_Windowing.GetMaxTextureSize()) : g_Windowing.GetMaxTextureSize();
  unsigned int height = maxHeight ? std::min(maxHeight, g_Windowing.GetMaxTextureSize()) : g_Windowing.GetMaxTextureSize();

  if(!dll.LoadImage(texturePath.c_str(), width, height, &image))
  {
    CLog::Log(LOGERROR, "Texture manager unable to load file: %s", texturePath.c_str());
    return false;
  }

  m_hasAlpha = NULL != image.alpha;

  Allocate(image.width, image.height, XB_FMT_A8R8G8B8);
  if (autoRotate && image.exifInfo.Orientation)
    m_orientation = image.exifInfo.Orientation - 1;
  if (originalWidth)
    *originalWidth = image.originalwidth;
  if (originalHeight)
    *originalHeight = image.originalheight;

  unsigned int dstPitch = GetPitch();
  unsigned int srcPitch = ((image.width + 1)* 3 / 4) * 4; // bitmap row length is aligned to 4 bytes

  unsigned char *dst = m_pixels;
  unsigned char *src = image.texture + (m_imageHeight - 1) * srcPitch;

  for (unsigned int y = 0; y < m_imageHeight; y++)
  {
    unsigned char *dst2 = dst;
    unsigned char *src2 = src;
    for (unsigned int x = 0; x < m_imageWidth; x++, dst2 += 4, src2 += 3)
    {
      dst2[0] = src2[0];
      dst2[1] = src2[1];
      dst2[2] = src2[2];
      dst2[3] = 0xff;
    }
    src -= srcPitch;
    dst += dstPitch;
  }

  if(image.alpha)
  {
    dst = m_pixels + 3;
    src = image.alpha + (m_imageHeight - 1) * m_imageWidth;

    for (unsigned int y = 0; y < m_imageHeight; y++)
    {
      unsigned char *dst2 = dst;
      unsigned char *src2 = src;

      for (unsigned int x = 0; x < m_imageWidth; x++,  dst2+=4, src2++)
        *dst2 = *src2;
      src -= m_imageWidth;
      dst += dstPitch;
    }
  }
  dll.ReleaseImage(&image);

  ClampToEdge();

  return true;
}

bool CBaseTexture::LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, bool hasAlpha, unsigned char* pixels)
{
  m_imageWidth = width;
  m_imageHeight = height;
  m_format = format;
  m_hasAlpha = hasAlpha;
  Update(width, height, pitch, format, pixels, false);
  return true;
}

bool CBaseTexture::LoadPaletted(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, const COLOR *palette)
{
  if (pixels == NULL || palette == NULL)
    return false;

  Allocate(width, height, format);

  for (unsigned int y = 0; y < m_imageHeight; y++)
  {
    unsigned char *dest = m_pixels + y * GetPitch();
    const unsigned char *src = pixels + y * pitch;
    for (unsigned int x = 0; x < m_imageWidth; x++)
    {
      COLOR col = palette[*src++];
      *dest++ = col.b;
      *dest++ = col.g;
      *dest++ = col.r;
      *dest++ = col.x;
    }
  }
  ClampToEdge();
  return true;
}

unsigned int CBaseTexture::PadPow2(unsigned int x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

bool CBaseTexture::SwapBlueRed(unsigned char *pixels, unsigned int height, unsigned int pitch, unsigned int elements, unsigned int offset)
{
  if (!pixels) return false;
  unsigned char *dst = pixels;
  for (unsigned int y = 0; y < height; y++)
  {
    dst = pixels + (y * pitch);
    for (unsigned int x = 0; x < pitch; x+=elements)
      std::swap(dst[x+offset], dst[x+2+offset]);
  }
  return true;
}

unsigned int CBaseTexture::GetPitch(unsigned int width) const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return ((width + 3) / 4) * 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return ((width + 3) / 4) * 16;
  case XB_FMT_A8:
    return width;
  case XB_FMT_RGB8:
    return (((width + 1)* 3 / 4) * 4);
  case XB_FMT_RGBA8:
  case XB_FMT_A8R8G8B8:
  default:
    return width*4;
  }
}

unsigned int CBaseTexture::GetRows(unsigned int height) const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return (height + 3) / 4;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return (height + 3) / 4;
  default:
    return height;
  }
}

unsigned int CBaseTexture::GetBlockSize() const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return 16;
  case XB_FMT_A8:
    return 1;
  default:
    return 4;
  }
}

bool CBaseTexture::HasAlpha() const
{
  return m_hasAlpha;
}
