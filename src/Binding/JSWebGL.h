/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef binding_jswebgl_h__
#define binding_jswebgl_h__

#include <Binding/JSExposer.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;

#define NIDIUM_GL_NEW_CLASS(name)\
class JS ## name: public JSExposer<JS ## name>\
{\
    public :\
        JS ## name ();\
        ~JS ## name ();\
         JS::PersistentRootedObject m_JsObj;\
        static void RegisterObject(JSContext *cx);\
};

#define GL_GLEXT_PROTOTYPES
#if __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

#include "Graphics/CanvasContext.h"
#include "Graphics/GLContext.h"

namespace Nidium {
namespace Binding {

// {{{ NGLContextAttributes
struct NGLContextAttributes {
    bool m_Alpha;
    bool m_Depth;
    bool m_Stencil;
    bool m_Antialias;
    bool m_PremultipliedAlpha;
    bool m_PreserveDrawingBuffer;
    bool m_PreferLowPowerToHightPerformance;

    void set(bool alpha, bool depth, bool stencil,
            bool antialias, bool premultipliedAlpha,
            bool preserveDrawingBuffer, bool preferLowPowerToHightPerformance)
    {
        m_Alpha = alpha;
        m_Depth = depth;
        m_Stencil = stencil;
        m_Antialias = antialias;
        m_PremultipliedAlpha = premultipliedAlpha;
        m_PreserveDrawingBuffer = preserveDrawingBuffer;
        m_PreferLowPowerToHightPerformance = preferLowPowerToHightPerformance;
    }

    NGLContextAttributes()
        :  m_Alpha(true), m_Depth(true), m_Stencil(false), m_Antialias(true),
           m_PremultipliedAlpha(true), m_PreserveDrawingBuffer(false),
           m_PreferLowPowerToHightPerformance(false)
    {
    }
};
// }}}

// {{{ CanvasWebGLContext
class CanvasWebGLContext: public Graphics::CanvasContext
{
    public :
        CanvasWebGLContext(JSContext *cx, NGLContextAttributes *attributes, int width, int height);
        ~CanvasWebGLContext();

        void translate(double x, double y) {};
        void setSize(int width, int height, bool redraw = true) {};
        void setScale(double x, double y, double px=1, double py=1) {};
        void clear(uint32_t color) {};
        void flush() {};

        void composeWith(Canvas2DContext *layer,
            double left, double top, double opacity,
            double zoom, const Graphics::Rect *rclip);

