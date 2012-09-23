

#include "NativeSkia.h"
#include "NativeSkGradient.h"
#include "NativeSkImage.h"
#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkGpuDevice.h"
#include "SkBlurDrawLooper.h"
#include "SkConfig8888.h"
//#include "SkGLCanvas.h"

#include "SkParse.h"

#include "GrContext.h"
#include "SkTypeface.h"

#include "gl/GrGLInterface.h"
#include "gl/GrGLUtil.h"
#include "GrRenderTarget.h"

#include "GrGLRenderTarget.h"


#include "SkGpuCanvas.h"
#include "gl/SkNativeGLContext.h"

#include "SkGraphics.h"
#include "SkXfermode.h"

#include "NativeShadowLooper.h"


//#define CANVAS_FLUSH() canvas->flush()
#define CANVAS_FLUSH()

static int count_separators(const char* str, const char* sep) {
    char c;
    int separators = 0;
    while ((c = *str++) != '\0') {
        if (strchr(sep, c) == NULL)
            continue;
        do {
            if ((c = *str++) == '\0')
            goto goHome;
        } while (strchr(sep, c) != NULL);
        separators++;
    }
    goHome:
    return separators;
}


static inline double calcHue(double temp1, double temp2, double hueVal) {
  if (hueVal < 0.0)
    hueVal++;
  else if (hueVal > 1.0)
    hueVal--;

  if (hueVal * 6.0 < 1.0)
    return temp1 + (temp2 - temp1) * hueVal * 6.0;
  if (hueVal * 2.0 < 1.0)
    return temp2;
  if (hueVal * 3.0 < 2.0)
    return temp1 + (temp2 - temp1) * (2.0 / 3.0 - hueVal) * 6.0;

  return temp1;
}


int NativeSkia::getWidth()
{
    return canvas->getDeviceSize().fWidth;
}

int NativeSkia::getHeight()
{
    return canvas->getDeviceSize().fHeight;
}

static U8CPU InvScaleByte(U8CPU component, uint32_t scale)
{
    SkASSERT(component == (uint8_t)component);
    return (component * scale + 0x8000) >> 16;
}


static SkColor SkPMColorToColor(SkPMColor pm)
{
    if (!pm)
        return 0;
    unsigned a = SkGetPackedA32(pm);
    if (!a) {
        // A zero alpha value when there are non-zero R, G, or B channels is an
        // invalid premultiplied color (since all channels should have been
        // multiplied by 0 if a=0).
        SkASSERT(false);
        // In production, return 0 to protect against division by zero.
        return 0;
    }
   
    uint32_t scale = (255 << 16) / a;
   
    return SkColorSetARGB(a,
                          InvScaleByte(SkGetPackedR32(pm), scale),
                          InvScaleByte(SkGetPackedG32(pm), scale),
                          InvScaleByte(SkGetPackedB32(pm), scale));
}


SkPMColor NativeSkia::HSLToSKColor(U8CPU alpha, float hsl[3])
{
  double hue = SkScalarToDouble(hsl[0]);
  double saturation = SkScalarToDouble(hsl[1]);
  double lightness = SkScalarToDouble(hsl[2]);
  double scaleFactor = 256.0;
  
  // If there's no color, we don't care about hue and can do everything based
  // on brightness.
  if (!saturation) {
    U8CPU lightness;

    if (hsl[2] < 0)
      lightness = 0;
    else if (hsl[2] >= SK_Scalar1)
      lightness = 255;
    else
      lightness = SkScalarToFixed(hsl[2]) >> 8;

    unsigned greyValue = SkAlphaMul(lightness, alpha);
    return SkColorSetARGB(alpha, greyValue, greyValue, greyValue);
  }

  double temp2 = (lightness < 0.5) ?
      lightness * (1.0 + saturation) :
      lightness + saturation - (lightness * saturation);
  double temp1 = 2.0 * lightness - temp2;

  double rh = calcHue(temp1, temp2, hue + 1.0 / 3.0);
  double gh = calcHue(temp1, temp2, hue);
  double bh = calcHue(temp1, temp2, hue - 1.0 / 3.0);

  return SkColorSetARGB(alpha,
      SkAlphaMul(static_cast<int>(rh * scaleFactor), alpha),
      SkAlphaMul(static_cast<int>(gh * scaleFactor), alpha),
      SkAlphaMul(static_cast<int>(bh * scaleFactor), alpha));
}

