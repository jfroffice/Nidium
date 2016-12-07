/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef graphics_shadowlooper_h__
#define graphics_shadowlooper_h__

#include <SkDrawLooper.h>
#include <SkColor.h>
#include <SkBlurMask.h>

class SkMaskFilter;
class SkColorFilter;

namespace Nidium {
namespace Graphics {

/** class ShadowLooper
    This class draws a shadow of the object (possibly offset), and then draws
    the original object in its original position.
    should there be an option to just draw the shadow/blur layer? webkit?
*/
class SK_API ShadowLooper : public SkDrawLooper
{
public:
    enum BlurFlags
    {
        kNone_BlurFlag = 0x00,
        /**
            The blur layer's dx/dy/radius aren't affected by the canvas
            transform.
        */
        kIgnoreTransform_BlurFlag = 0x01,
        kOverrideColor_BlurFlag   = 0x02,
        kHighQuality_BlurFlag     = 0x04,
        /** mask for all blur flags */
        kAll_BlurFlag = 0x07
    };

    static ShadowLooper *Create(SkColor color,
                                SkScalar sigma,
                                SkScalar dx,
                                SkScalar dy,
                                uint32_t flags = kNone_BlurFlag)
    {
        return SkNEW_ARGS(ShadowLooper, (color, sigma, dx, dy, flags));
    }

    static ShadowLooper *Create(SkScalar radius,
                                SkScalar dx,
                                SkScalar dy,
                                SkColor color,
                                uint32_t flags)
    {
        return SkNEW_ARGS(
            ShadowLooper,
            (color, SkBlurMask::ConvertRadiusToSigma(radius), dx, dy, flags));
    }

    virtual ~ShadowLooper();

    virtual ShadowLooper::Context *
    createContext(SkCanvas *, void *storage) const SK_OVERRIDE;
    virtual size_t contextSize() const SK_OVERRIDE
    {
        return sizeof(ShadowLooperContext);
    }

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(ShadowLooper)

protected:
    ShadowLooper(SkColor color,
                 SkScalar sigma,
                 SkScalar dx,
                 SkScalar dy,
                 uint32_t flags = kNone_BlurFlag);
    ShadowLooper(SkScalar radius,
                 SkScalar dx,
                 SkScalar dy,
                 SkColor color,
                 uint32_t flags = kNone_BlurFlag);
    ShadowLooper(SkReadBuffer &);

    virtual void flatten(SkWriteBuffer &) const SK_OVERRIDE;
    virtual bool asABlurShadow(BlurShadowRec *) const SK_OVERRIDE;

private:
    SkMaskFilter *m_fBlur;
    SkColorFilter *m_fColorFilter;
    SkScalar m_fDx, m_fDy, m_fSigma;
    SkColor m_fBlurColor;
    uint32_t m_fBlurFlags;

    enum State
    {
        kBeforeEdge,
        kAfterEdge,
        kDone
    };
    class ShadowLooperContext : public SkDrawLooper::Context
    {
    public:
        explicit ShadowLooperContext(const ShadowLooper *looper);

        virtual bool next(SkCanvas *canvas, SkPaint *paint) SK_OVERRIDE;

    private:
        const ShadowLooper *m_fLooper;
        State m_fState;
    };

    void init(SkScalar sigma,
              SkScalar dx,
              SkScalar dy,
              SkColor color,
              uint32_t flags);
    void initEffects();

    typedef SkDrawLooper INHERITED;
};

} // namespace Graphics
} // namespace Nidium

#endif
