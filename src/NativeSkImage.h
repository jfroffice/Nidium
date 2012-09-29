#ifndef nativeskimage_h__
#define nativeskimage_h__

#include "SkBitmap.h"
#include "SkImage.h"
/*
drawImage(R,9,2,282,148,0,3,300,150)
*/
class SkCanvas;
class SkBitmap;

class NativeSkImage
{	
  public:
  	int isCanvas;
  	SkCanvas *canvasRef;
  	SkBitmap img;
    SkImage *fixedImg;
  	NativeSkImage(SkCanvas *canvas);
  	NativeSkImage(const char *imgpath);
  	NativeSkImage(void *data, size_t len);
  	//~NativeSkImage();

  	int getWidth();
  	int getHeight();
};

#endif