/* TODO: Only accept ints int rgb(a)() */
uint32_t NativeSkia::parseColor(const char *str)
{
    SkColor color = SK_ColorBLACK;
    if (strncasecmp(str, "rgb", 3) == 0) {
        SkScalar array[4];

        int count = count_separators(str, ",") + 1;
        
        if (count == 4) {
            if (str[3] != 'a') {
                count = 3;
            }
        } else if (count != 3) {
            return 0;
        } 

        array[3] = SK_Scalar1;
        
        const char* end = SkParse::FindScalars(&str[(str[3] == 'a' ? 5 : 4)],
            array, count);

        if (end == NULL) {
            return 0;
        }

        array[3] *= 255;
        for (int c = 0; c <= 3; c++) {
            array[c] = SkMaxScalar(SkMinScalar(array[c], 255), 0);
        }

        color = SkColorSetARGB(SkScalarRound(array[3]), SkScalarRound(array[0]),
        SkScalarRound(array[1]), SkScalarRound(array[2]));

    } else if (strncasecmp(str, "hsl", 3) == 0) {
        SkScalar array[4];
        
        int count = count_separators(str, ",") + 1;
        
        if (count == 4) {
            if (str[3] != 'a') {
                count = 3;
            }
        } else if (count != 3) {
            return 0;
        } 
        array[3] = SK_Scalar1;

        const char* end = SkParse::FindScalars(&str[(str[3] == 'a' ? 5 : 4)],
            array, count);

        if (end == NULL) printf("Not found\n");
        else {
            SkScalar final[4];

            /* TODO: limits? */
            final[0] = SkScalarDiv(array[0], SkIntToScalar(360));
            final[1] = SkScalarDiv(array[1], SkIntToScalar(100));
            final[2] = SkScalarDiv(array[2], SkIntToScalar(100));
            final[3] = SkScalarMul(array[3], SkIntToScalar(255));

            SkPMColor hsl = HSLToSKColor(final[3], final);
            //printf("Got an HSL : %f %f %f %d\n", array[0], array[1], array[2], hsl);
            return SkPMColorToColor(hsl);

        }

    } else {
        SkParse::FindColor(str, &color);
    }

    return color;
}

int NativeSkia::bindOffScreen(int width, int height)
{
    SkBitmap bitmap;

    bitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
    bitmap.allocPixels();

    canvas = new SkCanvas(bitmap);

    /* TODO: Move the following in a common methode (init) */
    globalAlpha = 255;
    currentPath = NULL;

    paint = new SkPaint;

    memset(&currentShadow, 0, sizeof(NativeShadow_t));
    currentShadow.color = SkColorSetARGB(255, 0, 0, 0);

    paint->setARGB(255, 0, 0, 0);
    paint->setAntiAlias(true);

    paint->setStyle(SkPaint::kFill_Style);
    paint->setFilterBitmap(true);
 
    paint->setSubpixelText(true);
    paint->setAutohinted(true);

    paint_system = new SkPaint;

    paint_system->setARGB(255, 255, 0, 0);
    paint_system->setAntiAlias(true);
    //paint_system->setLCDRenderText(true);
    paint_system->setStyle(SkPaint::kFill_Style);
    paint->setSubpixelText(true);
    paint->setAutohinted(true);
   
    paint_stroke = new SkPaint;

    paint_stroke->setARGB(255, 0, 0, 0);
    paint_stroke->setAntiAlias(true);
    //paint_stroke->setLCDRenderText(true);
    paint_stroke->setStyle(SkPaint::kStroke_Style);
    
    this->setLineWidth(1);


    asComposite = 0;


    return 0;
}

