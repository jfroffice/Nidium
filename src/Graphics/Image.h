/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef graphics_image_h__
#define graphics_image_h__

#include <SkBitmap.h>
#include <SkImage.h>

class SkCanvas;
class SkBitmap;
class SkData;

namespace Nidium {
namespace Graphics {

class Image
{
  public:
    int m_IsCanvas;
    SkCanvas *m_CanvasRef;
    SkBitmap *m_Image;
#if 0
    SkImage *fixedImg;
#endif
    Image(SkCanvas *canvas);
    Image(void *data, size_t len);
    Image(void *data, int width, int height);


    SkData *getPNG();

    static bool ConvertToRGBA(Image *nimg, unsigned char* rgba,
        bool flipY, bool premultiply);

    ~Image();

    const uint8_t *getPixels(size_t *len);

    int getWidth();
    int getHeight();
    void shiftHue(int val, U8CPU alpha);
    void markColorsInAlpha();
    void desaturate();
};

} // namespace Graphics
} // namespace Nidium

#endif

