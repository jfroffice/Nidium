/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "Binding/JSAV.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Binding/JSConsole.h>

#include "Binding/JSCanvas.h"

#include "Binding/JSCanvas2DContext.h"

using Nidium::Core::SharedMessages;
using Nidium::Graphics::SkiaContext;
using Nidium::Graphics::CanvasContext;
using Nidium::Graphics::CanvasHandler;
using namespace Nidium::AV;

extern "C" {
#include <libavformat/avformat.h>
}

namespace Nidium {
namespace Binding {

// {{{ Preamble
extern JSClass Canvas_class;

JSAudio *JSAudio::m_Instance = NULL;

// TODO : Need to handle nodes GC, similar to
//        https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html#lifetime-AudioNode
// TODO : Need to handle video GC
// TODO : When stop/pause/kill fade out sound


#define NJS (NidiumJS::GetObject(m_Cx))
#define JS_PROPAGATE_ERROR(cx, ...)\
JS_ReportError(cx, __VA_ARGS__); \
if (!JS_ReportPendingException(cx)) {\
    JS_ClearPendingException(cx); \
}

#define GET_NODE(type, var)\
    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class); \
    var = static_cast<type *>(CppObj->m_Node);

#define CHECK_INVALID_CTX(obj) if (!obj) {\
JS_ReportError(cx, "Invalid Audio context"); \
return false; \
}

typedef enum {
    NODE_CUSTOM_PROCESS_CALLBACK,
    NODE_CUSTOM_INIT_CALLBACK,
    NODE_CUSTOM_SETTER_CALLBACK,
    NODE_CUSTOM_SEEK_CALLBACK
} CustomNodeCallbacks;

extern void reportError(JSContext *cx, const char *message, JSErrorReport *report);

static void AudioNode_Finalize(JSFreeOp *fop, JSObject *obj);
static void AudioContext_Finalize(JSFreeOp *fop, JSObject *obj);