int NativeSkia::bindGL(int width, int height)
{
    const GrGLInterface *interface =  GrGLCreateNativeInterface();
    
    if (interface == NULL) {
        printf("Cant get interface\n");
        return 0;
    }
    
    context = GrContext::Create(kOpenGL_Shaders_GrEngine,
        (GrPlatform3DContext)interface);

    if (context == NULL) {
        printf("Cant get context\n");
    }
    

    GrPlatformRenderTargetDesc desc;
    //GrGLRenderTarget *t = new GrGLRenderTarget();
    
    desc.fWidth = SkScalarRound(width);
    desc.fHeight = SkScalarRound(height);

    desc.fConfig = kSkia8888_PM_GrPixelConfig;

    GR_GL_GetIntegerv(interface, GR_GL_SAMPLES, &desc.fSampleCnt);
    GR_GL_GetIntegerv(interface, GR_GL_STENCIL_BITS, &desc.fStencilBits);

    GrGLint buffer = 0;
    GR_GL_GetIntegerv(interface, GR_GL_FRAMEBUFFER_BINDING, &buffer);
    desc.fRenderTargetHandle = buffer;

    printf("Samples : %d\n", desc.fSampleCnt);
 
    GrRenderTarget * target = context->createPlatformRenderTarget(desc);
    if (target == NULL) {
        printf("Failed to init Skia\n");
        return 0;
    }
    SkGpuDevice *dev = new SkGpuDevice(context, target);
    if (dev == NULL) {
        printf("Failed to init Skia (2)\n");
        return 0;
    }

    globalAlpha = 255;

    canvas = new SkCanvas(dev);
    
    SkSafeUnref(dev);

    glClearColor(1, 1, 1, 0);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    currentPath = NULL;

    paint = new SkPaint;

    memset(&currentShadow, 0, sizeof(NativeShadow_t));
    currentShadow.color = SkColorSetARGB(255, 0, 0, 0);

    paint->setARGB(255, 0, 0, 0);
    paint->setAntiAlias(true);
    
    //paint->setLCDRenderText(true);
    paint->setStyle(SkPaint::kFill_Style);
    paint->setFilterBitmap(true);
    //paint->setXfermodeMode(SkXfermode::kSrcOver_Mode);
    paint->setSubpixelText(true);
    paint->setAutohinted(true);

    paint_system = new SkPaint;

    paint_system->setARGB(255, 255, 0, 0);
    paint_system->setAntiAlias(true);
    //paint_system->setLCDRenderText(true);
    paint_system->setStyle(SkPaint::kFill_Style);
    paint->setSubpixelText(true);
    paint->setAutohinted(true);
   
    paint_stroke = new SkPaint;

    paint_stroke->setARGB(255, 0, 0, 0);
    paint_stroke->setAntiAlias(true);
    //paint_stroke->setLCDRenderText(true);
    paint_stroke->setStyle(SkPaint::kStroke_Style);
    
    this->setLineWidth(1);
    /*SkRect r;
    r.setXYWH(SkIntToScalar(0), SkIntToScalar(0),
        SkIntToScalar(640), SkIntToScalar(480));
    canvas->drawRect(r, cleared);*/

    asComposite = 0;

    screen = new SkBitmap();
    screen->setConfig(SkBitmap::kARGB_8888_Config, 640, 480);
    screen->allocPixels();

    //canvas->drawARGB(0, 0, 0, 0, SkXfermode::kClear_Mode);

#if 0
    SkIRect rr;
    SkPaint p;
    p.setColor(SK_ColorRED);
    canvas->saveLayer(NULL, NULL, SkCanvas::kARGB_ClipLayer_SaveFlag);
    rr.setXYWH(100, 0, 100, 100);
    canvas->drawIRect(rr, p);
    //canvas->flush();
    //canvas->restore();

    //canvas->saveLayer(NULL, NULL, SkCanvas::kARGB_ClipLayer_SaveFlag);
    SkPaint p2;
    p.setColor(SK_ColorBLUE);
    p.setXfermodeMode(SkXfermode::kDstOver_Mode);
    rr.setXYWH(150, 50, 100, 100);
    canvas->drawIRect(rr, p); 
    //canvas->flush();
    canvas->restore();
#endif
    //canvas->drawColor(SkColorSetRGB(255, 0, 0));
    /* TODO: stroke miter? */

    #if 0
    SkString path("skhello.png");
    //Set Text To Draw
    SkString text("Native Studio");
    
    SkPaint paint;
    
    //Set Text ARGB Color
    paint.setARGB(255, 255, 255, 255);
    
    //Turn AntiAliasing On
    
    paint.setAntiAlias(true);
    paint.setLCDRenderText(true);
    paint.setTypeface(SkTypeface::CreateFromName("Courier new bold", SkTypeface::kNormal));
    
    //Set Text Size
    paint.setTextSize(SkIntToScalar(40));

    canvas->drawARGB(255, 255, 255, 255);
    
    //Text X, Y Position Varibles
    int x = 80;
    int y = 60;
    
    canvas->drawText(text.c_str(), text.size(), x, y, paint);
    
    //Set Style and Stroke Width
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3);
    
    //Draw A Rectangle
    SkRect rect;
    paint.setARGB(255, 255, 255, 255);
    //Left, Top, Right, Bottom
    rect.set(50, 100, 200, 200);
    canvas->drawRoundRect(rect, 20, 20, paint);
    
    canvas->drawOval(rect, paint);
    
    //Draw A Line
    canvas->drawLine(10, 300, 300, 300, paint);
    
    //Draw Circle (X, Y, Size, Paint)
    canvas->drawCircle(100, 400, 50, paint);
    CANVAS_FLUSH();