        bool m_UnpackFlipY;
        bool m_UnpackPremultiplyAlpha;
    private:
        GLuint m_Tex;
        GLuint m_Fbo;
        int m_Width;
        int m_Height;
};
// }}}

// {{{ WebGL Classes declaration
NIDIUM_GL_NEW_CLASS(WebGLRenderingContext)
NIDIUM_GL_NEW_CLASS(WebGLObject)
NIDIUM_GL_NEW_CLASS(WebGLBuffer)
NIDIUM_GL_NEW_CLASS(WebGLFramebuffer)
NIDIUM_GL_NEW_CLASS(WebGLProgram)
NIDIUM_GL_NEW_CLASS(WebGLRenderbuffer)
NIDIUM_GL_NEW_CLASS(WebGLShader)
NIDIUM_GL_NEW_CLASS(WebGLTexture)
NIDIUM_GL_NEW_CLASS(WebGLUniformLocation)
NIDIUM_GL_NEW_CLASS(WebGLShaderPrecisionFormat)
class JSWebGLActiveInfo : public JSExposer<JSWebGLActiveInfo>
{
    public :
        JSWebGLActiveInfo();
        ~JSWebGLActiveInfo();
		static JS::HandleObject Create(JSContext *cx, 
				GLint size, GLenum type, const char *name);
        static void RegisterObject(JSContext *cx);
};

#undef NIDIUM_GL_NEW_CLASS
// }}}

// {{{ OpenGL defines
/* ClearBufferMask */
#define NGL_DEPTH_BUFFER_BIT                0x00000100
#define NGL_STENCIL_BUFFER_BIT              0x00000400
#define NGL_COLOR_BUFFER_BIT                0x00004000

/* BeginMode */
#define NGL_POINTS                          0x0000
#define NGL_LINES                           0x0001
#define NGL_LINE_LOOP                       0x0002
#define NGL_LINE_STRIP                      0x0003
#define NGL_TRIANGLES                       0x0004
#define NGL_TRIANGLE_STRIP                  0x0005
#define NGL_TRIANGLE_FAN                    0x0006

/* AlphaFunction (not supported in ES20) */
/*      NEVER */
/*      LESS */
/*      EQUAL */
/*      LEQUAL */
/*      GREATER */
/*      NOTEQUAL */
/*      GEQUAL */
/*      ALWAYS */

/* BlendingFactorDest */
#define NGL_ZERO                            0
#define NGL_ONE                             1
#define NGL_SRC_COLOR                       0x0300
#define NGL_ONE_MINUS_SRC_COLOR             0x0301
#define NGL_SRC_ALPHA                       0x0302
#define NGL_ONE_MINUS_SRC_ALPHA             0x0303
#define NGL_DST_ALPHA                       0x0304
#define NGL_ONE_MINUS_DST_ALPHA             0x0305

/* BlendingFactorSrc */
/*      ZERO */
/*      ONE */
#define NGL_DST_COLOR                       0x0306
#define NGL_ONE_MINUS_DST_COLOR             0x0307
#define NGL_SRC_ALPHA_SATURATE              0x0308
/*      SRC_ALPHA */
/*      ONE_MINUS_SRC_ALPHA */
/*      DST_ALPHA */
/*      ONE_MINUS_DST_ALPHA */

/* BlendEquationSeparate */
#define NGL_FUNC_ADD                        0x8006
#define NGL_BLEND_EQUATION                  0x8009
#define NGL_BLEND_EQUATION_RGB              0x8009   /* same as BLEND_EQUATION */
#define NGL_BLEND_EQUATION_ALPHA            0x883D

/* BlendSubtract */
#define NGL_FUNC_SUBTRACT                   0x800A
#define NGL_FUNC_REVERSE_SUBTRACT           0x800B

/* Separate Blend Functions */
#define NGL_BLEND_DST_RGB                   0x80C8
#define NGL_BLEND_SRC_RGB                   0x80C9
#define NGL_BLEND_DST_ALPHA                 0x80CA
#define NGL_BLEND_SRC_ALPHA                 0x80CB
#define NGL_CONSTANT_COLOR                  0x8001
#define NGL_ONE_MINUS_CONSTANT_COLOR        0x8002
#define NGL_CONSTANT_ALPHA                  0x8003
#define NGL_ONE_MINUS_CONSTANT_ALPHA        0x8004
#define NGL_BLEND_COLOR                     0x8005

/* Buffer Objects */
#define NGL_ARRAY_BUFFER                    0x8892
#define NGL_ELEMENT_ARRAY_BUFFER            0x8893
#define NGL_ARRAY_BUFFER_BINDING            0x8894
#define NGL_ELEMENT_ARRAY_BUFFER_BINDING    0x8895

#define NGL_STREAM_DRAW                     0x88E0
#define NGL_STATIC_DRAW                     0x88E4
#define NGL_DYNAMIC_DRAW                    0x88E8

#define NGL_BUFFER_SIZE                     0x8764
#define NGL_BUFFER_USAGE                    0x8765

#define NGL_CURRENT_VERTEX_ATTRIB           0x8626

/* CullFaceMode */
#define NGL_FRONT                           0x0404
#define NGL_BACK                            0x0405
#define NGL_FRONT_AND_BACK                  0x0408

/* DepthFunction */
/*      NEVER */
/*      LESS */
/*      EQUAL */
/*      LEQUAL */
/*      GREATER */
/*      NOTEQUAL */
/*      GEQUAL */
/*      ALWAYS */

/* EnableCap */
/* TEXTURE_2D */
#define NGL_CULL_FACE                       0x0B44
#define NGL_BLEND                           0x0BE2
#define NGL_DITHER                          0x0BD0
#define NGL_STENCIL_TEST                    0x0B90
#define NGL_DEPTH_TEST                      0x0B71
#define NGL_SCISSOR_TEST                    0x0C11
#define NGL_POLYGON_OFFSET_FILL             0x8037
#define NGL_SAMPLE_ALPHA_TO_COVERAGE        0x809E
#define NGL_SAMPLE_COVERAGE                 0x80A0

/* ErrorCode */
#define NGL_NO_ERROR                        0
#define NGL_INVALID_ENUM                    0x0500
#define NGL_INVALID_VALUE                   0x0501
#define NGL_INVALID_OPERATION               0x0502
#define NGL_OUT_OF_MEMORY                   0x0505

/* FrontFaceDirection */
#define NGL_CW                              0x0900
#define NGL_CCW                             0x0901

/* GetPName */
#define NGL_LINE_WIDTH                      0x0B21
#define NGL_ALIASED_POINT_SIZE_RANGE        0x846D
#define NGL_ALIASED_LINE_WIDTH_RANGE        0x846E
#define NGL_CULL_FACE_MODE                  0x0B45
#define NGL_FRONT_FACE                      0x0B46
#define NGL_DEPTH_RANGE                     0x0B70
#define NGL_DEPTH_WRITEMASK                 0x0B72
#define NGL_DEPTH_CLEAR_VALUE               0x0B73
#define NGL_DEPTH_FUNC                      0x0B74
#define NGL_STENCIL_CLEAR_VALUE             0x0B91
#define NGL_STENCIL_FUNC                    0x0B92
#define NGL_STENCIL_FAIL                    0x0B94
#define NGL_STENCIL_PASS_DEPTH_FAIL         0x0B95
#define NGL_STENCIL_PASS_DEPTH_PASS         0x0B96
#define NGL_STENCIL_REF                     0x0B97
#define NGL_STENCIL_VALUE_MASK              0x0B93
#define NGL_STENCIL_WRITEMASK               0x0B98
#define NGL_STENCIL_BACK_FUNC               0x8800
#define NGL_STENCIL_BACK_FAIL               0x8801
#define NGL_STENCIL_BACK_PASS_DEPTH_FAIL    0x8802
#define NGL_STENCIL_BACK_PASS_DEPTH_PASS    0x8803
#define NGL_STENCIL_BACK_REF                0x8CA3
#define NGL_STENCIL_BACK_VALUE_MASK         0x8CA4
#define NGL_STENCIL_BACK_WRITEMASK          0x8CA5
#define NGL_VIEWPORT                        0x0BA2
#define NGL_SCISSOR_BOX                     0x0C10
/*      SCISSOR_TEST */
#define NGL_COLOR_CLEAR_VALUE               0x0C22
#define NGL_COLOR_WRITEMASK                 0x0C23
#define NGL_UNPACK_ALIGNMENT                0x0CF5
#define NGL_PACK_ALIGNMENT                  0x0D05
#define NGL_MAX_TEXTURE_SIZE                0x0D33
#define NGL_MAX_VIEWPORT_DIMS               0x0D3A
#define NGL_SUBPIXEL_BITS                   0x0D50
#define NGL_RED_BITS                        0x0D52
#define NGL_GREEN_BITS                      0x0D53
#define NGL_BLUE_BITS                       0x0D54
#define NGL_ALPHA_BITS                      0x0D55
#define NGL_DEPTH_BITS                      0x0D56
#define NGL_STENCIL_BITS                    0x0D57
#define NGL_POLYGON_OFFSET_UNITS            0x2A00
/*      POLYGON_OFFSET_FILL */
#define NGL_POLYGON_OFFSET_FACTOR           0x8038
#define NGL_TEXTURE_BINDING_2D              0x8069
#define NGL_SAMPLE_BUFFERS                  0x80A8
#define NGL_SAMPLES                         0x80A9
#define NGL_SAMPLE_COVERAGE_VALUE           0x80AA
#define NGL_SAMPLE_COVERAGE_INVERT          0x80AB

/* GetTextureParameter */
/*      TEXTURE_MAG_FILTER */
/*      TEXTURE_MIN_FILTER */
/*      TEXTURE_WRAP_S */
/*      TEXTURE_WRAP_T */

#define NGL_COMPRESSED_TEXTURE_FORMATS      0x86A3

/* HintMode */
#define NGL_DONT_CARE                       0x1100
#define NGL_FASTEST                         0x1101
#define NGL_NICEST                          0x1102

/* HintTarget */
#define NGL_GENERATE_MIPMAP_HINT             0x8192

/* DataType */
#define NGL_BYTE                            0x1400
#define NGL_UNSIGNED_BYTE                   0x1401
#define NGL_SHORT                           0x1402
#define NGL_UNSIGNED_SHORT                  0x1403
#define NGL_INT                             0x1404
#define NGL_UNSIGNED_INT                    0x1405
#define NGL_FLOAT                           0x1406

/* PixelFormat */
#define NGL_DEPTH_COMPONENT                 0x1902
#define NGL_ALPHA                           0x1906
#define NGL_RGB                             0x1907
#define NGL_RGBA                            0x1908
#define NGL_LUMINANCE                       0x1909
#define NGL_LUMINANCE_ALPHA                 0x190A

/* PixelType */
/*      UNSIGNED_BYTE */
#define NGL_UNSIGNED_SHORT_4_4_4_4          0x8033
#define NGL_UNSIGNED_SHORT_5_5_5_1          0x8034
#define NGL_UNSIGNED_SHORT_5_6_5            0x8363

/* Shaders */
#define NGL_FRAGMENT_SHADER                   0x8B30
#define NGL_VERTEX_SHADER                     0x8B31
#define NGL_MAX_VERTEX_ATTRIBS                0x8869
#define NGL_MAX_VERTEX_UNIFORM_VECTORS        0x8DFB
#define NGL_MAX_VARYING_VECTORS               0x8DFC
#define NGL_MAX_COMBINED_TEXTURE_IMAGE_UNITS  0x8B4D
#define NGL_MAX_VERTEX_TEXTURE_IMAGE_UNITS    0x8B4C
#define NGL_MAX_TEXTURE_IMAGE_UNITS           0x8872
#define NGL_MAX_FRAGMENT_UNIFORM_VECTORS      0x8DFD
#define NGL_SHADER_TYPE                       0x8B4F
#define NGL_DELETE_STATUS                     0x8B80
#define NGL_LINK_STATUS                       0x8B82
#define NGL_VALIDATE_STATUS                   0x8B83
#define NGL_ATTACHED_SHADERS                  0x8B85
#define NGL_ACTIVE_UNIFORMS                   0x8B86
#define NGL_ACTIVE_ATTRIBUTES                 0x8B89
#define NGL_SHADING_LANGUAGE_VERSION          0x8B8C
#define NGL_CURRENT_PROGRAM                   0x8B8D

/* StencilFunction */
#define NGL_NEVER                           0x0200
#define NGL_LESS                            0x0201
#define NGL_EQUAL                           0x0202
#define NGL_LEQUAL                          0x0203
#define NGL_GREATER                         0x0204
#define NGL_NOTEQUAL                        0x0205
#define NGL_GEQUAL                          0x0206
#define NGL_ALWAYS                          0x0207

/* StencilOp */
/*      ZERO */
#define NGL_KEEP                            0x1E00
#define NGL_REPLACE                         0x1E01
#define NGL_INCR                            0x1E02
#define NGL_DECR                            0x1E03
#define NGL_INVERT                          0x150A
#define NGL_INCR_WRAP                       0x8507
#define NGL_DECR_WRAP                       0x8508

/* StringName */
#define NGL_VENDOR                          0x1F00
#define NGL_RENDERER                        0x1F01
#define NGL_VERSION                         0x1F02

/* TextureMagFilter */
#define NGL_NEAREST                         0x2600
#define NGL_LINEAR                          0x2601

/* TextureMinFilter */
/*      NEAREST */
/*      LINEAR */
#define NGL_NEAREST_MIPMAP_NEAREST          0x2700
#define NGL_LINEAR_MIPMAP_NEAREST           0x2701
#define NGL_NEAREST_MIPMAP_LINEAR           0x2702
#define NGL_LINEAR_MIPMAP_LINEAR            0x2703

/* TextureParameterName */
#define NGL_TEXTURE_MAG_FILTER              0x2800
#define NGL_TEXTURE_MIN_FILTER              0x2801
#define NGL_TEXTURE_WRAP_S                  0x2802
#define NGL_TEXTURE_WRAP_T                  0x2803

/* TextureTarget */
#define NGL_TEXTURE_2D                      0x0DE1
#define NGL_TEXTURE                         0x1702

#define NGL_TEXTURE_CUBE_MAP                0x8513
#define NGL_TEXTURE_BINDING_CUBE_MAP        0x8514
#define NGL_TEXTURE_CUBE_MAP_POSITIVE_X     0x8515
#define NGL_TEXTURE_CUBE_MAP_NEGATIVE_X     0x8516
#define NGL_TEXTURE_CUBE_MAP_POSITIVE_Y     0x8517
#define NGL_TEXTURE_CUBE_MAP_NEGATIVE_Y     0x8518
#define NGL_TEXTURE_CUBE_MAP_POSITIVE_Z     0x8519
#define NGL_TEXTURE_CUBE_MAP_NEGATIVE_Z     0x851A
#define NGL_MAX_CUBE_MAP_TEXTURE_SIZE       0x851C

/* TextureUnit */
#define NGL_TEXTURE0                        0x84C0
#define NGL_TEXTURE1                        0x84C1
#define NGL_TEXTURE2                        0x84C2
#define NGL_TEXTURE3                        0x84C3
#define NGL_TEXTURE4                        0x84C4
#define NGL_TEXTURE5                        0x84C5
#define NGL_TEXTURE6                        0x84C6
#define NGL_TEXTURE7                        0x84C7
#define NGL_TEXTURE8                        0x84C8
#define NGL_TEXTURE9                        0x84C9
#define NGL_TEXTURE10                       0x84CA
#define NGL_TEXTURE11                       0x84CB
#define NGL_TEXTURE12                       0x84CC
#define NGL_TEXTURE13                       0x84CD
#define NGL_TEXTURE14                       0x84CE
#define NGL_TEXTURE15                       0x84CF
#define NGL_TEXTURE16                       0x84D0
#define NGL_TEXTURE17                       0x84D1
#define NGL_TEXTURE18                       0x84D2
#define NGL_TEXTURE19                       0x84D3
#define NGL_TEXTURE20                       0x84D4
#define NGL_TEXTURE21                       0x84D5
#define NGL_TEXTURE22                       0x84D6
#define NGL_TEXTURE23                       0x84D7
#define NGL_TEXTURE24                       0x84D8
#define NGL_TEXTURE25                       0x84D9
#define NGL_TEXTURE26                       0x84DA
#define NGL_TEXTURE27                       0x84DB
#define NGL_TEXTURE28                       0x84DC
#define NGL_TEXTURE29                       0x84DD
#define NGL_TEXTURE30                       0x84DE
#define NGL_TEXTURE31                       0x84DF
#define NGL_ACTIVE_TEXTURE                  0x84E0

/* TextureWrapMode */
#define NGL_REPEAT                          0x2901
#define NGL_CLAMP_TO_EDGE                   0x812F
#define NGL_MIRRORED_REPEAT                 0x8370

/* Uniform Types */
#define NGL_FLOAT_VEC2                      0x8B50
#define NGL_FLOAT_VEC3                      0x8B51
#define NGL_FLOAT_VEC4                      0x8B52
#define NGL_INT_VEC2                        0x8B53
#define NGL_INT_VEC3                        0x8B54
#define NGL_INT_VEC4                        0x8B55
#define NGL_BOOL                            0x8B56
#define NGL_BOOL_VEC2                       0x8B57
#define NGL_BOOL_VEC3                       0x8B58
#define NGL_BOOL_VEC4                       0x8B59
#define NGL_FLOAT_MAT2                      0x8B5A
#define NGL_FLOAT_MAT3                      0x8B5B
#define NGL_FLOAT_MAT4                      0x8B5C
#define NGL_SAMPLER_2D                      0x8B5E
#define NGL_SAMPLER_CUBE                    0x8B60

/* Vertex Arrays */
#define NGL_VERTEX_ATTRIB_ARRAY_ENABLED         0x8622
#define NGL_VERTEX_ATTRIB_ARRAY_SIZE            0x8623
#define NGL_VERTEX_ATTRIB_ARRAY_STRIDE          0x8624
#define NGL_VERTEX_ATTRIB_ARRAY_TYPE            0x8625
#define NGL_VERTEX_ATTRIB_ARRAY_NORMALIZED      0x886A
#define NGL_VERTEX_ATTRIB_ARRAY_POINTER         0x8645
#define NGL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING  0x889F

/* Shader Source */
#define NGL_COMPILE_STATUS                  0x8B81

/* Shader Precision-Specified Types */
#define NGL_LOW_FLOAT                       0x8DF0
#define NGL_MEDIUM_FLOAT                    0x8DF1
#define NGL_HIGH_FLOAT                      0x8DF2
#define NGL_LOW_INT                         0x8DF3
#define NGL_MEDIUM_INT                      0x8DF4
#define NGL_HIGH_INT                        0x8DF5

/* Framebuffer Object. */
#define NGL_FRAMEBUFFER                     0x8D40
#define NGL_RENDERBUFFER                    0x8D41

#define NGL_RGBA4                           0x8056
#define NGL_RGB5_A1                         0x8057
#define NGL_RGB565                          0x8D62
#define NGL_DEPTH_COMPONENT16               0x81A5
#define NGL_STENCIL_INDEX                   0x1901
#define NGL_STENCIL_INDEX8                  0x8D48
#define NGL_DEPTH_STENCIL                   0x84F9

#define NGL_RENDERBUFFER_WIDTH              0x8D42
#define NGL_RENDERBUFFER_HEIGHT             0x8D43
#define NGL_RENDERBUFFER_INTERNAL_FORMAT    0x8D44
#define NGL_RENDERBUFFER_RED_SIZE           0x8D50
#define NGL_RENDERBUFFER_GREEN_SIZE         0x8D51
#define NGL_RENDERBUFFER_BLUE_SIZE          0x8D52
#define NGL_RENDERBUFFER_ALPHA_SIZE         0x8D53
#define NGL_RENDERBUFFER_DEPTH_SIZE         0x8D54
#define NGL_RENDERBUFFER_STENCIL_SIZE       0x8D55

#define NGL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE            0x8CD0
#define NGL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME            0x8CD1
#define NGL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL          0x8CD2
#define NGL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE  0x8CD3

#define NGL_COLOR_ATTACHMENT0               0x8CE0
#define NGL_DEPTH_ATTACHMENT                0x8D00
#define NGL_STENCIL_ATTACHMENT              0x8D20
#define NGL_DEPTH_STENCIL_ATTACHMENT        0x821A

#define NGL_NONE                            0

#define NGL_FRAMEBUFFER_COMPLETE                       0x8CD5
#define NGL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT          0x8CD6
#define NGL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT  0x8CD7
#define NGL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS          0x8CD9
#define NGL_FRAMEBUFFER_UNSUPPORTED                    0x8CDD

#define NGL_FRAMEBUFFER_BINDING             0x8CA6
#define NGL_RENDERBUFFER_BINDING            0x8CA7
#define NGL_MAX_RENDERBUFFER_SIZE           0x84E8

#define NGL_INVALID_FRAMEBUFFER_OPERATION   0x0506

/* WebGL-specific enums */
#define NGL_UNPACK_FLIP_Y_WEBGL             0x9240
#define NGL_UNPACK_PREMULTIPLY_ALPHA_WEBGL  0x9241
#define NGL_CONTEXT_LOST_WEBGL              0x9242
#define NGL_UNPACK_COLORSPACE_CONVERSION_WEBGL  0x9243
#define NGL_BROWSER_DEFAULT_WEBGL           0x9244
// }}}

} // namespace Binding
} // namespace Nidium

#endif