static bool nidium_Audio_constructor(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audio_getcontext(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audio_run(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audio_pFFT(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audio_load(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audio_createnode(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audio_connect(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audio_disconnect(JSContext *cx, unsigned argc, JS::Value *vp);

static bool nidium_audiothread_print(JSContext *cx, unsigned argc, JS::Value *vp);

static bool nidium_AudioNode_constructor(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_set(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_get(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_set(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_assign_processor(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_assign_init(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_assign_setter(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_assign_seek(JSContext *cx, unsigned argc, JS::Value *vp);
//static bool nidium_audionode_custom_get(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_threaded_set(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_threaded_get(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_threaded_send(JSContext *cx, unsigned argc, JS::Value *vp);

static bool nidium_audionode_source_open(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_source_play(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_source_pause(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_source_stop(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_source_close(JSContext *cx, unsigned argc, JS::Value *vp);

static bool nidium_audionode_custom_source_play(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_source_pause(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_source_stop(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_source_position_setter(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_audionode_custom_source_position_getter(JSContext *cx, unsigned argc, JS::Value *vp);

static bool nidium_audio_prop_setter(JSContext *cx, JS::HandleObject obj,
    uint8_t id, bool strict, JS::MutableHandleValue vp);
static bool nidium_audio_prop_getter(JSContext *cx, JS::HandleObject obj,
    uint8_t id, JS::MutableHandleValue vp);
static bool nidium_audionode_source_prop_setter(JSContext *cx, JS::HandleObject obj,
    uint8_t id, bool strict, JS::MutableHandleValue vp);
static bool nidium_audionode_source_prop_getter(JSContext *cx, JS::HandleObject obj,
    uint8_t id, JS::MutableHandleValue vp);

static JSClass Audio_class = {
    "Audio", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSClass AudioContext_class = {
    "AudioContext", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, AudioContext_Finalize,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

template<>
JSClass *JSExposer<JSAudio>::jsclass = &AudioContext_class;

static JSClass Global_AudioThread_class = {
    "_GLOBALAudioThread", JSCLASS_GLOBAL_FLAGS | JSCLASS_IS_GLOBAL,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr, nullptr, nullptr, JS_GlobalObjectTraceHook, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSClass AudioNodeLink_class = {
    "AudioNodeLink", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSClass AudioNodeEvent_class = {
    "AudioNodeEvent", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSClass AudioNode_class = {
    "AudioNode", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, AudioNode_Finalize,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

template<>
JSClass *JSExposer<JSAudioNode>::jsclass = &AudioNode_class;

static JSClass AudioNode_threaded_class = {
    "AudioNodeThreaded", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSPropertySpec AudioContext_props[] = {

    NIDIUM_JS_PSGS("volume", AUDIO_PROP_VOLUME, nidium_audio_prop_getter, nidium_audio_prop_setter),
    NIDIUM_JS_PSG("bufferSize", AUDIO_PROP_BUFFERSIZE, nidium_audio_prop_getter),
    NIDIUM_JS_PSG("channels", AUDIO_PROP_CHANNELS, nidium_audio_prop_getter),
    NIDIUM_JS_PSG("sampleRate", AUDIO_PROP_SAMPLERATE, nidium_audio_prop_getter),
    JS_PS_END
};

static JSPropertySpec AudioNode_props[] = {
    /* type, input, ouput readonly props are created in createnode function */
    JS_PS_END
};

static JSFunctionSpec AudioContext_funcs[] = {
    JS_FN("run", nidium_audio_run, 1, NIDIUM_JS_FNPROPS),
    JS_FN("load", nidium_audio_load, 1, NIDIUM_JS_FNPROPS),
    JS_FN("createNode", nidium_audio_createnode, 3, NIDIUM_JS_FNPROPS),
    JS_FN("connect", nidium_audio_connect, 2, NIDIUM_JS_FNPROPS),
    JS_FN("disconnect", nidium_audio_disconnect, 2, NIDIUM_JS_FNPROPS),
    JS_FN("pFFT", nidium_audio_pFFT, 2, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSFunctionSpec Audio_static_funcs[] = {
    JS_FN("getContext", nidium_audio_getcontext, 3, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSFunctionSpec AudioNode_funcs[] = {
    JS_FN("set", nidium_audionode_set, 2, NIDIUM_JS_FNPROPS),
    JS_FN("get", nidium_audionode_get, 1, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSFunctionSpec AudioNodeCustom_funcs[] = {
    JS_FN("set", nidium_audionode_custom_set, 2, NIDIUM_JS_FNPROPS),
    JS_FN("assignProcessor", nidium_audionode_custom_assign_processor, 1, NIDIUM_JS_FNPROPS),
    JS_FN("assignInit", nidium_audionode_custom_assign_init, 1, NIDIUM_JS_FNPROPS),
    JS_FN("assignSetter", nidium_audionode_custom_assign_setter, 1, NIDIUM_JS_FNPROPS),
    //JS_FN("get", nidium_audionode_custom_get, 1, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSFunctionSpec AudioNodeCustom_threaded_funcs[] = {
    JS_FN("set", nidium_audionode_custom_threaded_set, 2, NIDIUM_JS_FNPROPS),
    JS_FN("get", nidium_audionode_custom_threaded_get, 1, NIDIUM_JS_FNPROPS),
    JS_FN("send", nidium_audionode_custom_threaded_send, 2, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSFunctionSpec AudioNodeSource_funcs[] = {
    JS_FN("open", nidium_audionode_source_open, 1, NIDIUM_JS_FNPROPS),
    JS_FN("play", nidium_audionode_source_play, 0, NIDIUM_JS_FNPROPS),
    JS_FN("pause", nidium_audionode_source_pause, 0, NIDIUM_JS_FNPROPS),
    JS_FN("stop", nidium_audionode_source_stop, 0, NIDIUM_JS_FNPROPS),
    JS_FN("close", nidium_audionode_source_close, 0, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSFunctionSpec AudioNodeCustomSource_funcs[] = {
    JS_FN("play", nidium_audionode_custom_source_play, 0, NIDIUM_JS_FNPROPS),
    JS_FN("pause", nidium_audionode_custom_source_pause, 0, NIDIUM_JS_FNPROPS),
    JS_FN("stop", nidium_audionode_custom_source_stop, 0, NIDIUM_JS_FNPROPS),
    JS_FN("assignSeek", nidium_audionode_custom_assign_seek, 1, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSPropertySpec AudioNodeSource_props[] = {

    NIDIUM_JS_PSGS("position", SOURCE_PROP_POSITION, nidium_audionode_source_prop_getter, nidium_audionode_source_prop_setter),

    NIDIUM_JS_PSG("duration", SOURCE_PROP_DURATION, nidium_audionode_source_prop_getter),
    NIDIUM_JS_PSG("metadata", SOURCE_PROP_METADATA, nidium_audionode_source_prop_getter),
    NIDIUM_JS_PSG("bitrate", SOURCE_PROP_BITRATE, nidium_audionode_source_prop_getter),
    JS_PS_END
};

static JSFunctionSpec glob_funcs_threaded[] = {
    JS_FN("echo", nidium_audiothread_print, 1, NIDIUM_JS_FNPROPS),
    JS_FS_END
};


static bool nidium_Video_constructor(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_play(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_pause(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_stop(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_close(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_open(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_get_audionode(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_nextframe(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_prevframe(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_frameat(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_video_setsize(JSContext *cx, unsigned argc, JS::Value *vp);

static bool nidium_video_prop_getter(JSContext *cx, JS::HandleObject obj, uint8_t id, JS::MutableHandleValue vp);
static bool nidium_video_prop_setter(JSContext *cx, JS::HandleObject obj, uint8_t id, bool strict, JS::MutableHandleValue vp);

static void Video_Finalize(JSFreeOp *fop, JSObject *obj);

static JSFunctionSpec Video_funcs[] = {
    JS_FN("play", nidium_video_play, 0, NIDIUM_JS_FNPROPS),
    JS_FN("pause", nidium_video_pause, 0, NIDIUM_JS_FNPROPS),
    JS_FN("stop", nidium_video_stop, 0, NIDIUM_JS_FNPROPS),
    JS_FN("close", nidium_video_close, 0, NIDIUM_JS_FNPROPS),
    JS_FN("open", nidium_video_open, 1, NIDIUM_JS_FNPROPS),
    JS_FN("getAudioNode", nidium_video_get_audionode, 0, NIDIUM_JS_FNPROPS),
    JS_FN("nextFrame", nidium_video_nextframe, 0, NIDIUM_JS_FNPROPS),
    JS_FN("prevFrame", nidium_video_prevframe, 0, NIDIUM_JS_FNPROPS),
    JS_FN("frameAt", nidium_video_frameat, 1, NIDIUM_JS_FNPROPS),
    JS_FN("setSize", nidium_video_setsize, 2, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static JSClass Video_class = {
    "Video", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Video_Finalize,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

template<>
JSClass *JSExposer<JSVideo>::jsclass = &Video_class;

static JSPropertySpec Video_props[] = {

    NIDIUM_JS_PSG("width", VIDEO_PROP_WIDTH, nidium_video_prop_getter),
    NIDIUM_JS_PSG("height", VIDEO_PROP_HEIGHT, nidium_video_prop_getter),

    NIDIUM_JS_PSG("duration", SOURCE_PROP_DURATION, nidium_video_prop_getter),
    NIDIUM_JS_PSG("metadata", SOURCE_PROP_METADATA, nidium_video_prop_getter),
    NIDIUM_JS_PSG("bitrate", SOURCE_PROP_BITRATE, nidium_video_prop_getter),

    NIDIUM_JS_PSGS("position", SOURCE_PROP_POSITION, nidium_video_prop_getter, nidium_video_prop_setter),


    JS_PS_END
};
// }}}

// {{{ Implementation
static int FFT(int dir, int nn, double *x, double *y)
{
   long m, i, i1, j, i2, l, l2;
   double c1, c2, tx, ty, t1, t2, z;

   m = log2(nn);

   /* Do the bit reversal */
   i2 = nn >> 1;
   j = 0;
   for (i = 0; i < nn - 1; i++) {
      long k;

      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0;
   c2 = 0.0;
   l2 = 1;
   for (l = 0; l < m; l++) {
      long l1;
      double u1, u2;

      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j = 0; j < l1; j++) {
         for (i = j; i < nn; i += l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1;
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1)
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i = 0; i < nn; i++) {
         x[i] /= (double)nn;
         y[i] /= (double)nn;
      }
   }

   return true;
}

const char *JSAVEventRead(int ev)
{
    switch (ev) {
        case CUSTOM_SOURCE_SEND:
            return "message";
        case SOURCE_EVENT_PAUSE:
            return "pause";
        break;
        case SOURCE_EVENT_PLAY:
            return "play";
        break;
        case SOURCE_EVENT_STOP:
            return "stop";
        break;
        case SOURCE_EVENT_EOF:
            return "end";
        break;
        case SOURCE_EVENT_ERROR:
            return "error";
        break;
        case SOURCE_EVENT_BUFFERING:
            return "buffering";
        break;
        case SOURCE_EVENT_READY:
            return "ready";
        break;
        default:
            return NULL;
        break;
    }
}

static JS::HandleValue consumeSourceMessage(JSContext *cx, JS::HandleObject obj, const SharedMessages::Message &msg)
{
    JS::RootedObject evObj(cx, JSEvents::CreateEventObject(cx));
    JSObjectBuilder ev(cx, evObj);

    if (msg.event() == CUSTOM_SOURCE_SEND) {
        nidium_thread_msg *ptr = static_cast<struct nidium_thread_msg *>(msg.dataPtr());
        JS::RootedValue inval(cx, JSVAL_NULL);
        if (!JS_ReadStructuredClone(cx, ptr->data, ptr->nbytes,
            JS_STRUCTURED_CLONE_VERSION, &inval, nullptr, NULL)) {
            JS_PROPAGATE_ERROR(cx, "Failed to transfer custom node message to audio thread");
            return JS::UndefinedHandleValue;
        }

        ev.set("data", inval);

        delete ptr;
    } else {
        AVSourceEvent *cmsg =
                static_cast<struct AVSourceEvent*>(msg.dataPtr());

        if (cmsg->m_Ev == SOURCE_EVENT_ERROR) {
            int errorCode = cmsg->m_Args[0].toInt();
            const char *errorStr = AV::AVErrorsStr[errorCode];
            JS::RootedString jstr(cx, JS_NewStringCopyN(cx, errorStr, strlen(errorStr)));

            JS::RootedValue code(cx);
            code.setInt32(errorCode);

            JS::RootedValue err(cx);
            err.setString(jstr);

            ev.set("code", code);
            ev.set("error", err);
        } else if (cmsg->m_Ev == SOURCE_EVENT_BUFFERING) {
            ev.set("filesize", cmsg->m_Args[0].toInt());
            ev.set("startByte", cmsg->m_Args[1].toInt());
            ev.set("bufferedBytes", cmsg->m_Args[2].toInt());
        }

        delete cmsg;
    }

    JS::RootedValue rval(cx);
    rval.set(ev.jsval());
    return rval;
}

bool JSTransferableFunction::prepare(JSContext *cx, JS::HandleValue val)
{
    if (!JS_WriteStructuredClone(cx, val, &m_Data, &m_Bytes, nullptr, nullptr, JS::NullHandleValue)) {
        return false;
    }

    return true;
}

bool JSTransferableFunction::call(JS::HandleObject obj, JS::HandleValueArray params, JS::MutableHandleValue rval)
{
    if (m_Data != NULL) {
        if (!this->transfert()) {
            return false;
        }
    }

    JS::RootedValue fun(m_DestCx, m_Fn.get());

    return JS_CallFunctionValue(m_DestCx, obj, fun, params, rval);
}

bool JSTransferableFunction::transfert()
{
    JS::RootedValue fun(m_DestCx);

    bool ok = JS_ReadStructuredClone(m_DestCx, m_Data, m_Bytes, JS_STRUCTURED_CLONE_VERSION, &fun, nullptr, NULL);

    JS_ClearStructuredClone(m_Data, m_Bytes, nullptr, NULL);

    m_Data = NULL;
    m_Bytes = 0;
    m_Fn.set(fun);

    return ok;
}

JSTransferableFunction::~JSTransferableFunction()
{
    JSAutoRequest ar(m_DestCx);

    if (m_Data != NULL) {
        JS_ClearStructuredClone(m_Data, m_Bytes, nullptr, NULL);
    }

    JS::RootedValue fun(m_DestCx, m_Fn);
    if (!fun.isUndefined()) {
        fun.setUndefined();
    }
}

JSAudio *JSAudio::GetContext(JSContext *cx, JS::HandleObject obj, \
    unsigned int bufferSize, unsigned int channels, unsigned int sampleRate)
{
    ape_global *net = static_cast<ape_global *>(JS_GetContextPrivate(cx));
    Audio *audio;

    try {
        audio = new Audio(net, bufferSize, channels, sampleRate);
    } catch (...) {
        return NULL;
    }

    audio->setMainCtx(cx);

    return new JSAudio(audio, cx, obj);
}

void JSAudio::initNode(JSAudioNode *node, JS::HandleObject jnode, JS::HandleString name)
{
    int in = node->m_Node->m_InCount;
    int out = node->m_Node->m_OutCount;

    JS::RootedValue nameVal(m_Cx, STRING_TO_JSVAL(name));
    JS_DefineProperty(m_Cx, jnode, "type", nameVal, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
    JS::RootedObject arrayIn(m_Cx, JS_NewArrayObject(m_Cx, in));
    JS::RootedObject arrayOut(m_Cx, JS_NewArrayObject(m_Cx, out));
    JS::RootedValue inputLinks(m_Cx, OBJECT_TO_JSVAL(arrayIn));
    JS::RootedValue outputLinks(m_Cx, OBJECT_TO_JSVAL(arrayOut));

    for (int i = 0; i < in; i++) {
        JS::RootedObject link(m_Cx, JS_NewObject(m_Cx, &AudioNodeLink_class, JS::NullPtr(), JS::NullPtr()));
        JS_SetPrivate(link, node->m_Node->m_Input[i]);
        JS_DefineElement(m_Cx, inputLinks.toObjectOrNull(), i, OBJECT_TO_JSVAL(link), nullptr, nullptr, 0);
    }

    for (int i = 0; i < out; i++) {
        JS::RootedObject link(m_Cx, JS_NewObject(m_Cx, &AudioNodeLink_class, JS::NullPtr(), JS::NullPtr()));
        JS_SetPrivate(link, node->m_Node->m_Output[i]);

        JS_DefineElement(m_Cx, outputLinks.toObjectOrNull(), i, OBJECT_TO_JSVAL(link), nullptr, nullptr, 0);
    }

    if (in > 0) {
        JS_DefineProperty(m_Cx, jnode, "input", inputLinks, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
    }

    if (out > 0) {
        JS_DefineProperty(m_Cx, jnode, "output", outputLinks, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
    }

    node->m_nJs = NJS;

    node->setJSObject(jnode);
    node->setJSContext(m_Cx);

    NJS->rootObjectUntilShutdown(node->getJSObject());
    JS_SetPrivate(jnode, node);
}

JSAudio *JSAudio::GetContext()
{
    return JSAudio::m_Instance;
}

JSAudio::JSAudio(Audio *audio, JSContext *cx, JS::HandleObject obj)
    :
      JSExposer<JSAudio>(obj, cx),
      m_Audio(audio), m_Nodes(NULL), m_JsGlobalObj(NULL), m_JsRt(NULL), m_JsTcx(NULL),
      m_Target(NULL)
{
    JSAudio::m_Instance = this;

    JS_SetPrivate(obj, this);

    NJS->rootObjectUntilShutdown(obj);
    JS::RootedObject ob(cx, obj);
    JS_DefineFunctions(cx, ob, AudioContext_funcs);
    JS_DefineProperties(cx, ob, AudioContext_props);

    NIDIUM_PTHREAD_VAR_INIT(&m_ShutdownWait)

    m_Audio->postMessage(JSAudio::CtxCallback, static_cast<void *>(this), true);
}

bool JSAudio::createContext()
{
    if (m_JsRt != NULL) return false;

    if ((m_JsRt = JS_NewRuntime(128L * 1024L * 1024L, JS_USE_HELPER_THREADS)) == NULL) {
        printf("Failed to init JS runtime\n");
        return false;
    }

    JS_SetGCParameter(m_JsRt, JSGC_MAX_BYTES, 0xffffffff);
    JS_SetGCParameter(m_JsRt, JSGC_SLICE_TIME_BUDGET, 15);

    if ((m_JsTcx = JS_NewContext(m_JsRt, 8192)) == NULL) {
        printf("Failed to init JS context\n");
        return false;
    }

    JS_SetStructuredCloneCallbacks(m_JsRt, NidiumJS::m_JsScc);

    JSAutoRequest ar(m_JsTcx);

    //JS_SetGCParameterForThread(this->tcx, JSGC_MAX_CODE_CACHE_BYTES, 16 * 1024 * 1024);
    JS::CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);

    JS::RootedObject global(m_JsTcx, JS_NewGlobalObject(m_JsTcx,
        &Global_AudioThread_class, nullptr, JS::DontFireOnNewGlobalHook, options));

    JSAutoCompartment ac(m_JsTcx, global);

    m_JsGlobalObj = global;

    js::SetDefaultObjectForContext(m_JsTcx, global);
    if (!JS_InitStandardClasses(m_JsTcx, global)) {
        printf("Failed to init std class\n");
        return false;
    }
    JS_SetErrorReporter(m_JsTcx, reportError);
    JS_FireOnNewGlobalObject(m_JsTcx, global);
    JS_DefineFunctions(m_JsTcx, global, glob_funcs_threaded);
    JSConsole::RegisterObject(m_JsTcx);

    JS_SetRuntimePrivate(m_JsRt, NidiumJS::GetObject(m_Audio->getMainCtx()));

    //JS_SetContextPrivate(this->tcx, static_cast<void *>(this));

    return true;

}

bool JSAudio::run(char *str)
{
    if (!m_JsTcx) {
        printf("No JS context for audio thread\n");
        return false;
    }
    JSAutoRequest ar(m_JsTcx);
    JSAutoCompartment ac(m_JsTcx, m_JsGlobalObj);

    JS::CompileOptions options(m_JsTcx);
    JS::RootedObject globalObj(m_JsTcx, JS::CurrentGlobalOrNull(m_JsTcx));
    options.setIntroductionType("audio Thread").setUTF8(true);

    JS::RootedFunction fun(m_JsTcx, JS_CompileFunction(m_JsTcx, globalObj, "Audio_run", 0, nullptr, str, strlen(str), options));
    if (!fun.get()) {
        JS_ReportError(m_JsTcx, "Failed to compile script on audio thread\n");
        return false;
    }

    JS::RootedValue rval(m_JsTcx);
    JS_CallFunction(m_JsTcx, globalObj, fun, JS::HandleValueArray::empty(), &rval);

    return true;
}

void JSAudio::unroot()
{
    if (m_JSObject != NULL) {
        NJS->unrootObject(m_JSObject);
        m_JSObject = nullptr;
    }
}

void JSAudio::ShutdownCallback(void *custom)
{
    JSAudio *audio = static_cast<JSAudio *>(custom);
    JSAudio::Nodes *nodes = audio->m_Nodes;

    // Let's shutdown all custom nodes
    while (nodes != NULL) {
        if (nodes->curr->m_NodeType == Audio::CUSTOM ||
            nodes->curr->m_NodeType == Audio::CUSTOM_SOURCE) {
            nodes->curr->ShutdownCallback(nodes->curr->m_Node, nodes->curr);
        }

        nodes = nodes->next;
    }

    if (audio->m_JsTcx != NULL) {
        JSRuntime *rt = JS_GetRuntime(audio->m_JsTcx);

        JS_DestroyContext(audio->m_JsTcx);
        JS_DestroyRuntime(rt);

        audio->m_JsTcx = NULL;
    }

    NIDIUM_PTHREAD_SIGNAL(&audio->m_ShutdownWait)
}

JSAudio::~JSAudio()
{
    m_Audio->lockSources();
    m_Audio->lockQueue();

    // Unroot all js audio nodes
    this->unroot();

    // Delete all nodes
    JSAudio::Nodes *nodes = m_Nodes;
    JSAudio::Nodes *next = NULL;
    while (nodes != NULL) {
        next = nodes->next;
        // Node destructor will remove the node
        // from the nodes linked list
        delete nodes->curr;
        nodes = next;
    }

    // Unroot custom nodes objects and clear threaded js context
    m_Audio->postMessage(JSAudio::ShutdownCallback, this, true);

    NIDIUM_PTHREAD_WAIT(&m_ShutdownWait)

    // Unlock the sources, so the decode thread can exit
    // when we call Audio::shutdown()
    m_Audio->unlockSources();

    // Shutdown the audio
    m_Audio->shutdown();

    m_Audio->unlockQueue();

    // And delete the audio
    delete m_Audio;

    JSAudio::m_Instance = NULL;
}

void JSAudioNode::add()
{
    JSAudio::Nodes *nodes = new JSAudio::Nodes(this, NULL, m_Audio->m_Nodes);

    if (m_Audio->m_Nodes != NULL) {
        m_Audio->m_Nodes->prev = nodes;
    }

    m_Audio->m_Nodes = nodes;
}

void nidium_audio_node_custom_set_internal(JSContext *cx, JSAudioNode *node,
    JS::HandleObject obj, const char *name, JS::HandleValue val)
{
    JS_SetProperty(cx, obj, name, val);

    JSTransferableFunction *setterFn;
    setterFn = node->m_TransferableFuncs[JSAudioNode::SETTER_FN];
    if (setterFn) {
        JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
        JS::RootedString jstr(cx, JS_NewStringCopyZ(cx, name));
        JS::AutoValueArray<3> params(cx);
        params[0].setString(jstr);
        params[1].set(val);
        params[2].setObjectOrNull(global);

        JS::RootedValue rval(cx);
        setterFn->call(*node->m_NodeObj, params, &rval);
    }
}

void JSAudioNode::SetPropCallback(AudioNode *node, void *custom)
{
    JSContext *tcx;
    JSAudioNode::Message *msg;

    msg = static_cast<struct JSAudioNode::Message*>(custom);
    tcx = msg->jsNode->m_Audio->m_JsTcx;

    JSAutoRequest ar(tcx);
    JSAutoCompartment ac(tcx, msg->jsNode->m_Audio->m_JsGlobalObj);

    JS::RootedValue data(tcx);
    if (!JS_ReadStructuredClone(tcx, msg->clone.datap, msg->clone.nbytes, JS_STRUCTURED_CLONE_VERSION, &data, nullptr, NULL)) {
        JS_PROPAGATE_ERROR(tcx, "Failed to read structured clone");

        JS_free(msg->jsNode->getJSContext(), msg->name);
        JS_ClearStructuredClone(msg->clone.datap, msg->clone.nbytes, nullptr, NULL);
        delete msg;

        return;
    }
    if (msg->name == NULL) {
        JS::RootedObject props(tcx);
        JS_ValueToObject(tcx, data, &props);
        JS::AutoIdArray ida(tcx, JS_Enumerate(tcx, props));
        for (size_t i = 0; i < ida.length(); i++) {
            JS::RootedId id(tcx, ida[i]);
            JSAutoByteString name(tcx, JSID_TO_STRING(id));
            JS::RootedValue val(tcx);
            if (!JS_GetPropertyById(tcx, props, id, &val)) {
                break;
            }
            nidium_audio_node_custom_set_internal(tcx, msg->jsNode,
                    *msg->jsNode->m_HashObj, name.ptr(), val);
        }
    } else {
        nidium_audio_node_custom_set_internal(tcx, msg->jsNode,
                *msg->jsNode->m_HashObj, msg->name, data);
    }

    if (msg->name != NULL) {
        JS_free(msg->jsNode->getJSContext(), msg->name);
    }

    JS_ClearStructuredClone(msg->clone.datap, msg->clone.nbytes, nullptr, NULL);

    delete msg;
}

void JSAudioNode::CustomCallback(const struct NodeEvent *ev)
{
    JSAudioNode *thiz;
    JSContext *tcx;
    JSTransferableFunction *processFn;
    unsigned long size;
    int count;

    thiz = static_cast<JSAudioNode *>(ev->custom);

    if (!thiz->m_Audio->m_JsTcx || !thiz->m_Cx || !thiz->getJSObject() || !thiz->m_Node) {
        return;
    }

    tcx = thiz->m_Audio->m_JsTcx;

    JSAutoRequest ar(tcx);
    JSAutoCompartment ac(tcx, thiz->m_Audio->m_JsGlobalObj);

    processFn = thiz->m_TransferableFuncs[JSAudioNode::PROCESS_FN];

    if (!processFn) {
        return;
    }

    count = thiz->m_Node->m_InCount > thiz->m_Node->m_OutCount ? thiz->m_Node->m_InCount : thiz->m_Node->m_OutCount;
    size = thiz->m_Node->m_Audio->m_OutputParameters->m_BufferSize/2;

    JS::RootedObject obj(tcx, JS_NewObject(tcx, &AudioNodeEvent_class, JS::NullPtr(), JS::NullPtr()));
    JS::RootedObject frames(tcx, JS_NewArrayObject(tcx, 0));
    for (int i = 0; i < count; i++) {
        uint8_t *data;

        // TODO : Avoid memcpy (custom allocator for AudioNode?)
        JS::RootedObject arrBuff(tcx, JS_NewArrayBuffer(tcx, size));
        data = JS_GetArrayBufferData(arrBuff);
        memcpy(data, ev->data[i], size);
        JS::RootedObject arr(tcx, JS_NewFloat32ArrayWithBuffer(tcx, arrBuff, 0, -1));
        if (arr.get()) {
            JS_DefineElement(tcx, frames, i, OBJECT_TO_JSVAL(arr), nullptr, nullptr,
                 JSPROP_ENUMERATE | JSPROP_PERMANENT);
        } else {
            JS_ReportOutOfMemory(tcx);
            return;
        }
    }

    JS::RootedObject global(tcx, JS::CurrentGlobalOrNull(tcx));
    JS::RootedValue vFrames(tcx, OBJECT_TO_JSVAL(frames));
    JS::RootedValue vSize(tcx, DOUBLE_TO_JSVAL(ev->size));

    JS_DefineProperty(tcx, obj, "data", vFrames, JSPROP_PERMANENT | JSPROP_ENUMERATE);
    JS_DefineProperty(tcx, obj, "size", vSize, JSPROP_PERMANENT | JSPROP_ENUMERATE);

    JS::AutoValueArray<2> params(tcx);
    params[0].setObjectOrNull(obj);
    params[1].setObjectOrNull(global);

    JS::RootedValue rval(tcx);
    processFn->call(*thiz->m_NodeObj, params, &rval);

    for (int i = 0; i < count; i++) {
        JS::RootedValue val(tcx);
        JS_GetElement(tcx, frames, i, &val);
        if (!val.isObject()) {
            continue;
        }

        JS::RootedObject arr(tcx, val.toObjectOrNull());
        if (!arr.get() || !JS_IsFloat32Array(arr) || JS_GetTypedArrayLength(arr) != ev->size) {
            continue;
        }

        memcpy(ev->data[i], JS_GetFloat32ArrayData(arr), size);
    }
}

void JSAudioNode::onMessage(const SharedMessages::Message &msg)
{
    if (m_IsDestructing) return;
    JS::RootedObject obj(m_Cx, m_JSObject);

    const char *evName = nullptr;
    JS::RootedValue ev(m_Cx);

    evName = JSAVEventRead(msg.event());
    if (!evName) {
        return;
    }

    ev = consumeSourceMessage(m_Cx, obj, msg);

    if (!ev.isNull()) {
        this->fireJSEvent(evName, &ev);
    }
}

void JSAudioNode::onEvent(const struct AVSourceEvent *cev)
{
    JSAudioNode *jnode = static_cast<JSAudioNode *>(cev->m_Custom);
    jnode->postMessage((void *)cev, cev->m_Ev);
}

void JSAudio::RunCallback(void *custom)
{
    JSAudio *audio = JSAudio::GetContext();

    if (!audio) return; // This should not happend

    char *str = static_cast<char *>(custom);
    audio->run(str);
    JS_free(audio->getJSContext(), custom);
}


void JSAudio::CtxCallback(void *custom)
{
    JSAudio *audio = static_cast<JSAudio*>(custom);

    if (!audio->createContext()) {
        printf("Failed to create audio thread context\n");
        //JS_ReportError(jsNode->audio->cx, "Failed to create audio thread context\n");
        // XXX : Can't report error from another thread?
    }
}

void JSAudioNode::ShutdownCallback(AudioNode *nnode, void *custom)
{
    JSAudioNode *node = static_cast<JSAudioNode *>(custom);
    JSAutoRequest ar(node->m_Audio->m_JsTcx);

    delete node->m_NodeObj;
    delete node->m_HashObj;

    // JSTransferableFunction need to be destroyed on the JS thread
    // since they belong to it
    for (int i = 0; i < JSAudioNode::END_FN; i++) {
        delete node->m_TransferableFuncs[i];
        node->m_TransferableFuncs[i] = NULL;
    }

    NIDIUM_PTHREAD_SIGNAL(&node->m_ShutdownWait)
}

void JSAudioNode::DeleteTransferableFunc(AudioNode *node, void *custom)
{
    JSTransferableFunction *fun = static_cast<JSTransferableFunction*>(custom);
    delete fun;
}

void JSAudioNode::InitCustomObject(AudioNode *node, void *custom)
{
    JSAudioNode *jnode = static_cast<JSAudioNode *>(custom);
    JSContext *tcx = jnode->m_Audio->m_JsTcx;

    if (jnode->m_HashObj || jnode->m_NodeObj) {
        return;
    }

    JSAutoRequest ar(tcx);
    JS::RootedObject global(tcx, jnode->m_Audio->m_JsGlobalObj);
    JSAutoCompartment ac(tcx, global);

    jnode->m_HashObj = new JS::PersistentRootedObject(tcx,
            JS_NewObject(tcx, nullptr, JS::NullPtr(), JS::NullPtr()));
    if (!jnode->m_HashObj) {
        JS_PROPAGATE_ERROR(tcx, "Failed to create hash object for custom node");
        return;
    }

    jnode->m_NodeObj = new JS::PersistentRootedObject(tcx,
            JS_NewObject(tcx, &AudioNode_threaded_class, JS::NullPtr(), JS::NullPtr()));
    if (!jnode->m_NodeObj) {
        JS_PROPAGATE_ERROR(tcx, "Failed to create node object for custom node");
        return;
    }

    JS_DefineFunctions(tcx, *jnode->m_NodeObj, AudioNodeCustom_threaded_funcs);
    JS_SetPrivate(*jnode->m_NodeObj, static_cast<void *>(jnode));

    JSTransferableFunction *initFn =
        jnode->m_TransferableFuncs[JSAudioNode::INIT_FN];

    if (initFn) {
        JS::AutoValueArray<1> params(tcx);
        JS::RootedValue glVal(tcx, OBJECT_TO_JSVAL(global));
        params[0].set(glVal);

        JS::RootedValue rval(tcx);
        initFn->call(*jnode->m_NodeObj, params, &rval);

        jnode->m_TransferableFuncs[JSAudioNode::INIT_FN] = NULL;

        delete initFn;
    }
}

JSAudioNode::~JSAudioNode()
{
    JSAudio::Nodes *nodes = m_Audio->m_Nodes;
    JSAutoRequest ar(m_Cx);

    m_IsDestructing = true;

    // Block Audio threads execution.
    // While the node is destructed we don't want any thread
    // to call some method on a node that is being destroyed
    m_Audio->m_Audio->lockQueue();
    m_Audio->m_Audio->lockSources();

    // Wakeup audio thread. This will flush all pending messages.
    // That way, we are sure nothing will need to be processed
    // later for this node.
    m_Audio->m_Audio->wakeup();

    if (m_NodeType == Audio::SOURCE) {
        // Only source from Video has reserved slot
        JS::RootedValue source(m_Cx, JS_GetReservedSlot(m_JSObject, 0));
        JS::RootedObject obj(m_Cx, source.toObjectOrNull());
        if (obj.get()) {
            // If it exist, we must inform the video
            // that audio node no longer exist
            JSVideo *video = (JSVideo *)JS_GetPrivate(obj);
            if (video != NULL) {
                JS_SetReservedSlot(m_JSObject, 0, JSVAL_NULL);
                video->stopAudio();
            }
        }
    }

    // Remove JS node from nodes linked list
    while (nodes != NULL) {
        if (nodes->curr == this) {
            if (nodes->prev != NULL) {
                nodes->prev->next = nodes->next;
            } else {
                m_Audio->m_Nodes = nodes->next;
            }

            if (nodes->next != NULL) {
                nodes->next->prev = nodes->prev;
            }

            delete nodes;

            break;
        }
        nodes = nodes->next;
    }


    // Custom nodes and sources must release all JS object on the JS thread
    if (m_Node != NULL && m_Audio->m_JsTcx != NULL &&
            (m_NodeType == Audio::CUSTOM ||
             m_NodeType == Audio::CUSTOM_SOURCE)) {

        m_Node->callback(JSAudioNode::ShutdownCallback, this, true);

        NIDIUM_PTHREAD_WAIT(&m_ShutdownWait);
    }

    if (m_ArrayContent != NULL) {
        free(m_ArrayContent);
    }

    if (m_JSObject != NULL) {
        JS_SetPrivate(m_JSObject, nullptr);
        m_JSObject = nullptr;
    }

    delete m_Node;

    m_Audio->m_Audio->unlockQueue();
    m_Audio->m_Audio->unlockSources();
}


static bool nidium_audio_pFFT(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    int dir, n;
    double *dx, *dy;
    uint32_t dlenx, dleny;

    JS::RootedObject x(cx);
    JS::RootedObject y(cx);
    if (!JS_ConvertArguments(cx, args, "ooii", x.address(), y.address(), &n, &dir)) {
        return false;
    }

    if (!JS_IsTypedArrayObject(x) || !JS_IsTypedArrayObject(y)) {
        JS_ReportError(cx, "Bad argument");
        return false;
    }

    if (JS_GetObjectAsFloat64Array(x, &dlenx, &dx) == NULL) {
        JS_ReportError(cx, "Can't convert typed array (expected Float64Array)");
        return false;
    }
    if (JS_GetObjectAsFloat64Array(y, &dleny, &dy) == NULL) {
        JS_ReportError(cx, "Can't convert typed array (expected Float64Array)");
        return false;
    }

    if (dlenx != dleny) {
        JS_ReportError(cx, "Buffers size must match");
        return false;
    }

    if ((n & (n -1)) != 0 || n < 32 || n > 4096) {
        JS_ReportError(cx, "Invalid frame size");
        return false;
    }

    if (n > dlenx) {
        JS_ReportError(cx, "Buffer is too small");
        return false;
    }

    FFT(dir, n, dx, dy);

    return true;
}

static bool nidium_Audio_constructor(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS_ReportError(cx, "Illegal constructor");
    return false;
}

static bool nidium_audio_getcontext(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    unsigned int bufferSize, channels, sampleRate;

    if (argc > 0) {
        JS::ToUint32(cx, args[0], &bufferSize);
    } else {
        bufferSize = 0;
    }

    if (argc > 1) {
        JS::ToUint32(cx, args[1], &channels);
    } else {
        channels = 2;
    }

    if (argc > 2) {
        JS::ToUint32(cx, args[2], &sampleRate);
    } else {
        sampleRate = 44100;
    }

    switch (bufferSize) {
        case 0:
        case 128:
        case 256:
        case 512:
        case 1024:
        case 2048:
        case 4096:
        case 8192:
        case 16384:
            // Supported buffer size
            // Multiply by 8 to get the bufferSize in bytes
            // rather than in samples per buffer
            bufferSize *= 8;
            break;
        default :
            JS_ReportError(cx, "Unsuported buffer size %d. "
                "Supported values are : 0, 128, 256, 512, 1024, 2048, 4096, 8192, 16384\n", bufferSize);
            return false;
            break;
    }

    if (channels < 1 || channels > 32) {
        JS_ReportError(cx, "Unsuported channels number %d. Channels must be between 1 and 32\n", channels);
        return false;
    }

    if (sampleRate < 22050 || sampleRate> 96000) {
        JS_ReportError(cx, "Unsuported sample rate %dKHz. Sample rate must be between 22050 and 96000\n", sampleRate);
        return false;
    }

    bool paramsChanged = false;
    JSAudio *jaudio = JSAudio::GetContext();

    if (jaudio) {
        AudioParameters *params = jaudio->m_Audio->m_OutputParameters;
        if (params->m_AskedBufferSize != bufferSize ||
            params->m_Channels != channels ||
            params->m_SampleRate != sampleRate) {
            paramsChanged = true;
        }
    }

    if (!paramsChanged && jaudio) {
        JS::RootedObject retObj(cx, jaudio->getJSObject());
        args.rval().setObjectOrNull(retObj);
        return true;
    }

    if (paramsChanged) {
        JSContext *m_Cx = cx;
        JS_SetPrivate(jaudio->getJSObject(), NULL);
        NJS->unrootObject(jaudio->getJSObject());
        delete jaudio;
    }

    JS::RootedObject ret(cx, JS_NewObjectForConstructor(cx, &AudioContext_class, args));

    JSAudio *naudio = JSAudio::GetContext(cx, ret, bufferSize, channels, sampleRate);

    if (naudio == NULL) {
        JS_ReportError(cx, "Failed to initialize audio context\n");
        return false;
    }

    args.rval().setObjectOrNull(ret);

    return true;
}

static bool nidium_audio_run(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JSAudio *audio = JSAudio::GetContext();

    CHECK_INVALID_CTX(audio);

    JS::RootedString fn(cx);
    JS::RootedFunction nfn(cx);
    if ((nfn = JS_ValueToFunction(cx, args[0])) == NULL ||
        (fn = JS_DecompileFunctionBody(cx, nfn, 0)) == NULL) {
        JS_ReportError(cx, "Failed to read callback function\n");
        return false;
    }

    char *funStr = JS_EncodeString(cx, fn);
    if (!funStr) {
        JS_ReportError(cx, "Failed to convert callback function to source string");
        return false;
    }

    audio->m_Audio->postMessage(JSAudio::RunCallback, static_cast<void *>(funStr));

    return true;
}

static bool nidium_audio_load(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS_ReportError(cx, "Not implemented");
    return false;
}

static bool nidium_audio_createnode(JSContext *cx, unsigned argc, JS::Value *vp)
{
    int in, out;
    JSAudio *audio;
    JSAudioNode *node;

    NIDIUM_JS_PROLOGUE_CLASS_NO_RET(JSAudio, &AudioContext_class);
    audio = CppObj;
    node = NULL;

    JS::RootedString name(cx);
    if (!JS_ConvertArguments(cx, args, "Suu", name.address(), &in, &out)) {
        return false;
    }

    if (in == 0 && out == 0) {
        JS_ReportError(cx, "Node must have at least one input or output");
        return false;
    } else if (in < 0 || out < 0) {
        JS_ReportError(cx, "Wrong channel count (Must be greater or equal to 0)");
        return false;
    } else if (in > 32 || out > 32) {
        JS_ReportError(cx, "Wrong channel count (Must be lower or equal to 32)");
        return false;
    }

    JS::RootedObject ret(cx, JS_NewObjectForConstructor(cx, &AudioNode_class, args));
    if (!ret) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    JS_SetReservedSlot(ret, 0, JSVAL_NULL);

    JSAutoByteString cname(cx, name);
    try {
        if (strcmp("source", cname.ptr()) == 0) {
            node = new JSAudioNode(ret, cx, Audio::SOURCE, in, out, audio);

            AudioSource *source = static_cast<AudioSource*>(node->m_Node);
            source->eventCallback(JSAudioNode::onEvent, node);

            JS_DefineFunctions(cx, ret, AudioNodeSource_funcs);
            JS_DefineProperties(cx, ret, AudioNodeSource_props);
        } else if (strcmp("custom-source", cname.ptr()) == 0) {
            node = new JSAudioNode(ret, cx, Audio::CUSTOM_SOURCE, in, out, audio);

            AudioSourceCustom *source = static_cast<AudioSourceCustom*>(node->m_Node);
            source->eventCallback(JSAudioNode::onEvent, node);

            JS::RootedValue tmp(cx, JS::NumberValue(0));
            JS_DefineProperty(cx, ret, "position", tmp,
                    JSPROP_PERMANENT | JSPROP_ENUMERATE | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,
                    JS_CAST_NATIVE_TO(nidium_audionode_custom_source_position_getter, JSPropertyOp),
                    JS_CAST_NATIVE_TO(nidium_audionode_custom_source_position_setter, JSStrictPropertyOp));

            JS_DefineFunctions(cx, ret, AudioNodeCustom_funcs);
            JS_DefineFunctions(cx, ret, AudioNodeCustomSource_funcs);
        } else if (strcmp("custom", cname.ptr()) == 0) {
            node = new JSAudioNode(ret, cx, Audio::CUSTOM, in, out, audio);
            JS_DefineFunctions(cx, ret, AudioNodeCustom_funcs);
        } else if (strcmp("reverb", cname.ptr()) == 0) {
            node = new JSAudioNode(ret, cx, Audio::REVERB, in, out, audio);
        } else if (strcmp("delay", cname.ptr()) == 0) {
            node = new JSAudioNode(ret, cx, Audio::DELAY, in, out, audio);
        } else if (strcmp("gain", cname.ptr()) == 0) {
            node = new JSAudioNode(ret, cx, Audio::GAIN, in, out, audio);
        } else if (strcmp("target", cname.ptr()) == 0) {
            if (audio->m_Target != NULL) {
                JS::RootedObject retObj(cx, audio->m_Target->getJSObject());
                args.rval().setObjectOrNull(retObj);
                return true;
            } else {
                node = new JSAudioNode(ret, cx, Audio::TARGET, in, out, audio);
                audio->m_Target = node;
            }
        } else if (strcmp("stereo-enhancer", cname.ptr()) == 0) {
            node = new JSAudioNode(ret, cx, Audio::STEREO_ENHANCER, in, out, audio);
        } else {
            JS_ReportError(cx, "Unknown node name : %s\n", cname.ptr());
            return false;
        }
    } catch (AudioNodeException *e) {
        delete node;
        JS_ReportError(cx, "Error while creating node : %s\n", e->what());
        return false;
    }

    if (node == NULL || node->m_Node == NULL) {
        delete node;
        JS_ReportError(cx, "Error while creating node : %s\n", cname.ptr());
        return false;
    }

    audio->initNode(node, ret, name);

    args.rval().setObjectOrNull(ret);

    return true;
}

static bool nidium_audio_connect(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NodeLink *nlink1;
    NodeLink *nlink2;
    JSAudio *jaudio;

    NIDIUM_JS_PROLOGUE_CLASS_NO_RET(JSAudio, &AudioContext_class);
    jaudio = CppObj;
    Audio *audio = jaudio->m_Audio;

    JS::RootedObject link1(cx);
    JS::RootedObject link2(cx);
    if (!JS_ConvertArguments(cx, args, "oo", link1.address(), link2.address())) {
        return false;
    }

    nlink1 = (NodeLink *)JS_GetInstancePrivate(cx, link1, &AudioNodeLink_class, &args);
    nlink2 = (NodeLink *)JS_GetInstancePrivate(cx, link2, &AudioNodeLink_class, &args);

    if (nlink1 == NULL || nlink2 == NULL) {
        JS_ReportError(cx, "Bad AudioNodeLink\n");
        return false;
    }

    if (nlink1->m_Type == AV::INPUT && nlink2->m_Type == AV::OUTPUT) {
        if (!audio->connect(nlink2, nlink1)) {
            JS_ReportError(cx, "connect() failed (max connection reached)\n");
            return false;
        }
    } else if (nlink1->m_Type == AV::OUTPUT && nlink2->m_Type == AV::INPUT) {
        if (!audio->connect(nlink1, nlink2)) {
            JS_ReportError(cx, "connect() failed (max connection reached)\n");
            return false;
        }
    } else {
        JS_ReportError(cx, "connect() take one input and one output\n");
        return false;
    }

    args.rval().setUndefined();

    return true;
}

static bool nidium_audio_disconnect(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NodeLink *nlink1;
    NodeLink *nlink2;
    JSAudio *jaudio;

    NIDIUM_JS_PROLOGUE_CLASS(JSAudio, &AudioContext_class);
    jaudio = CppObj;
    Audio *audio = jaudio->m_Audio;

    JS::RootedObject link1(cx);
    JS::RootedObject link2(cx);
    if (!JS_ConvertArguments(cx, args, "oo", link1.address(), link2.address())) {
        return true;
    }

    nlink1 = (NodeLink *)JS_GetInstancePrivate(cx, link1, &AudioNodeLink_class, &args);
    nlink2 = (NodeLink *)JS_GetInstancePrivate(cx, link2, &AudioNodeLink_class, &args);

    if (nlink1 == NULL || nlink2 == NULL) {
        JS_ReportError(cx, "Bad AudioNodeLink\n");
        return false;
    }

    if (nlink1->m_Type == AV::INPUT && nlink2->m_Type == AV::OUTPUT) {
        audio->disconnect(nlink2, nlink1);
    } else if (nlink1->m_Type == AV::OUTPUT && nlink2->m_Type == AV::INPUT) {
        audio->disconnect(nlink1, nlink2);
    } else {
        JS_ReportError(cx, "disconnect() take one input and one output\n");
        return false;
    }

    return true;
}

static bool nidium_audiothread_print(JSContext *cx, unsigned argc, JS::Value *vp)
{
    if (argc == 0) {
        return true;
    }

    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    char *bytes;

    JS::RootedString str(cx, args[0].toString());
    if (!str) {
        return true;
    }

    bytes = JS_EncodeString(cx, str);
    if (!bytes)
        return true;

    printf("%s\n", bytes);

    JS_free(cx, bytes);

    return true;
}

static bool nidium_audio_prop_setter(JSContext *cx, JS::HandleObject obj, uint8_t id, bool strict, JS::MutableHandleValue vp)
{
    JSAudio *jaudio = JSAudio::GetContext();

    CHECK_INVALID_CTX(jaudio);

    if (vp.isNumber()) {
        jaudio->m_Audio->setVolume((float)vp.toNumber());
    }

    return true;
}

static bool nidium_audio_prop_getter(JSContext *cx, JS::HandleObject obj, uint8_t id, JS::MutableHandleValue vp)
{
    JSAudio *jaudio = JSAudio::GetContext();

    CHECK_INVALID_CTX(jaudio);

    AudioParameters *params = jaudio->m_Audio->m_OutputParameters;

    switch(id) {
        case AUDIO_PROP_BUFFERSIZE:
            vp.setInt32(params->m_BufferSize/8);
        break;
        case AUDIO_PROP_CHANNELS:
            vp.setInt32(params->m_Channels);
        break;
        case AUDIO_PROP_SAMPLERATE:
            vp.setInt32(params->m_SampleRate);
        break;
        case AUDIO_PROP_VOLUME:
            vp.setNumber(jaudio->m_Audio->getVolume());
        break;
        default:
            return false;
        break;
    }

    return true;
}

static bool nidium_AudioNode_constructor(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS_ReportError(cx, "Illegal constructor");
    return false;
}

static void nidium_audionode_set_internal(JSContext *cx, AudioNode *node, const char *prop, JS::HandleValue val)
{
    ArgType type;
    void *value;
    int intVal = 0;
    double doubleVal = 0;
    unsigned long size;

    if (val.isInt32()) {
        type = AV::INT;
        size = sizeof(int);
        intVal = (int) val.toInt32();
        value = &intVal;
    } else if (val.isDouble()) {
        type = AV::DOUBLE;
        size = sizeof(double);
        doubleVal = val.toNumber();
        value = &doubleVal;
    } else {
        JS_ReportError(cx, "Unsuported value\n");
        return;
    }

    if (!node->set(prop, type, value, size)) {
        JS_ReportError(cx, "Unknown argument name %s\n", prop);
    }
}
static bool nidium_audionode_set(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSAudioNode *jnode;

    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);
    jnode = CppObj;

    AudioNode *node = jnode->m_Node;

    NIDIUM_JS_CHECK_ARGS("set", 1)

    if (args[0].isObject()) {
        JS::RootedObject props(cx, args[0].toObjectOrNull());

        if (!props) {
            JS_ReportError(cx, "Invalid argument");
            return false;
        }

        JS::AutoIdArray ida(cx, JS_Enumerate(cx, props));

        for (size_t i = 0; i < ida.length(); i++) {
            JS::RootedId id(cx, ida[i]);
            JSAutoByteString cname(cx, JSID_TO_STRING(id));
            JS::RootedValue val(cx);
            if (!JS_GetPropertyById(cx, props, id, &val)) {
                break;
            }
            nidium_audionode_set_internal(cx, node, cname.ptr(), val);
        }
    } else {
        NIDIUM_JS_CHECK_ARGS("set", 2)

        JS::RootedString name(cx);
        if (!JS_ConvertArguments(cx, args, "S", name.address())) {
            return false;
        }

        JSAutoByteString cname(cx, name);
        nidium_audionode_set_internal(cx, node, cname.ptr(), args[1]);
    }

    return true;
}

static bool nidium_audionode_get(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS_ReportError(cx, "Not implemented");
    return true;
}

static bool nidium_audionode_custom_set(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSAudioNode::Message *msg;
    AudioNodeCustom *node;
    JSAudioNode *jnode;

    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);
    jnode = CppObj;

    JS::RootedString name(cx);
    JS::RootedValue val(cx);

    if (argc == 1) {
        if (!args[0].isPrimitive()) {
            val = args[0];
        } else {
            JS_ReportError(cx, "Invalid argument");
            return false;
        }
    } else if (argc == 2) {
        if (!args[0].isString()) {
            JS_ReportError(cx, "First argument must be a string");
            return false;
        }

        if (!JS_ConvertArguments(cx, args, "S", name.address())) {
            return false;
        }

        val = args[1];
    } else {
        JS_ReportError(cx, "Invalid arguments");
        return false;
    }

    node = static_cast<AudioNodeCustom *>(jnode->m_Node);

    msg = new JSAudioNode::Message();

    if (name && JS_GetStringLength(name) > 0) {
        msg->name = JS_EncodeString(cx, name);
    }
    msg->jsNode = jnode;

    if (!JS_WriteStructuredClone(cx, val, &msg->clone.datap, &msg->clone.nbytes, nullptr, nullptr, JS::NullHandleValue)) {
        JS_ReportError(cx, "Failed to write structured clone");

        JS_free(cx, msg->name);
        delete msg;

        return false;
    }

    if (!jnode->m_NodeObj) {
        node->callback(JSAudioNode::InitCustomObject, static_cast<void *>(jnode));
    }

    node->callback(JSAudioNode::SetPropCallback, msg, true);

    return true;
}

static bool nidium_audionode_custom_assign(JSContext *cx, JSAudioNode *jnode, \
    CustomNodeCallbacks callbackID, JS::HandleValue callback)
{
    JSTransferableFunction *fun;
    JSAudioNode::TransferableFunction funID;
    AudioNode *node = static_cast<AudioNode *>(jnode->m_Node);

    switch (callbackID) {
        case NODE_CUSTOM_PROCESS_CALLBACK:
            funID = JSAudioNode::PROCESS_FN;
        break;
        case NODE_CUSTOM_SETTER_CALLBACK:
            funID = JSAudioNode::SETTER_FN;
        break;
        case NODE_CUSTOM_INIT_CALLBACK:
            funID = JSAudioNode::INIT_FN;
        break;
        case NODE_CUSTOM_SEEK_CALLBACK:
            funID = JSAudioNode::SEEK_FN;
        break;
        default:
            return true;
        break;
    }

    if (jnode->m_TransferableFuncs[funID] != NULL) {
        node->callback(JSAudioNode::DeleteTransferableFunc, static_cast<void *>(jnode->m_TransferableFuncs[funID]));
        jnode->m_TransferableFuncs[funID] = NULL;
    }

    if (!callback.isObject() || !JS_ObjectIsCallable(cx, &callback.toObject())) {
        return true;
    }

    fun = new JSTransferableFunction(jnode->m_Audio->m_JsTcx);

    if (!fun->prepare(cx, callback)) {
        JS_ReportError(cx, "Failed to read custom node callback function\n");
        delete fun;
        return false;
    }

    jnode->m_TransferableFuncs[funID] = fun;

    if (!jnode->m_NodeObj) {
        node->callback(JSAudioNode::InitCustomObject, static_cast<void *>(jnode));
    }

    return true;
}

static bool nidium_audionode_custom_assign_processor(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);
    NIDIUM_JS_CHECK_ARGS("assignProcessor", 1);

    AudioNodeCustom *node = static_cast<AudioNodeCustom *>(CppObj->m_Node);

    if (!nidium_audionode_custom_assign(cx, CppObj, NODE_CUSTOM_PROCESS_CALLBACK, args[0])) {
        return false;
    }

    node->setCallback(JSAudioNode::CustomCallback, static_cast<void *>(CppObj));

    return true;
}

static bool nidium_audionode_custom_assign_init(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);
    NIDIUM_JS_CHECK_ARGS("assignInit", 1);

    return nidium_audionode_custom_assign(cx, CppObj, NODE_CUSTOM_INIT_CALLBACK, args[0]);
}

static bool nidium_audionode_custom_assign_setter(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);
    NIDIUM_JS_CHECK_ARGS("assignSetter", 1);

    return nidium_audionode_custom_assign(cx, CppObj, NODE_CUSTOM_SETTER_CALLBACK, args[0]);
}

static bool nidium_audionode_custom_assign_seek(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);
    NIDIUM_JS_CHECK_ARGS("assignSeek", 1);

    AudioSourceCustom *node = static_cast<AudioSourceCustom *>(CppObj->m_Node);

    if (!nidium_audionode_custom_assign(cx, CppObj, NODE_CUSTOM_SEEK_CALLBACK, args[0])) {
        return false;
    }

    node->setSeek(JSAudioNode::SeekCallback, CppObj);

    return true;
}

static bool nidium_audionode_custom_source_position_setter(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);

    AudioSourceCustom *source = static_cast<AudioSourceCustom*>(CppObj->m_Node);

    if (args[0].isNumber()) {
        source->seek(args[0].toNumber());
    }

    return true;
}

static bool nidium_audionode_custom_source_position_getter(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);

    args.rval().setDouble(0.0);
    return true;
}

#if 0
static bool nidium_audionode_custom_get(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSAudioNode *jsNode;

    printf("hello get\n");
    printf("get node\n");

    jsNode = GET_NODE(JS_THIS_OBJECT(cx, vp));

    printf("convert\n");
    JS::RootedString name(cx);
    if (!JS_ConvertArguments(cx, m_Args, "S", name, address())) {
        return true;
    }

    printf("str\n");
    JSAutoByteString str(cx, name);
    JS_SetRuntimeThread(JS_GetRuntime(cx));

    printf("get\n");
    JS::RootedObject m_HashObj(cx, jnode->m_HashObj);
    JS::RootedValuel val(cx);
    JS_GetProperty(jsNode->m_Audio->m_JsTcx, m_HashObj, str.ptr(), val.address());

    printf("return\n");
    JS_ClearRuntimeThread(JS_GetRuntime(cx));

    m_Args.rval().set(val);

    return true;
}
#endif

static bool nidium_audionode_custom_threaded_set(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSAudioNode *jnode;
    JSTransferableFunction *fn;

    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_threaded_class);
    jnode = CppObj;

    if (argc != 2) {
        JS_ReportError(cx, "set() require two arguments\n");
        return false;
    }

    JS::RootedString name(cx);
    if (!JS_ConvertArguments(cx, args, "S", name.address())) {
        return false;
    }

    fn = jnode->m_TransferableFuncs[JSAudioNode::SETTER_FN];
    JS::RootedValue val(cx, args[1]);
    JSAutoByteString str(cx, name);
    JS_SetProperty(cx, *jnode->m_HashObj, str.ptr(), val);

    if (fn) {
        JS::AutoValueArray<3> params(cx);
        JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));

        params[0].setString(name);
        params[1].set(val);
        params[2].setObjectOrNull(global);

        JS::RootedValue rval(cx);
        fn->call(*jnode->m_NodeObj, params, &rval);
    }

    return true;
}

static bool nidium_audionode_custom_threaded_get(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSAudioNode *jnode;

    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_threaded_class);
    jnode = CppObj;

    if (!jnode->m_HashObj) {
        return false;
    }

    JS::RootedString name(cx);
    if (!JS_ConvertArguments(cx, args, "S", name.address())) {
        return false;
    }

    JSAutoByteString str(cx, name);
    JS::RootedValue val(cx);

    JS_GetProperty(jnode->m_Audio->m_JsTcx, *jnode->m_HashObj, str.ptr(), &val);

    args.rval().set(val);

    return true;
}

static bool nidium_audionode_custom_threaded_send(JSContext *cx, unsigned argc, JS::Value *vp)
{
    uint64_t *datap;
    size_t nbytes;
    JSAudioNode *jnode;

    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_threaded_class);
    NIDIUM_JS_CHECK_ARGS("send", 1);

    jnode = CppObj;

    struct nidium_thread_msg *msg;
    if (!JS_WriteStructuredClone(cx, args[0], &datap, &nbytes, nullptr, nullptr, JS::NullHandleValue)) {
        JS_ReportError(cx, "Failed to write structured clone");
        return false;
    }

    msg = new struct nidium_thread_msg;

    msg->data   = datap;
    msg->nbytes = nbytes;
    msg->callee = jnode->getJSObject();

    jnode->postMessage(msg, CUSTOM_SOURCE_SEND);

    return true;
}

static bool nidium_audionode_source_open(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSAudioNode *jnode;

    NIDIUM_JS_PROLOGUE_CLASS(JSAudioNode, &AudioNode_class);
    jnode = CppObj;

    AudioSource *source = (AudioSource *)jnode->m_Node;

    JS::RootedValue src(cx, args[0]);

    int ret = -1;

    if (src.isString()) {
        JSAutoByteString csrc(cx, src.toString());
        ret = source->open(csrc.ptr());
    } else if (src.isObject()) {
        JS::RootedObject arrayBuff(cx, src.toObjectOrNull());
        if (!JS_IsArrayBufferObject(arrayBuff)) {
            JS_ReportError(cx, "Data is not an ArrayBuffer\n");
            return false;
        }
        int length = JS_GetArrayBufferByteLength(arrayBuff);
        jnode->m_ArrayContent = JS_StealArrayBufferContents(cx, arrayBuff);
        ret = source->open(jnode->m_ArrayContent, length);
    } else {
        JS_ReportError(cx, "Invalid argument", ret);
        return false;
    }

    if (ret < 0) {
        JS_ReportError(cx, "Failed to open stream %d\n", ret);
        return false;
    }

    return true;
}
static bool nidium_audionode_source_play(JSContext *cx, unsigned argc, JS::Value *vp)
{
    AudioSource *source;

    GET_NODE(AudioSource, source);

    source->play();

    // play() may call the JS "onready" callback in a synchronous way
    // thus, if an exception happen in the callback, we should return false
    return !JS_IsExceptionPending(cx);
}

static bool nidium_audionode_source_pause(JSContext *cx, unsigned argc, JS::Value *vp)
{
    AudioSource *source;

    GET_NODE(AudioSource, source);

    source->pause();

    return !JS_IsExceptionPending(cx);
}

static bool nidium_audionode_source_stop(JSContext *cx, unsigned argc, JS::Value *vp)
{
    AudioSource *source;

    GET_NODE(AudioSource, source);

    source->stop();

    return !JS_IsExceptionPending(cx);
}

static bool nidium_audionode_source_close(JSContext *cx, unsigned argc, JS::Value *vp)
{
    AudioSource *source;

    GET_NODE(AudioSource, source);

    source->close();

    return true;
}

static bool nidium_audionode_source_prop_getter(JSContext *cx, JS::HandleObject obj,
    uint8_t id, JS::MutableHandleValue vp)
{
    JSAudioNode *jnode = (JSAudioNode *)JS_GetPrivate(obj);

    CHECK_INVALID_CTX(jnode);

    AudioSource *source = static_cast<AudioSource *>(jnode->m_Node);

    return JSAVSource::PropGetter(source, cx, id, vp);
}

static bool nidium_audionode_source_prop_setter(JSContext *cx, JS::HandleObject obj,
    uint8_t id, bool strict, JS::MutableHandleValue vp)
{
    JSAudioNode *jnode = (JSAudioNode *)JS_GetPrivate(obj);

    CHECK_INVALID_CTX(jnode);

    AudioSource *source = static_cast<AudioSource*>(jnode->m_Node);

    return JSAVSource::PropSetter(source, id, vp);
}

static bool nidium_audionode_custom_source_play(JSContext *cx, unsigned argc, JS::Value *vp)
{
    AudioSourceCustom *source;

    GET_NODE(AudioSourceCustom, source);

    source->play();

    // play() may call the JS "onready" callback in a synchronous way
    // thus, if an exception happen in the callback, we should return false
    return !JS_IsExceptionPending(cx);
}

static bool nidium_audionode_custom_source_pause(JSContext *cx, unsigned argc, JS::Value *vp)
{
    AudioSourceCustom *source;

    GET_NODE(AudioSourceCustom, source);

    source->pause();

    return !JS_IsExceptionPending(cx);
}

static bool nidium_audionode_custom_source_stop(JSContext *cx, unsigned argc, JS::Value *vp)
{
    AudioSourceCustom *source;

    GET_NODE(AudioSourceCustom, source);

    source->stop();

    return !JS_IsExceptionPending(cx);
}

static bool nidium_video_prop_getter(JSContext *cx, JS::HandleObject obj, uint8_t id, JS::MutableHandleValue vp)
{
    JSVideo *v = (JSVideo *)JS_GetPrivate(obj);
    if (v == NULL) {
        return false;
    }

    switch (id) {
        case VIDEO_PROP_WIDTH:
            vp.setInt32(v->m_Video->m_CodecCtx->width);
        break;
        case VIDEO_PROP_HEIGHT:
            vp.setInt32(v->m_Video->m_CodecCtx->height);
        break;
        default:
            return JSAVSource::PropGetter(v->m_Video, cx, id, vp);
            break;
    }

    return true;
}

static bool nidium_video_prop_setter(JSContext *cx, JS::HandleObject obj, uint8_t id, bool strict, JS::MutableHandleValue vp)
{
    JSVideo *v = (JSVideo *)JS_GetPrivate(obj);
    if (v == NULL) {
        return false;
    }

    return JSAVSource::PropSetter(v->m_Video, id, vp);
}

void AudioContext_Finalize(JSFreeOp *fop, JSObject *obj)
{
    JSAudio *audio = (JSAudio *)JS_GetPrivate(obj);
    if (audio != NULL) {
        JS_SetPrivate(obj, nullptr);
        delete audio;
    }
}

void AudioNode_Finalize(JSFreeOp *fop, JSObject *obj)
{
    JSAudioNode *node = (JSAudioNode *)JS_GetPrivate(obj);
    if (node != NULL) {
        JS_SetPrivate(obj, nullptr);
        delete node;
    }
}

JSVideo::JSVideo(JS::HandleObject obj,
    Canvas2DContext *canvasCtx, JSContext *cx) :
    JSExposer<JSVideo>(obj, cx),
    m_Video(NULL), m_AudioNode(NULL), m_ArrayContent(NULL),
    m_Width(-1), m_Height(-1), m_Left(0), m_Top(0), m_IsDestructing(false),
    m_CanvasCtx(canvasCtx), m_Cx(cx)
{
    m_Video = new Video((ape_global *)JS_GetContextPrivate(cx));
    m_Video->frameCallback(JSVideo::FrameCallback, this);
    m_Video->eventCallback(JSVideo::onEvent, this);
    m_CanvasCtx->getHandler()->addListener(this);
}

void JSVideo::stopAudio()
{
    m_Video->stopAudio();

    this->releaseAudioNode();
}

void JSVideo::onMessage(const SharedMessages::Message &msg)
{
    if (m_IsDestructing) return;

    if (msg.event() == NIDIUM_EVENT(CanvasHandler, RESIZE_EVENT) && (m_Width == -1 || m_Height == -1)) {
        this->setSize(m_Width, m_Height);
    } else {
        if (msg.event() == SOURCE_EVENT_PLAY) {
            this->setSize(m_Width, m_Height);
        }

        JS::RootedObject obj(m_Cx, this->getJSObject());
        const char *evName = nullptr;
        JS::RootedValue ev(m_Cx);

        evName = JSAVEventRead(msg.event());
        if (!evName) {
            return;
        }

        ev = consumeSourceMessage(m_Cx, obj, msg);

        if (!ev.isNull()) {
            this->fireJSEvent(evName, &ev);
        }
    }
}

void JSVideo::onEvent(const struct AVSourceEvent *cev)
{
    JSVideo *thiz = static_cast<JSVideo *>(cev->m_Custom);
    thiz->postMessage((void *)cev, cev->m_Ev);
}

void JSVideo::FrameCallback(uint8_t *data, void *custom)
{
    JSVideo *v = (JSVideo *)custom;
    CanvasHandler *handler = v->m_CanvasCtx->getHandler();
    SkiaContext *surface = v->m_CanvasCtx->getSurface();
    JSContext *cx = v->m_Cx;

    surface->setFillColor(0xFF000000);
    surface->drawRect(0, 0, handler->getWidth(), handler->getHeight(), 0);
    surface->drawPixels(data, v->m_Video->m_Width, v->m_Video->m_Height, v->m_Left, v->m_Top);

    JS::RootedValue onframe(v->m_Cx);
    JS::RootedObject vobj(v->m_Cx, v->getJSObject());
    JS::RootedObject evObj(cx);

    evObj = JSEvents::CreateEventObject(cx);
    JSObjectBuilder ev(cx, evObj);
    ev.set("video", v->getJSObject());
    JS::RootedValue evjsval(cx, ev.jsval());

    v->fireJSEvent("frame", &evjsval);
}

void JSVideo::setSize(int width, int height)
{
    m_Width = width;
    m_Height = height;

    if (!m_Video->m_CodecCtx) {
        // setSize will be called again when video is ready
        return;
    }

    int canvasWidth = m_CanvasCtx->getHandler()->getWidth();
    int canvasHeight = m_CanvasCtx->getHandler()->getHeight();

    // Invalid dimension, force size to canvas
    if (width == 0) width = -1;
    if (height == 0) height = -1;

    // Size the video
    if (m_Width == -1 || m_Height == -1) {
        int videoWidth = m_Video->m_CodecCtx->width;
        int videoHeight = m_Video->m_CodecCtx->height;

        int maxWidth = nidium_min(m_Width == -1 ? canvasWidth : m_Width, canvasWidth);
        int maxHeight = nidium_min(m_Height == -1 ? canvasHeight : m_Height, canvasHeight);
        double ratio = nidium_max(videoHeight / (double)maxHeight, videoWidth / (double)maxWidth);

        width = videoWidth / ratio;
        height = videoHeight / ratio;
    }

    if (height < canvasHeight) {
        m_Top = (canvasHeight / 2) - (height / 2);
    } else {
        m_Top = 0;
    }

    if (width < canvasWidth) {
        m_Left = (canvasWidth / 2) - (width / 2);
    } else {
        m_Left = 0;
    }

    m_Video->setSize(width, height);
}

static bool nidium_video_play(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    CppObj->m_Video->play();

    return true;
}

static bool nidium_video_pause(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    CppObj->m_Video->pause();

    return true;
}

static bool nidium_video_stop(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    CppObj->m_Video->stop();

    return true;
}

static bool nidium_video_close(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    CppObj->close();

    return true;
}

static bool nidium_video_open(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSVideo *v;
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);
    v = CppObj;

    JS::RootedValue src(cx, args[0]);
    int ret = -1;

    if (src.isString()) {
        JSAutoByteString csrc(cx, src.toString());
        ret = v->m_Video->open(csrc.ptr());
    } else if (src.isObject()) {
        JS::RootedObject arrayBuff(cx, src.toObjectOrNull());

        if (!JS_IsArrayBufferObject(arrayBuff)) {
            JS_ReportError(cx, "Data is not an ArrayBuffer\n");
            return false;
        }

        int length = JS_GetArrayBufferByteLength(arrayBuff);
        v->m_ArrayContent = JS_StealArrayBufferContents(cx, arrayBuff);
        if (v->m_Video->open(v->m_ArrayContent, length) < 0) {
            args.rval().setBoolean(false);
            return true;
        }
    }

    args.rval().setBoolean(false);

    return true;
}

static bool nidium_video_get_audionode(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JSAudio *jaudio = JSAudio::GetContext();
    JSVideo *v;

    NIDIUM_JS_PROLOGUE_CLASS_NO_RET(JSVideo, &Video_class);

    v = CppObj;

    if (!jaudio) {
        JS_ReportError(cx, "No Audio context");
        args.rval().setNull();
        return false;
    }

    if (v->m_AudioNode.get()) {
        JS::RootedObject retObj(cx, v->m_AudioNode);
        args.rval().setObjectOrNull(retObj);
        return true;
    }

    AudioSource *source = v->m_Video->getAudioNode(jaudio->m_Audio);

    if (source != NULL) {
        JS::RootedObject audioNode(cx, JS_NewObjectForConstructor(cx, &AudioNode_class, args));
        v->m_AudioNode = audioNode;

        JSAudioNode *node = new JSAudioNode(audioNode, cx,
          Audio::SOURCE, static_cast<class AudioNode *>(source), jaudio);

        JS::RootedString name(cx, JS_NewStringCopyN(cx, "video-source", 12));
        JS::RootedObject an(cx, v->m_AudioNode);
        jaudio->initNode(node, an, name);

        JS_SetReservedSlot(node->getJSObject(), 0, OBJECT_TO_JSVAL(v->getJSObject()));
        JS::RootedObject retObj(cx, v->m_AudioNode);

        args.rval().setObjectOrNull(retObj);
    } else {
        args.rval().setNull();
    }

    return true;
}

static bool nidium_video_nextframe(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    CppObj->m_Video->nextFrame();

    return true;
}

static bool nidium_video_prevframe(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    CppObj->m_Video->prevFrame();

    return true;
}

static bool nidium_video_frameat(JSContext *cx, unsigned argc, JS::Value *vp)
{
    double time;
    bool keyframe = false;

    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    if (!JS_ConvertArguments(cx, args, "db", &time, keyframe)) {
        return true;
    }

    CppObj->m_Video->frameAt(time, keyframe);

    return true;
}

static bool nidium_video_setsize(JSContext *cx, unsigned argc, JS::Value *vp)
{
    uint32_t width, height;
    NIDIUM_JS_PROLOGUE_CLASS(JSVideo, &Video_class);

    NIDIUM_JS_CHECK_ARGS("setSize", 2)

    JS::RootedValue jwidth(cx, args[0]);
    JS::RootedValue jheight(cx, args[1]);

    if (jwidth.isString()) {
        width = -1;
    } else if (jwidth.isNumber()) {
        JS::ToUint32(cx, jwidth, &width);
    } else {
        JS_ReportError(cx, "Wrong argument type for width");
        return false;
    }

    if (jheight.isString()) {
        height = -1;
    } else if (jheight.isNumber()) {
        JS::ToUint32(cx, jheight, &height);
    } else {
        JS_ReportError(cx, "Wrong argument type for height");
        return false;
    }

    CppObj->setSize(width, height);

    return true;
}

static bool nidium_Video_constructor(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    JS::RootedObject canvas(cx);
    if (!JS_ConvertArguments(cx, args, "o", canvas.address())) {
        return true;
    }

    CanvasHandler *handler = static_cast<class JSCanvas*>(
        JS_GetInstancePrivate(cx, canvas, &Canvas_class, &args))->getHandler();

    if (!handler) {
        JS_ReportError(cx, "Video constructor argument must be Canvas");
        return false;
    }

    CanvasContext *ncc = handler->getContext();
    if (ncc == NULL || ncc->m_Mode != CanvasContext::CONTEXT_2D) {
        JS_ReportError(cx, "Invalid canvas context. Did you called canvas.getContext('2d') ?");
        return false;
    }
    JSContext *m_Cx = cx;
    JS::RootedObject ret(cx, JS_NewObjectForConstructor(cx, &Video_class, args));
    NJS->rootObjectUntilShutdown(ret);
    JS_DefineFunctions(cx, ret, Video_funcs);
    JS_DefineProperties(cx, ret, Video_props);
    JSVideo *v = new JSVideo(ret, (Canvas2DContext*)ncc, cx);
    JS_SetPrivate(ret, v);

    JS_DefineProperty(cx, ret, "canvas", args[0], JSPROP_PERMANENT | JSPROP_READONLY);

    args.rval().setObjectOrNull(ret);

    return true;
}


void JSVideo::releaseAudioNode()
{
    if (m_AudioNode) {
        JSAudioNode *node = JSAudioNode::GetObject(m_AudioNode, m_Cx);

        NJS->unrootObject(m_AudioNode);

        if (node) {
            JS_SetReservedSlot(node->getJSObject(), 0, JSVAL_NULL);
            // will remove the source from JSAudio and Audio
            delete node;
        }

        m_AudioNode = nullptr;
    }
}
void JSVideo::close()
{
    // Stop the audio first.
    // This will also release the JS audio node
    this->stopAudio();

    m_Video->close();
}

JSVideo::~JSVideo()
{
    JSAutoRequest ar(m_Cx);
    m_IsDestructing = true;

    // Release JS AudioNode
    this->stopAudio();

    NJS->unrootObject(this->getJSObject());

    if (m_ArrayContent != NULL) {
        free(m_ArrayContent);
    }

    delete m_Video;
}

static void Video_Finalize(JSFreeOp *fop, JSObject *obj) {
    JSVideo *v = (JSVideo *)JS_GetPrivate(obj);

    if (v != NULL) {
        JS_SetPrivate(obj, nullptr);
        delete v;
    }
}

bool JSAVSource::PropSetter(AVSource *source, uint8_t id, JS::MutableHandleValue vp)
{
    switch(id) {
        case SOURCE_PROP_POSITION:
            if (vp.isNumber()) {
                source->seek(vp.toNumber());
            }
            break;
        default:
            break;
    }

    return true;
}

void CopyMetaDataToJS(AVDictionary *dict, JSContext *cx, JS::HandleObject obj) {
    AVDictionaryEntry *tag = NULL;

    if (!dict) return;

    while ((tag = av_dict_get(dict, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        JS::RootedString val(cx, JS_NewStringCopyN(cx, tag->value, strlen(tag->value)));
        JS::RootedValue value(cx, STRING_TO_JSVAL(val));
        JS_DefineProperty(cx, obj, tag->key, value, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
    }
}
bool JSAVSource::PropGetter(AVSource *source, JSContext *cx, uint8_t id, JS::MutableHandleValue vp)
{
    switch(id) {
        case SOURCE_PROP_POSITION:
            vp.setDouble(source->getClock());
        break;
        case SOURCE_PROP_DURATION:
            vp.setDouble(source->getDuration());
        break;
        case SOURCE_PROP_BITRATE:
            vp.setInt32(source->getBitrate());
        break;
        case SOURCE_PROP_METADATA:
        {
            AVFormatContext *avctx = source->getAVFormatContext();

            if (avctx != NULL) {
                JS::RootedObject metadata(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
                AVDictionary *cmetadata = avctx->metadata;

                if (cmetadata) {
                    CopyMetaDataToJS(cmetadata, cx, metadata);
                }

                JS::RootedObject arr(cx, JS_NewArrayObject(cx, 0));
                JS_DefineProperty(cx, metadata, "streams", arr, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

                for (int i = 0; i < avctx->nb_streams; i++) {
                    JS::RootedObject streamMetaData(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));

                    CopyMetaDataToJS(avctx->streams[i]->metadata, cx, streamMetaData);

                    JS_DefineElement(cx, arr, i, OBJECT_TO_JSVAL(streamMetaData), nullptr, nullptr, 0);
                }

                /*
                for (int i = 0; i < avctx->nb_chapters; i++) {
                    // TODO
                }
                */

                // XXX : Not tested
                /*
                if (avctx->nb_programs) {
                    JS::RootedObject arr(cx, JS_NewArrayObject(cx, 0));
                    JS_DefineProperty(cx, metadata, "programs", arr, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

                    for (int i = 0; i < avctx->nb_programs; i++) {
                        JS::RootedObject progMetaData(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));

                        CopyMetaDataToJS(avctx->programs[i]->metadata, cx, progMetaData);

                        JS_DefineElement(cx, arr, i, OBJECT_TO_JSVAL(progMetaData), nullptr, nullptr, 0);
                    }
                }
                */

                vp.setObject(*metadata);
            } else {
                vp.setUndefined();
            }
        }
        break;
        default:
            vp.setUndefined();
        break;
    }

    return true;
}

bool JSAudioNode::PropSetter(JSAudioNode *jnode, JSContext *cx,
        uint8_t id, JS::MutableHandleValue vp)
{
    AudioSourceCustom *source = static_cast<AudioSourceCustom*>(jnode->m_Node);

    switch(id) {
        case SOURCE_PROP_POSITION:
            if (vp.isNumber()) {
                source->seek(vp.toNumber());
            }
        break;
        default:
        break;
    }

    return true;
}

void JSAudioNode::SeekCallback(AudioSourceCustom *node, double seekTime, void *custom)
{
    JSAudioNode *jnode = static_cast<JSAudioNode*>(custom);
    JSContext *threadCx = jnode->m_Audio->m_JsTcx;

    JSAutoRequest ar(threadCx);
    JSAutoCompartment ac(threadCx, jnode->m_Audio->m_JsGlobalObj);

    JSTransferableFunction *fn = jnode->m_TransferableFuncs[JSAudioNode::SEEK_FN];
    if (!fn) return;

    JS::AutoValueArray<2> params(jnode->m_Audio->m_JsTcx);
    JS::RootedObject global(jnode->m_Audio->m_JsTcx, JS::CurrentGlobalOrNull(jnode->m_Audio->m_JsTcx));
    params[0].setDouble(seekTime);
    params[1].setObjectOrNull(global);

    JS::RootedValue rval(jnode->m_Audio->m_JsTcx);
    fn->call(*jnode->m_NodeObj, params, &rval);
}
// }}}

// {{{ Registration
void JSAudioNode::RegisterObject(JSContext *cx)
{
    JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
    JS::RootedObject obj(cx, JS_InitClass(cx, global, JS::NullPtr(),
            &AudioNode_class, nidium_AudioNode_constructor, 0,
            AudioNode_props, AudioNode_funcs, nullptr, nullptr));
}

void JSAudio::RegisterObject(JSContext *cx)
{
    JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
    JS_InitClass(cx, global, JS::NullPtr(),
        &Audio_class, nidium_Audio_constructor, 0,
        nullptr, nullptr, nullptr, Audio_static_funcs);

    JS_InitClass(cx, global, JS::NullPtr(),
        &AudioContext_class, nidium_Audio_constructor, 0,
        AudioContext_props, AudioContext_funcs, nullptr, nullptr);
}

NIDIUM_JS_OBJECT_EXPOSE(Video);
// }}}

} // namespace Binding
} // namespace Nidium