#endif
    return 1;
}

void NativeSkia::drawRect(double x, double y, double width,
    double height, double stroke)
{
    canvas->drawRectCoords(SkDoubleToScalar(x), SkDoubleToScalar(y),
        SkDoubleToScalar(width), SkDoubleToScalar(height),
        (stroke ? *paint_stroke : *paint));

    CANVAS_FLUSH();

}

NativeSkia::~NativeSkia()
{
    delete paint;
    delete paint_stroke;
    delete paint_system;
    delete screen;
    

    if (currentPath) delete currentPath;

    delete canvas;
}

/* TODO: check if there is a best way to do this;
    context->clear() ?
*/
void NativeSkia::clearRect(int x, int y, int width, int height)
{
/*
    SkPaint paint;
    platformContext()->setupPaintForFilling(&paint);
    paint.setXfermodeMode(SkXfermode::kClear_Mode);
*/
    //CANVAS_FLUSH();
    //glClear(GL_COLOR_BUFFER_BIT);
    SkPaint clearPaint;
    
    clearPaint.setStyle(SkPaint::kFill_Style);
    clearPaint.setARGB(0,0,0,0);
    clearPaint.setXfermodeMode(SkXfermode::kClear_Mode);

    canvas->drawRectCoords(SkIntToScalar(x), SkIntToScalar(y),
        SkIntToScalar(width), SkIntToScalar(height), clearPaint);

    CANVAS_FLUSH();

}

void NativeSkia::setFontSize(double size)
{
    SkScalar ssize = SkDoubleToScalar(size);
    paint->setTextSize(ssize);
    paint_stroke->setTextSize(ssize);
}

void NativeSkia::setFontType(const char *str)
{
    SkTypeface *tf = SkTypeface::CreateFromName(str,
        SkTypeface::kNormal);

    paint->setTypeface(tf)->unref();
    paint_stroke->setTypeface(tf);
}

/* TODO: bug with alpha */
void NativeSkia::drawText(const char *text, int x, int y)
{
    canvas->drawText(text, strlen(text),
        SkIntToScalar(x), SkIntToScalar(y), *paint);

    CANVAS_FLUSH();
}

void NativeSkia::system(const char *text, int x, int y)
{
    canvas->drawText(text, strlen(text),
        SkIntToScalar(x), SkIntToScalar(y), *paint_system);

    CANVAS_FLUSH();
}

void NativeSkia::setFillColor(NativeSkGradient *gradient)
{ 
    SkShader *shader;

    if ((shader = gradient->build()) == NULL) {
        /* Make paint invalid (no future draw) */
        //paint->setShader(NULL);
        return;
    }
    paint->setColor(SK_ColorBLACK);
    paint->setShader(gradient->build()); /* TODO: SafeUnref(setShader())? */
    paint->setAlpha(SkAlphaMul(paint->getAlpha(),
        SkAlpha255To256(globalAlpha)));
}

void NativeSkia::setStrokeColor(NativeSkGradient *gradient)
{ 
    SkShader *shader;

    if ((shader = gradient->build()) == NULL) {
        return;
    }
    paint_stroke->setColor(SK_ColorBLACK);
    paint_stroke->setShader(gradient->build()); 
    paint_stroke->setAlpha(SkAlphaMul(paint_stroke->getAlpha(),
        SkAlpha255To256(globalAlpha)));

}

NativeShadowLooper *NativeSkia::buildShadow()
{
    if (currentShadow.blur == 0) {
        return NULL;
    }

    currentShadow.color = SkColorSetA(currentShadow.color,
        SkAlphaMul(SkColorGetA(currentShadow.color),
            SkAlpha255To256(globalAlpha)));

    return new NativeShadowLooper (SkDoubleToScalar(currentShadow.blur),
                                SkDoubleToScalar(currentShadow.x),
                                SkDoubleToScalar(currentShadow.y),
                                currentShadow.color,
                                SkBlurDrawLooper::kIgnoreTransform_BlurFlag |
                                SkBlurDrawLooper::kHighQuality_BlurFlag );
}

void NativeSkia::setShadowOffsetX(double x)
{
    currentShadow.x = x;
    SkSafeUnref(paint->setLooper(buildShadow()));
}

void NativeSkia::setShadowOffsetY(double y)
{
    currentShadow.y = y;
    SkSafeUnref(paint->setLooper(buildShadow()));
}

void NativeSkia::setShadowBlur(double blur)
{
    currentShadow.blur = blur;

    SkSafeUnref(paint->setLooper(buildShadow()));
}

void NativeSkia::setShadowColor(const char *str)
{
    SkColor color = parseColor(str);

    currentShadow.color = color;
    SkSafeUnref(paint->setLooper(buildShadow()));
}

void NativeSkia::setShadow()
{
    SkBlurDrawLooper *shadown =  new SkBlurDrawLooper (SkIntToScalar(10), SkIntToScalar(10),
                              SkIntToScalar(10), 0xFFFF0000,
                              SkBlurDrawLooper::kIgnoreTransform_BlurFlag |
                              SkBlurDrawLooper::kOverrideColor_BlurFlag |
                              SkBlurDrawLooper::kHighQuality_BlurFlag );


    paint->setLooper(shadown)->unref();
}

/* TODO : move color logic to a separate function */
void NativeSkia::setFillColor(const char *str)
{   
    SkColor color = parseColor(str);

    SkShader *shader = paint->getShader();

    if (shader) {
        paint->setShader(NULL);
    }

    paint->setColor(color);

    paint->setAlpha(SkAlphaMul(paint->getAlpha(),
        SkAlpha255To256(globalAlpha)));
    //printf("Setting alpha to : %d\n", paint->getAlpha());
}

void NativeSkia::setStrokeColor(const char *str)
{   
    SkColor color = parseColor(str);

    SkShader *shader = paint_stroke->getShader();

    if (shader) {
        paint_stroke->setShader(NULL);
    }

    paint_stroke->setColor(color);

    paint_stroke->setAlpha(SkAlphaMul(paint_stroke->getAlpha(),
        SkAlpha255To256(globalAlpha)));

}

void NativeSkia::setGlobalAlpha(double value)
{
    /*
        TODO : replace by :
                //The SrcIn xfer mode will multiply 'color' by the incoming alpha
        fColorFilter = SkColorFilter::CreateModeFilter(opaqueColor,
                                                       SkXfermode::kSrcIn_Mode);
        paint->setColorMask

        OR

        setColorFilter?
    */
    if (value < 0) return;

    SkScalar maxuint = SkIntToScalar(255);
    globalAlpha = SkMinScalar(SkDoubleToScalar(value) * maxuint, maxuint);

    paint->setAlpha(globalAlpha);
    paint_stroke->setAlpha(globalAlpha);
}


static SkXfermode::Mode lst[] = {
       SkXfermode::kDstOut_Mode, SkXfermode::kSrcOver_Mode,/* 0, 1 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kDarken_Mode,/* 2, 3 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 4, 5 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 6, 7 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 8, 9 */
       SkXfermode::kDstOver_Mode, SkXfermode::kDstIn_Mode,/* 10, 11 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 12, 13 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 14, 15 */
       SkXfermode::kLighten_Mode, SkXfermode::kSrcOver_Mode,/* 16, 17 */
       SkXfermode::kSrcATop_Mode, SkXfermode::kSrcOver_Mode,/* 18, 19 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 20, 21 */
       SkXfermode::kDstATop_Mode, SkXfermode::kOverlay_Mode,/* 22, 23 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 24, 25 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 26, 27 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kSrcOver_Mode,/* 28, 29 */
       SkXfermode::kSrcOver_Mode, SkXfermode::kXor_Mode,/* 30, 31 */
};

void NativeSkia::setGlobalComposite(const char *str)
{
    int sum = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        sum += str[i];
    }

    //canvas->drawColor(SkColor color)
    printf("Set to : %d %d\n", lst[sum % 35], sum % 35);
    //SkXfermode *xfer = SkXfermode::Create(SkXfermode::kDstOver_Mode);
    //paint->setAlpha(0);
    paint->setAlpha(0);
    paint->setXfermodeMode(SkXfermode::kDstOver_Mode);
    
    asComposite = 1;

}

void NativeSkia::setLineWidth(double size)
{
    paint_stroke->setStrokeWidth(SkDoubleToScalar(size));
}

void NativeSkia::beginPath()
{
    if (currentPath) {
        delete currentPath;
    }

    currentPath = new SkPath();

    //currentPath->moveTo(SkIntToScalar(0), SkIntToScalar(0));
}

/* TODO: bug? looks like we need to add to the previous value (strange) */
void NativeSkia::moveTo(double x, double y)
{
    if (!currentPath) {
        beginPath();
    }

    currentPath->moveTo(SkDoubleToScalar(x), SkDoubleToScalar(y));
}

void NativeSkia::lineTo(double x, double y)
{
    /* moveTo is set? */
    if (!currentPath) {
        beginPath();
    }

    currentPath->lineTo(SkDoubleToScalar(x), SkDoubleToScalar(y));
}

void NativeSkia::fill()
{
    if (!currentPath) {
        return;
    }

    canvas->drawPath(*currentPath, *paint);
    CANVAS_FLUSH();
}

void NativeSkia::stroke()
{
    if (!currentPath) {
        return;
    }

    canvas->drawPath(*currentPath, *paint_stroke);
    CANVAS_FLUSH();   
}

void NativeSkia::closePath()
{
    if (!currentPath) {
        return;
    }

    currentPath->close();

}

void NativeSkia::clip()
{
    if (!currentPath) {
        return;
    }

    canvas->clipPath(*currentPath);
    CANVAS_FLUSH();
}

void NativeSkia::rect(double x, double y, double width, double height)
{
    if (!currentPath) {
        beginPath();
    }

    SkRect r;

    r.setXYWH(SkDoubleToScalar(x), SkDoubleToScalar(y),
        SkDoubleToScalar(width), SkDoubleToScalar(height));
    
    currentPath->addRect(r);
}


void NativeSkia::arc(int x, int y, int r,
    double startAngle, double endAngle, int CCW)
{
    if (!currentPath || (!startAngle && !endAngle) || !r) {
        return;
    }

    double sweep = endAngle - startAngle;

    SkRect rect;
    SkScalar cx = SkIntToScalar(x);
    SkScalar cy = SkIntToScalar(y);
    SkScalar s360 = SkIntToScalar(360);
    SkScalar radius = SkIntToScalar(r);

    SkScalar start = SkDoubleToScalar(180 * startAngle / SK_ScalarPI);
    SkScalar end = SkDoubleToScalar(180 * sweep / SK_ScalarPI);

    rect.set(cx-radius, cy-radius, cx+radius, cy+radius);

    if (end >= s360 || end <= -s360) {
        // Move to the start position (0 sweep means we add a single point).
        currentPath->arcTo(rect, start, 0, false);
        // Draw the circle.
        currentPath->addOval(rect);
        // Force a moveTo the end position.
        currentPath->arcTo(rect, start + end, 0, true);        
    } else {
        if (CCW && end > 0) {
            end -= s360;
        } else if (!CCW && end < 0) {
            end += s360;
        }

        currentPath->arcTo(rect, start, end, false);        
    }
}

void NativeSkia::quadraticCurveTo(int cpx, int cpy, int x, int y)
{
    if (!currentPath) {
        return;
    }

    currentPath->quadTo(SkIntToScalar(cpx), SkIntToScalar(cpy),
        SkIntToScalar(x), SkIntToScalar(y));
}

void NativeSkia::bezierCurveTo(double cpx, double cpy, double cpx2, double cpy2,
    double x, double y)
{
    if (!currentPath) {
        return;
    }

    currentPath->cubicTo(SkDoubleToScalar(cpx), SkDoubleToScalar(cpy),
        SkDoubleToScalar(cpx2), SkDoubleToScalar(cpy2),
        SkDoubleToScalar(x), SkDoubleToScalar(y));
}

void NativeSkia::rotate(double angle)
{
    canvas->rotate(SkDoubleToScalar(180 * angle / SK_ScalarPI));
}

void NativeSkia::scale(double x, double y)
{
    canvas->scale(SkDoubleToScalar(x), SkDoubleToScalar(y));
}

void NativeSkia::translate(double x, double y)
{
    canvas->translate(SkDoubleToScalar(x), SkDoubleToScalar(y));
}

void NativeSkia::save()
{
    //canvas->saveLayer(NULL, NULL, SkCanvas::kARGB_NoClipLayer_SaveFlag);
    canvas->save();
}

void NativeSkia::restore()
{
    canvas->restore();
}

double NativeSkia::measureText(const char *str, size_t length)
{
    return SkScalarToDouble(paint->measureText(str, length));
}

void NativeSkia::skew(double x, double y)
{
    canvas->skew(SkDoubleToScalar(x), SkDoubleToScalar(y));
}

/*
    composite :
    http://code.google.com/p/webkit-mirror/source/browse/Source/WebCore/platform/graphics/skia/SkiaUtils.cpp

    pointInPath :
    http://code.google.com/p/webkit-mirror/source/browse/Source/WebCore/platform/graphics/skia/SkiaUtils.cpp#115
*/

void NativeSkia::transform(double scalex, double skewy, double skewx,
            double scaley, double translatex, double translatey, int set)
{
    SkMatrix m;

    m.setScaleX(SkDoubleToScalar(scalex));
    m.setSkewX(SkDoubleToScalar(skewx));
    m.setTranslateX(SkDoubleToScalar(translatex));

    m.setScaleY(SkDoubleToScalar(scaley));
    m.setSkewY(SkDoubleToScalar(skewy));
    m.setTranslateY(SkDoubleToScalar(translatey));

    m.setPerspX(0);
    m.setPerspY(0);

    m.set(SkMatrix::kMPersp2, SK_Scalar1);

    if (set) {
        canvas->setMatrix(m);
    } else {
        canvas->concat(m);
    }
}

void NativeSkia::setLineCap(const char *capStyle)
{
    if (strcasecmp(capStyle, "round") == 0) {
        paint_stroke->setStrokeCap(SkPaint::kRound_Cap);
    } else if (strcasecmp(capStyle, "square") == 0) {
        paint_stroke->setStrokeCap(SkPaint::kSquare_Cap);
    } else {
        paint_stroke->setStrokeCap(SkPaint::kButt_Cap);
    }
}

void NativeSkia::setLineJoin(const char *joinStyle)
{
     if (strcasecmp(joinStyle, "round") == 0) {
        paint_stroke->setStrokeJoin(SkPaint::kRound_Join);
    } else if (strcasecmp(joinStyle, "bevel") == 0) {
        paint_stroke->setStrokeJoin(SkPaint::kBevel_Join);
    } else {
        paint_stroke->setStrokeJoin(SkPaint::kMiter_Join);
    }
    
}

void NativeSkia::drawImage(NativeSkImage *image, double x, double y)
{
    if (image->isCanvas) {
        canvas->readPixels(SkIRect::MakeSize(canvas->getDeviceSize()),
            &image->img);
    }

    canvas->drawBitmap(image->img, SkDoubleToScalar(x), SkDoubleToScalar(y),
        paint);

    /* TODO: clear read'd pixel? */
    CANVAS_FLUSH();
}

void NativeSkia::drawImage(NativeSkImage *image, double x, double y,
    double width, double height)
{
    SkRect r;
    r.setXYWH(SkDoubleToScalar(x), SkDoubleToScalar(y),
        SkDoubleToScalar(width), SkDoubleToScalar(height));

    if (image->isCanvas) {
        canvas->readPixels(SkIRect::MakeSize(canvas->getDeviceSize()),
            &image->img);
    }

    if (!image->img.hasMipMap()) {
        printf("build mipmap\n");
        image->img.buildMipMap();
    }
    canvas->drawBitmapRect(image->img, NULL, r, paint)

    CANVAS_FLUSH();
}

void NativeSkia::drawImage(NativeSkImage *image,
    int sx, int sy, int swidth, int sheight,
    double dx, double dy, double dwidth, double dheight)
{
    SkRect dst;
    SkIRect src;

    /* TODO: ->readPixels : switch to readPixels(bitmap, x, y); */
    src.setXYWH(sx, sy, swidth, sheight);

    if (image->isCanvas) {
        canvas->readPixels(src, &image->img);
    }

    dst.setXYWH(SkDoubleToScalar(dx), SkDoubleToScalar(dy),
        SkDoubleToScalar(dwidth), SkDoubleToScalar(dheight));

    if (!image->img.hasMipMap()) {
        image->img.buildMipMap();
    }

    canvas->drawBitmapRect(image->img, (image->isCanvas ? NULL : &src), dst, paint);

    CANVAS_FLUSH();
}

void NativeSkia::redrawScreen()
{
    canvas->readPixels(SkIRect::MakeSize(canvas->getDeviceSize()),
        screen);
    canvas->writePixels(*screen, 0, 0);
    CANVAS_FLUSH();  
}

#if 0
void NativeSkia::drawPixelsGL(uint8_t *pixels, int width, int height,
    int x, int y)
{
    canvas->flush();
    glDisable(GL_ALPHA_TEST);

    glWindowPos2i(x, y);

    if (glGetError() != GL_NO_ERROR) {
        printf("got an error\n");
    }
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);
    if (glGetError() != GL_NO_ERROR) {
        printf("got an error\n");
    }
    context->resetContext();
}
#endif

void NativeSkia::drawPixels(uint8_t *pixels, int width, int height,
    int x, int y)
{
    //drawPixelsGL(pixels, width, height, x, y);
    //return;

    SkBitmap bt;
    SkPaint pt;
    SkRect r;

    uint32_t *PMPixels = (uint32_t *)alloca(width * height * 4);
    bt.setConfig(SkBitmap::kARGB_8888_Config, width, height, width*4);

    SkConvertConfig8888Pixels(PMPixels, width*4,
        SkCanvas::kNative_Premul_Config8888,
        (uint32_t*)pixels, width*4, SkCanvas::kRGBA_Unpremul_Config8888,
        width, height);

    bt.setIsVolatile(true);
    bt.setPixels(PMPixels);
    r.setXYWH(x, y, width, height);

    canvas->saveLayer(NULL, NULL);
        canvas->clipRect(r, SkRegion::kReplace_Op);
        canvas->drawColor(SK_ColorWHITE);
        canvas->drawBitmap(bt, x, y);
    canvas->restore();
}

int NativeSkia::readPixels(int top, int left, int width, int height,
    uint8_t *pixels)
{
    SkBitmap bt;

    bt.setConfig(SkBitmap::kARGB_8888_Config, width, height, width*4);
    memset(pixels, 0, width * height * 4);

    bt.setPixels(pixels);

    if (!canvas->readPixels(&bt, left, top, SkCanvas::kRGBA_Premul_Config8888)) {
        printf("Failed to read pixels\n");
        return 0;
    }

    return 1;
}

/*
static SkBitmap load_bitmap() {
    SkStream* stream = new SkFILEStream("/skimages/sesame_street_ensemble-hp.jpg");
    SkAutoUnref aur(stream);
    
    SkBitmap bm;
    if (SkImageDecoder::DecodeStream(stream, &bm, SkBitmap::kNo_Config,
                                     SkImageDecoder::kDecodeBounds_Mode)) {
        SkPixelRef* pr = new SkImageRef_GlobalPool(stream, bm.config(), 1);
        bm.setPixelRef(pr)->unref();
    }
    return bm;
}
*/

void NativeSkia::flush()
{
    canvas->flush();
}
