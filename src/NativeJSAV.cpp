#include "NativeJSAV.h"
#include "NativeSharedMessages.h"
#include "NativeJSThread.h"
#include "NativeJS.h"
#include <NativeJSConsole.h>

#include "NativeJSCanvas.h"
#include "NativeCanvasHandler.h"
#include "NativeCanvas2DContext.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

// TODO : Need to handle nodes GC, similar to https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html#lifetime-AudioNode
// TODO : Need to handle video GC
// TODO : When stop/pause/kill fade out sound

NativeJSAudio *NativeJSAudio::instance = NULL;
extern JSClass Canvas_class;

#define NJS (NativeJS::getNativeClass(m_Cx))
#define JS_PROPAGATE_ERROR(cx, ...)\
JS_ReportError(cx, __VA_ARGS__);\
if (!JS_ReportPendingException(cx)) {\
    JS_ClearPendingException(cx);\
}

#define JSNATIVE_AV_GET_NODE(type, var)\
    JSNATIVE_PROLOGUE_CLASS(NativeJSAudioNode, &AudioNode_class);\
    var = static_cast<type *>(CppObj->node);

#define CHECK_INVALID_CTX(obj) if (!obj) {\
JS_ReportError(cx, "Invalid NativeAudio context");\
return false;\
}
void FIXMEReportError(JSContext *cx, const char *message, JSErrorReport *report)
{
    printf("%s\n", message);
}

extern void reportError(JSContext *cx, const char *message, JSErrorReport *report);

static void AudioNode_Finalize(JSFreeOp *fop, JSObject *obj);
static void AudioContext_Finalize(JSFreeOp *fop, JSObject *obj);

static JSBool native_Audio_constructor(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audio_getcontext(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audio_run(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audio_pFFT(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audio_load(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audio_createnode(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audio_connect(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audio_disconnect(JSContext *cx, unsigned argc, jsval *vp);

static JSBool native_audiothread_print(JSContext *cx, unsigned argc, jsval *vp);

static JSBool native_AudioNode_constructor(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_set(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_get(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_custom_set(JSContext *cx, unsigned argc, jsval *vp);
//static JSBool native_audionode_custom_get(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_custom_threaded_set(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_custom_threaded_get(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_custom_threaded_send(JSContext *cx, unsigned argc, jsval *vp);

static JSBool native_audionode_source_open(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_source_play(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_source_pause(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_source_stop(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_source_close(JSContext *cx, unsigned argc, jsval *vp);

static JSBool native_audionode_custom_source_play(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_custom_source_pause(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_audionode_custom_source_stop(JSContext *cx, unsigned argc, jsval *vp);

static JSBool native_audio_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);
static JSBool native_audio_prop_getter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp);
static JSBool native_audionode_custom_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);
static JSBool native_audionode_source_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);
static JSBool native_audionode_source_prop_getter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp);
static JSBool native_audionode_custom_source_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);

static JSClass messageEvent_class = {
    "ThreadMessageEvent", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass Audio_class = {
    "Audio", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass AudioContext_class = {
    "AudioContext", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, AudioContext_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

template<>
JSClass *NativeJSExposer<NativeJSAudio>::jsclass = &AudioContext_class;


static JSClass global_AudioThread_class = {
    "_GLOBALAudioThread", JSCLASS_GLOBAL_FLAGS | JSCLASS_IS_GLOBAL,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass AudioNodeLink_class = {
    "AudioNodeLink", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass AudioNodeEvent_class = {
    "AudioNodeEvent", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass AudioNode_class = {
    "AudioNode", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, AudioNode_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

template<>
JSClass *NativeJSExposer<NativeJSAudioNode>::jsclass = &AudioNode_class;

static JSClass AudioNode_threaded_class = {
    "AudioNodeThreaded", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSPropertySpec AudioContext_props[] = {
    {"volume", AUDIO_PROP_VOLUME, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT, 
        JSOP_WRAPPER(native_audio_prop_getter), JSOP_WRAPPER(native_audio_prop_setter)},
    {"bufferSize", AUDIO_PROP_BUFFERSIZE, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audio_prop_getter), JSOP_NULLWRAPPER},
    {"channels", AUDIO_PROP_CHANNELS, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audio_prop_getter), JSOP_NULLWRAPPER},
    {"sampleRate", AUDIO_PROP_SAMPLERATE, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audio_prop_getter), JSOP_NULLWRAPPER},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static JSPropertySpec AudioNode_props[] = {
    /* type, input, ouput readonly props are created in createnode function */
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static JSPropertySpec AudioNodeEvent_props[] = {
    {"data", NODE_EV_PROP_DATA, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER},
    {"size", NODE_EV_PROP_SIZE, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static JSPropertySpec AudioNodeCustom_props[] = {
    {"process", NODE_CUSTOM_PROP_PROCESS, 0, JSOP_NULLWRAPPER, JSOP_WRAPPER(native_audionode_custom_prop_setter)},
    {"init", NODE_CUSTOM_PROP_INIT, 0, JSOP_NULLWRAPPER, JSOP_WRAPPER(native_audionode_custom_prop_setter)},
    {"setter", NODE_CUSTOM_PROP_SETTER, 0, JSOP_NULLWRAPPER, JSOP_WRAPPER(native_audionode_custom_prop_setter)},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static JSFunctionSpec AudioContext_funcs[] = {
    JS_FN("run", native_audio_run, 1, 0),
    JS_FN("load", native_audio_load, 1, 0),
    JS_FN("createNode", native_audio_createnode, 3, 0),
    JS_FN("connect", native_audio_connect, 2, 0),
    JS_FN("disconnect", native_audio_disconnect, 2, 0),
    JS_FN("pFFT", native_audio_pFFT, 2, 0),
    JS_FS_END
};

static JSFunctionSpec Audio_static_funcs[] = {
    JS_FN("getContext", native_audio_getcontext, 3, 0),
    JS_FS_END
};

static JSFunctionSpec AudioNode_funcs[] = {
    JS_FN("set", native_audionode_set, 2, 0),
    JS_FN("get", native_audionode_get, 1, 0),
    JS_FS_END
};

static JSFunctionSpec AudioNodeCustom_funcs[] = {
    JS_FN("set", native_audionode_custom_set, 2, 0),
    //JS_FN("get", native_audionode_custom_get, 1, 0),
    JS_FS_END
};

static JSFunctionSpec AudioNodeCustom_threaded_funcs[] = {
    JS_FN("set", native_audionode_custom_threaded_set, 2, 0),
    JS_FN("get", native_audionode_custom_threaded_get, 1, 0),
    JS_FN("send", native_audionode_custom_threaded_send, 2, 0),
    JS_FS_END
};

static JSFunctionSpec AudioNodeSource_funcs[] = {
    JS_FN("open", native_audionode_source_open, 1, 0),
    JS_FN("play", native_audionode_source_play, 0, 0),
    JS_FN("pause", native_audionode_source_pause, 0, 0),
    JS_FN("stop", native_audionode_source_stop, 0, 0),
    JS_FN("close", native_audionode_source_close, 0, 0),
    JS_FS_END
};

static JSFunctionSpec AudioNodeCustomSource_funcs[] = {
    JS_FN("play", native_audionode_custom_source_play, 0, 0),
    JS_FN("pause", native_audionode_custom_source_pause, 0, 0),
    JS_FN("stop", native_audionode_custom_source_stop, 0, 0),
    JS_FS_END
};

static JSPropertySpec AudioNodeSource_props[] = {
    {"position", SOURCE_PROP_POSITION, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT, 
        JSOP_WRAPPER(native_audionode_source_prop_getter), 
        JSOP_WRAPPER(native_audionode_source_prop_setter)},
    {"duration", SOURCE_PROP_DURATION, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audionode_source_prop_getter), 
        JSOP_NULLWRAPPER},
    {"metadata", SOURCE_PROP_METADATA, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audionode_source_prop_getter), 
        JSOP_NULLWRAPPER},
     {"bitrate", SOURCE_PROP_BITRATE, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audionode_source_prop_getter), 
        JSOP_NULLWRAPPER},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static JSPropertySpec AudioNodeCustomSource_props[] = {
    {"seek", CUSTOM_SOURCE_PROP_SEEK, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT, 
        JSOP_NULLWRAPPER, 
        JSOP_WRAPPER(native_audionode_custom_source_prop_setter)},
#if 0
    {"duration", SOURCE_PROP_DURATION, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audionode_source_prop_getter), 
        JSOP_NULLWRAPPER},
    {"metadata", SOURCE_PROP_METADATA, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audionode_source_prop_getter), 
        JSOP_NULLWRAPPER},
     {"bitrate", SOURCE_PROP_BITRATE, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_audionode_source_prop_getter), 
        JSOP_NULLWRAPPER},
#endif
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static JSFunctionSpec glob_funcs_threaded[] = {
    JS_FN("echo", native_audiothread_print, 1, 0),
    JS_FS_END
};


static JSBool native_Video_constructor(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_play(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_pause(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_stop(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_close(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_open(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_get_audionode(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_nextframe(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_prevframe(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_frameat(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_video_setsize(JSContext *cx, unsigned argc, jsval *vp);

static JSBool native_video_prop_getter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp);
static JSBool native_video_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);

static void Video_Finalize(JSFreeOp *fop, JSObject *obj);

static JSFunctionSpec Video_funcs[] = {
    JS_FN("play", native_video_play, 0, 0),
    JS_FN("pause", native_video_pause, 0, 0),
    JS_FN("stop", native_video_stop, 0, 0),
    JS_FN("close", native_video_close, 0, 0),
    JS_FN("open", native_video_open, 1, 0),
    JS_FN("getAudioNode", native_video_get_audionode, 0, 0),
    JS_FN("nextFrame", native_video_nextframe, 0, 0),
    JS_FN("prevFrame", native_video_prevframe, 0, 0),
    JS_FN("frameAt", native_video_frameat, 1, 0),
    JS_FN("setSize", native_video_setsize, 2, 0),
    JS_FS_END
};

static JSClass Video_class = {
    "Video", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Video_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

template<>
JSClass *NativeJSExposer<NativeJSVideo>::jsclass = &Video_class;

static JSPropertySpec Video_props[] = {
    {"width", VIDEO_PROP_WIDTH, 
        JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT, 
        JSOP_WRAPPER(native_video_prop_getter), 
        JSOP_NULLWRAPPER},
    {"height", VIDEO_PROP_HEIGHT, 
        JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT, 
        JSOP_WRAPPER(native_video_prop_getter), 
        JSOP_NULLWRAPPER},
    {"position", SOURCE_PROP_POSITION, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT, 
        JSOP_WRAPPER(native_video_prop_getter), 
        JSOP_WRAPPER(native_video_prop_setter)},
    {"duration", SOURCE_PROP_DURATION, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_video_prop_getter), 
        JSOP_NULLWRAPPER},
    {"metadata", SOURCE_PROP_METADATA, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_video_prop_getter), 
        JSOP_NULLWRAPPER},
     {"bitrate", SOURCE_PROP_BITRATE, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT|JSPROP_READONLY, 
        JSOP_WRAPPER(native_video_prop_getter), 
        JSOP_NULLWRAPPER},
    {"onframe", VIDEO_PROP_ONFRAME, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT, 
        JSOP_NULLWRAPPER, 
        JSOP_NULLWRAPPER},
    {"canvas", VIDEO_PROP_CANVAS, 
        JSPROP_ENUMERATE|JSPROP_PERMANENT, 
        JSOP_NULLWRAPPER, 
        JSOP_NULLWRAPPER},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

int FFT(int dir,int nn,double *x,double *y)
{
   long m,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

   m = log2(nn);

   /* Do the bit reversal */
   i2 = nn >> 1;
   j = 0;
   for (i=0;i<nn-1;i++) {
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
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<nn;i+=l2) {
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
      for (i=0;i<nn;i++) {
         x[i] /= (double)nn;
         y[i] /= (double)nn;
      }
   }

   return true;
}

const char *NativeJSAVEventRead(int ev)
{
    switch (ev) {
        case SOURCE_EVENT_PAUSE:
            return "onpause";
        break;
        case SOURCE_EVENT_PLAY:
            return "onplay";
        break;
        case SOURCE_EVENT_STOP:
            return "onstop";
        break;
        case SOURCE_EVENT_EOF:
            return "onend";
        break;
        case SOURCE_EVENT_ERROR:
            return "onerror";
        break;
        case SOURCE_EVENT_BUFFERING:
            return "onbuffering";
        break;
        case SOURCE_EVENT_READY:
            return "onready";
        break;
        default:
            return NULL;
        break;
    }
}

void native_av_thread_message(JSContext *cx, JSObject *obj, const NativeSharedMessages::Message &msg)
{

    jsval jscbk, rval;
    if (msg.event() == CUSTOM_SOURCE_SEND) {
        native_thread_msg *ptr = static_cast<struct native_thread_msg *>(msg.dataPtr());

        if (JS_GetProperty(cx, obj, "onmessage", &jscbk) &&
            !JSVAL_IS_PRIMITIVE(jscbk) && 
            JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(jscbk))) {

            jsval inval = JSVAL_NULL;

            if (!JS_ReadStructuredClone(cx, ptr->data, ptr->nbytes,
                JS_STRUCTURED_CLONE_VERSION, &inval, NULL, NULL)) {
                JS_PROPAGATE_ERROR(cx, "Failed to transfert custom node message to audio thread");
                return;
            }

            JSObject *event = JS_NewObject(cx, &messageEvent_class, NULL, NULL);

            JS_DefineProperty(cx, event, "data", inval, NULL, NULL, 
                    JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE);

            jsval jsvalEvent= OBJECT_TO_JSVAL(event);

            JS_CallFunctionValue(cx, event, jscbk, 1, &jsvalEvent, &rval);          

        }
        delete ptr;
    } else {
        NativeAVSourceEvent *cmsg = static_cast<struct NativeAVSourceEvent*>(msg.dataPtr());

        const char *prop = NativeJSAVEventRead(msg.event());
        if (!prop) {
            // delete cmsg; // XXX : Unknown message. Don't delete it.
            return;
        }

        if (JS_GetProperty(cx, obj, prop, &jscbk) &&
            !JSVAL_IS_PRIMITIVE(jscbk) &&
            JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(jscbk))) {

            if (cmsg->ev == SOURCE_EVENT_ERROR) {
                jsval event[2];
                int errorCode = cmsg->args[0].toInt();
                const char *errorStr = NativeAVErrorsStr[errorCode];

                event[0] = INT_TO_JSVAL(errorCode);
                event[1] = STRING_TO_JSVAL(JS_NewStringCopyN(cx, errorStr, strlen(errorStr)));

                JS_CallFunctionValue(cx, obj, jscbk, 2, event, &rval);
            } else if (cmsg->ev == SOURCE_EVENT_BUFFERING) {
                jsval event[2];

                event[0] = INT_TO_JSVAL(cmsg->args[0].toInt());
                event[1] = INT_TO_JSVAL(cmsg->args[1].toInt());
                event[2] = INT_TO_JSVAL(cmsg->args[2].toInt());

                JS_CallFunctionValue(cx, obj, jscbk, 3, event, &rval);
            } else {
                JS_CallFunctionValue(cx, obj, jscbk, 0, NULL, &rval);
            }
            
        }

        delete cmsg;
    }
}

bool JSTransferableFunction::prepare(JSContext *cx, jsval val)
{
    if (!JS_WriteStructuredClone(cx, val, &m_Data, &m_Bytes, NULL, NULL, JSVAL_VOID)) {
        return false;
    }

    return true;
}

bool JSTransferableFunction::call(JSContext *cx, JSObject *obj, int argc, jsval *params, jsval *rval)
{
    if (m_Fn == NULL) {
        if (m_Data == NULL) return false;

        m_Fn = new JS::Value();
        JS_AddValueRoot(cx, m_Fn);
        m_Fn->setNull();

        if (!this->transfert(cx)) {
            JS_RemoveValueRoot(cx, m_Fn);
            delete m_Fn;
            m_Fn = NULL;
            return false;
        } else {
            // Function is transfered
        }
    }

    return JS_CallFunctionValue(cx, obj, *m_Fn, argc, params, rval);
}

bool JSTransferableFunction::transfert(JSContext *destCx)
{
    if (m_DestCx != NULL) return false;

    m_DestCx = destCx;

    bool ok = JS_ReadStructuredClone(destCx, m_Data, m_Bytes, 
                JS_STRUCTURED_CLONE_VERSION, m_Fn, NULL, NULL);

    JS_ClearStructuredClone(m_Data, m_Bytes);

    m_Data = NULL;
    m_Bytes = 0;

    return ok;
}

JSTransferableFunction::~JSTransferableFunction()
{
    if (m_Data != NULL) {
        JS_ClearStructuredClone(m_Data, m_Bytes);
    }

    if (m_Fn != NULL) {
        JS_RemoveValueRoot(m_DestCx, m_Fn);
        delete m_Fn;
    }
}

NativeJSAudio *NativeJSAudio::getContext(JSContext *cx, JSObject *obj, int bufferSize, int channels, int sampleRate) 
{
    ape_global *net = static_cast<ape_global *>(JS_GetContextPrivate(cx));
    NativeAudio *audio;

    try {
        audio = new NativeAudio(net, bufferSize, channels, sampleRate);
    } catch (...) {
        return NULL;
    }

    audio->setMainCtx(cx);

    return new NativeJSAudio(audio, cx, obj);
}

void NativeJSAudio::initNode(NativeJSAudioNode *node, JSObject *jnode, JSString *name)
{
    int in = node->node->inCount;
    int out = node->node->outCount;
    JS::Value inputLinks, outputLinks;

    JS_DefineProperty(m_Cx, jnode, "type", STRING_TO_JSVAL(name), NULL, NULL,
        JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);

    inputLinks.setObjectOrNull(JS_NewArrayObject(m_Cx, in, NULL));
    outputLinks.setObjectOrNull(JS_NewArrayObject(m_Cx, out, NULL));

    for (int i = 0; i < in; i++) {
        JSObject *link = JS_NewObject(m_Cx, &AudioNodeLink_class, NULL, NULL);
        JS_SetPrivate(link, node->node->input[i]);

        JS_DefineElement(m_Cx, inputLinks.toObjectOrNull(), i, OBJECT_TO_JSVAL(link), NULL, NULL, 0);
    }

    for (int i = 0; i < out; i++) {
        JSObject *link = JS_NewObject(m_Cx, &AudioNodeLink_class, NULL, NULL);
        JS_SetPrivate(link, node->node->output[i]);

        JS_DefineElement(m_Cx, outputLinks.toObjectOrNull(), i, OBJECT_TO_JSVAL(link), NULL, NULL, 0);
    }

    if (in > 0) {
        JS_DefineProperty(m_Cx, jnode, "input", inputLinks, NULL, NULL,
            JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
    }

    if (out > 0) {
        JS_DefineProperty(m_Cx, jnode, "output", outputLinks, NULL, NULL,
            JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
    }

    node->njs = NJS;

    node->setJSObject(jnode);
    node->setJSContext(m_Cx);

    NJS->rootObjectUntilShutdown(node->getJSObject());
    JS_SetPrivate(jnode, node);
}

NativeJSAudio *NativeJSAudio::getContext()
{
    return NativeJSAudio::instance;
}

NativeJSAudio::NativeJSAudio(NativeAudio *audio, JSContext *cx, JSObject *obj)
    :
      NativeJSExposer<NativeJSAudio>(obj, cx),
      audio(audio), nodes(NULL), gbl(NULL), rt(NULL), tcx(NULL),
      target(NULL)
{

    NativeJSAudio::instance = this;

    JS_SetPrivate(obj, this);

    NJS->rootObjectUntilShutdown(obj);

    JS_DefineFunctions(cx, obj, AudioContext_funcs);
    JS_DefineProperties(cx, obj, AudioContext_props);

    NATIVE_PTHREAD_VAR_INIT(&this->m_ShutdownWait)

    this->audio->sharedMsg->postMessage(
            (void *)new NativeAudioNode::CallbackMessage(NativeJSAudio::ctxCallback, NULL, static_cast<void *>(this)), 
            NATIVE_AUDIO_NODE_CALLBACK);
}

bool NativeJSAudio::createContext() 
{
    if (this->rt == NULL) {
        if ((this->rt = JS_NewRuntime(128L * 1024L * 1024L, JS_USE_HELPER_THREADS)) == NULL) {
            printf("Failed to init JS runtime\n");
            return false;
        }
        //JS_SetRuntimePrivate(rt, this->cx);

        JS_SetGCParameter(this->rt, JSGC_MAX_BYTES, 0xffffffff);
        JS_SetGCParameter(this->rt, JSGC_MODE, JSGC_MODE_INCREMENTAL);
        JS_SetGCParameter(this->rt, JSGC_SLICE_TIME_BUDGET, 15);

        if ((this->tcx = JS_NewContext(this->rt, 8192)) == NULL) {
            printf("Failed to init JS context\n");
            return false;     
        }

        JS_SetStructuredCloneCallbacks(this->rt, NativeJS::jsscc);

        JSAutoRequest ar(this->tcx);

        //JS_SetGCParameterForThread(this->tcx, JSGC_MAX_CODE_CACHE_BYTES, 16 * 1024 * 1024);

        this->gbl = JS_NewGlobalObject(this->tcx, &global_AudioThread_class, NULL);
        JS_AddObjectRoot(this->tcx, &this->gbl);
        if (!JS_InitStandardClasses(tcx, this->gbl)) {
            printf("Failed to init std class\n");
            return false;
        }

        JS_SetVersion(this->tcx, JSVERSION_LATEST);

        JS_SetOptions(this->tcx, JSOPTION_VAROBJFIX | JSOPTION_METHODJIT | JSOPTION_TYPE_INFERENCE | JSOPTION_ION);

        JS_SetErrorReporter(this->tcx, reportError);
        JS_SetGlobalObject(this->tcx, this->gbl);
        JS_DefineFunctions(this->tcx, this->gbl, glob_funcs_threaded);
        NativeJSconsole::registerObject(this->tcx);

        JS_SetRuntimePrivate(this->rt, NativeJS::getNativeClass(this->audio->getMainCtx()));

        //JS_SetContextPrivate(this->tcx, static_cast<void *>(this));
    }

    return true;
    
}

bool NativeJSAudio::run(char *str)
{
    jsval rval;
    JSFunction *fun;

    if (!this->tcx) {
        printf("No JS context for audio thread\n");
        return false;
    }

    JSAutoRequest ar(this->tcx);

    fun = JS_CompileFunction(this->tcx, JS_GetGlobalObject(this->tcx), "Audio_run", 0, NULL, str, strlen(str), "(Audio Thread)", 0);

    if (!fun) {
        JS_ReportError(this->tcx, "Failed to execute script on audio thread\n");
        return false;
    }

    JS_CallFunction(this->tcx, JS_GetGlobalObject(this->tcx), fun, 0, NULL, &rval);

    return true;
}

void NativeJSAudio::unroot() 
{
    NativeJSAudio::Nodes *nodes = this->nodes;

    NativeJS *njs = NativeJS::getNativeClass(m_Cx);

    if (m_JSObject != NULL) {
        JS_RemoveObjectRoot(njs->cx, &m_JSObject);
    }
    if (this->gbl != NULL) {
        JS_RemoveObjectRoot(njs->cx, &this->gbl);
    }

    while (nodes != NULL) {
        JS_RemoveObjectRoot(njs->cx, nodes->curr->getJSObjectAddr());
        nodes = nodes->next;
    }
}

void NativeJSAudio::shutdownCallback(NativeAudioNode *dummy, void *custom)
{
    NativeJSAudio *audio = static_cast<NativeJSAudio *>(custom);
    NativeJSAudio::Nodes *nodes = audio->nodes;

    // Let's shutdown all custom nodes
    while (nodes != NULL) {
        if (nodes->curr->type == NativeAudio::CUSTOM ||
            nodes->curr->type == NativeAudio::CUSTOM_SOURCE) {
            nodes->curr->shutdownCallback(nodes->curr->node, nodes->curr);
        }

        nodes = nodes->next;
    }

    if (audio->tcx != NULL) {
        JS_BeginRequest(audio->tcx);

        JSRuntime *rt = JS_GetRuntime(audio->tcx);
        JS_RemoveObjectRoot(audio->tcx, &audio->gbl);

        JS_EndRequest(audio->tcx);

        JS_DestroyContext(audio->tcx);
        JS_DestroyRuntime(rt);
        audio->tcx = NULL;
    }

    NATIVE_PTHREAD_SIGNAL(&audio->m_ShutdownWait)
}

NativeJSAudio::~NativeJSAudio() 
{
    this->audio->lockSources();
    this->audio->lockQueue();

    // Unroot all js audio nodes
    this->unroot();

    // Delete all nodes 
    NativeJSAudio::Nodes *nodes = this->nodes;
    NativeJSAudio::Nodes *next = NULL;
    while (nodes != NULL) {
        next = nodes->next;
        // Node destructor will remove the node 
        // from the nodes linked list
        delete nodes->curr;
        nodes = next;
    }

    // Unroot custom nodes objets and clear threaded js context
    this->audio->sharedMsg->postMessage(
            (void *)new NativeAudioNode::CallbackMessage(NativeJSAudio::shutdownCallback, NULL, this),
            NATIVE_AUDIO_CALLBACK);

    this->audio->wakeup();

    NATIVE_PTHREAD_WAIT(&this->m_ShutdownWait)

    // Unlock the sources, so the decode thread can exit
    // when we call NativeAudio::shutdown()
    this->audio->unlockSources();

    // Shutdown the audio
    this->audio->shutdown();

    this->audio->unlockQueue();

    // And delete the audio
    delete this->audio;

    NativeJSAudio::instance = NULL;
}

void NativeJSAudioNode::add() 
{
    NativeJSAudio::Nodes *nodes = new NativeJSAudio::Nodes(this, NULL, this->audio->nodes);

    if (this->audio->nodes != NULL) {
        this->audio->nodes->prev = nodes;
    }

    this->audio->nodes = nodes;
}

void native_audio_node_custom_set_internal(JSContext *cx, NativeJSAudioNode *node, JSObject *obj, const char *name, JS::Value *val)
{
    JS_SetProperty(cx, obj, name, val);

    JSTransferableFunction *setterFn;
    setterFn = node->m_TransferableFuncs[NativeJSAudioNode::SETTER_FN];
    if (setterFn) {
        jsval params[3];
        jsval rval;
        params[0] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, name));
        params[1] = *val;
        params[2] = OBJECT_TO_JSVAL(JS_GetGlobalObject(cx));

        setterFn->call(cx, node->nodeObj, 3, params, &rval);
    }
}

void NativeJSAudioNode::setPropCallback(NativeAudioNode *node, void *custom)
{
    NativeJSAudioNode::Message *msg;
    jsval data;
    JSContext *tcx;

    msg = static_cast<struct NativeJSAudioNode::Message*>(custom);
    tcx = msg->jsNode->audio->tcx;

    JSAutoRequest ar(tcx);

    if (!JS_ReadStructuredClone(tcx,
                msg->clone.datap,
                msg->clone.nbytes,
                JS_STRUCTURED_CLONE_VERSION, &data, NULL, NULL)) {
        JS_PROPAGATE_ERROR(tcx, "Failed to read structured clone");

        JS_free(msg->jsNode->getJSContext(), msg->name);
        JS_ClearStructuredClone(msg->clone.datap, msg->clone.nbytes);
        delete msg;

        return;
    }

    if (msg->name == NULL) {
        JSObject *props = JSVAL_TO_OBJECT(data);
        js::AutoIdArray ida(tcx, JS_Enumerate(tcx, props));
        for (size_t i = 0; i < ida.length(); i++) {
            js::Rooted<jsid> id(tcx, ida[i]);
            JS::Value val;
            JSAutoByteString name(tcx, JSID_TO_STRING(id));

            if (!JS_GetPropertyById(tcx, props, id, &val)) {
                break;
            }
            native_audio_node_custom_set_internal(tcx, msg->jsNode, msg->jsNode->hashObj, name.ptr(), &val);
        }
    } else {
        native_audio_node_custom_set_internal(tcx, msg->jsNode, msg->jsNode->hashObj, msg->name, &data);
    }

    if (msg->name != NULL) {
        JS_free(msg->jsNode->getJSContext(), msg->name);
    }

    JS_ClearStructuredClone(msg->clone.datap, msg->clone.nbytes);

    delete msg;
}

void NativeJSAudioNode::customCallback(const struct NodeEvent *ev)
{
    NativeJSAudioNode *thiz;
    JSContext *tcx;
    JSTransferableFunction *processFn;

    thiz = static_cast<NativeJSAudioNode *>(ev->custom);

    if (!thiz->audio->tcx || !thiz->m_Cx || !thiz->m_JSObject || !thiz->node) {
        return;
    }

    tcx = thiz->audio->tcx;

    JSAutoRequest ar(tcx);

    processFn = thiz->m_TransferableFuncs[NativeJSAudioNode::PROCESS_FN];

    if (!processFn) return;

    jsval rval, params[2], vFrames, vSize;
    JSObject *obj, *frames;
    unsigned long size;
    int count;

    count = thiz->node->inCount > thiz->node->outCount ? thiz->node->inCount : thiz->node->outCount;
    size = thiz->node->audio->outputParameters->bufferSize/2;

    obj = JS_NewObject(tcx, &AudioNodeEvent_class, NULL, NULL);
    JS_DefineProperties(tcx, obj, AudioNodeEvent_props);

    frames = JS_NewArrayObject(tcx, 0, NULL);

    for (int i = 0; i < count; i++) 
    {
        JSObject *arrBuff, *arr;
        uint8_t *data;

        // TODO : Avoid memcpy (custom allocator for NativeAudioNode?)
        arrBuff = JS_NewArrayBuffer(tcx, size);
        data = JS_GetArrayBufferData(arrBuff);

        memcpy(data, ev->data[i], size);

        arr = JS_NewFloat32ArrayWithBuffer(tcx, arrBuff, 0, -1);
        if (arr != NULL) {
            JS_DefineElement(tcx, frames, i, OBJECT_TO_JSVAL(arr), 
                NULL, NULL, JSPROP_ENUMERATE | JSPROP_PERMANENT);
        } else {
            JS_ReportOutOfMemory(tcx);
            return;
        }
    }

    vFrames = OBJECT_TO_JSVAL(frames);
    vSize = DOUBLE_TO_JSVAL(ev->size);

    JS_SetProperty(tcx, obj, "data", &vFrames);
    JS_SetProperty(tcx, obj, "size", &vSize);

    params[0] = OBJECT_TO_JSVAL(obj);
    params[1] = OBJECT_TO_JSVAL(JS_GetGlobalObject(tcx));

    //JS_CallFunction(tcx, thiz->nodeObj, thiz->processFn, 2, params, &rval);
    processFn->call(tcx, thiz->nodeObj, 2, params, &rval);

    for (int i = 0; i < count; i++) 
    {
        JS::Value val;
        JSObject *arr;

        JS_GetElement(tcx, frames, i, &val);
        if (!val.isObject()) {
            continue;
        }

        arr = val.toObjectOrNull();
        if (arr == NULL || !JS_IsFloat32Array(arr) || JS_GetTypedArrayLength(arr) != ev->size) {
            continue;
        }

        memcpy(ev->data[i], JS_GetFloat32ArrayData(arr), size);
    }
}

void NativeJSAudioNode::onMessage(const NativeSharedMessages::Message &msg)
{
    if (m_IsDestructing) return;

    native_av_thread_message(m_Cx, m_JSObject, msg);
}

void NativeJSAudioNode::onEvent(const struct NativeAVSourceEvent *cev) 
{
    NativeJSAudioNode *jnode = static_cast<NativeJSAudioNode *>(cev->custom);
    jnode->postMessage((void *)cev, cev->ev);
}

void NativeJSAudio::runCallback(NativeAudioNode *node, void *custom)
{
    NativeJSAudio *audio = NativeJSAudio::getContext();

    if (!audio) return;// This should not happend

    char *str = static_cast<char *>(custom);
    audio->run(str);
    JS_free(audio->getJSContext(), custom);
}


void NativeJSAudio::ctxCallback(NativeAudioNode *dummy, void *custom)
{
    NativeJSAudio *audio = static_cast<NativeJSAudio*>(custom);

    if (!audio->createContext()) {
        printf("Failed to create audio thread context\n");
        //JS_ReportError(jsNode->audio->cx, "Failed to create audio thread context\n");
        // XXX : Can't report error from another thread?
    }
}



void NativeJSAudioNode::shutdownCallback(NativeAudioNode *nnode, void *custom)
{
    NativeJSAudioNode *node = static_cast<NativeJSAudioNode *>(custom);
    JSAutoRequest ar(node->audio->tcx);

    if (node->nodeObj != NULL) {
        JS_RemoveObjectRoot(node->audio->tcx, &node->nodeObj);
    }
    if (node->hashObj != NULL) {
        JS_RemoveObjectRoot(node->audio->tcx, &node->hashObj);
    }

    // JSTransferableFunction need to be destroyed on the JS thread
    // since they belong to it
    for (int i = 0; i < NativeJSAudioNode::END_FN; i++) {
        delete node->m_TransferableFuncs[i];
        node->m_TransferableFuncs[i] = NULL;
    }

    NATIVE_PTHREAD_SIGNAL(&node->m_ShutdownWait)
}

void NativeJSAudioNode::initCustomObject(NativeAudioNode *node, void *custom)
{
    NativeJSAudioNode *jnode = static_cast<NativeJSAudioNode *>(custom);
    JSContext *tcx = jnode->audio->tcx;

    if (jnode->hashObj != NULL || jnode->nodeObj != NULL) {
        return;
    }

    JSAutoRequest ar(tcx);

    jnode->hashObj = JS_NewObject(tcx, NULL, NULL, NULL);
    if (!jnode->hashObj) {
        JS_PROPAGATE_ERROR(tcx, "Failed to create hash object for custom node");
        return;
    }
    JS_AddObjectRoot(tcx, &jnode->hashObj);

    jnode->nodeObj = JS_NewObject(tcx, &AudioNode_threaded_class, NULL, NULL);
    if (!jnode->nodeObj) {
        JS_PROPAGATE_ERROR(tcx, "Failed to create node object for custom node");
        return;
    }
    JS_AddObjectRoot(tcx, &jnode->nodeObj);

    JS_DefineFunctions(tcx, jnode->nodeObj, AudioNodeCustom_threaded_funcs);

    JS_SetPrivate(jnode->nodeObj, static_cast<void *>(jnode));

    JSTransferableFunction *initFn =
        jnode->m_TransferableFuncs[NativeJSAudioNode::INIT_FN];

    if (initFn) {
        jsval params[1];
        jsval rval;

        params[0] = OBJECT_TO_JSVAL(JS_GetGlobalObject(tcx));

        initFn->call(tcx, jnode->nodeObj, 1, params, &rval);

        jnode->m_TransferableFuncs[NativeJSAudioNode::INIT_FN] = NULL;

        delete initFn;
    }
}


NativeJSAudioNode::~NativeJSAudioNode()
{
    NativeJSAudio::Nodes *nodes = this->audio->nodes;
    m_IsDestructing = true;

    // Block NativeAudio threads execution.
    // While the node is destructed we don't want any thread
    // to call some method on a node that is being destroyed
    this->audio->audio->lockQueue();
    this->audio->audio->lockSources();

    // Wakeup audio thread. This will flush all pending messages.
    // That way, we are sure nothing will need to be processed
    // later for this node.
    this->audio->audio->wakeup();

    if (this->type == NativeAudio::SOURCE) {
        // Only source from NativeVideo has reserved slot
        JS::Value s = JS_GetReservedSlot(m_JSObject, 0);
        JSObject *obj = s.toObjectOrNull();
        if (obj != NULL) {
            // If it exist, we must inform the video 
            // that audio node no longer exist
            NativeJSVideo *video = (NativeJSVideo *)JS_GetPrivate(obj);
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
                this->audio->nodes = nodes->next;
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
    if (this->node != NULL && this->audio->tcx != NULL && 
            (this->type == NativeAudio::CUSTOM ||
             this->type == NativeAudio::CUSTOM_SOURCE)) {

        this->node->callback(NativeJSAudioNode::shutdownCallback, this);

        this->audio->audio->wakeup();

        NATIVE_PTHREAD_WAIT(&this->m_ShutdownWait);
    }

    if (this->arrayContent != NULL) {
        free(this->arrayContent);
    }

    if (m_JSObject != NULL) {
        JS_SetPrivate(m_JSObject, NULL);
    }

    delete node;

    this->audio->audio->unlockQueue();
    this->audio->audio->unlockSources();
}


static JSBool native_audio_pFFT(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JSObject *x, *y;
    int dir, n;

//int FFT(int dir,int m,double *x,double *y)

    if (!JS_ConvertArguments(cx, args.length(), args.array(),
        "ooii", &x, &y, &n, &dir)) {
        return false;
    }

    if (!JS_IsTypedArrayObject(x) || !JS_IsTypedArrayObject(y)) {
        JS_ReportError(cx, "Bad argument");
        return false;
    }

    double *dx, *dy;
    uint32_t dlenx, dleny;

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

static JSBool native_Audio_constructor(JSContext *cx, unsigned argc, jsval *vp)
{
    JS_ReportError(cx, "Illegal constructor");
    return false;
}

static JSBool native_audio_getcontext(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    unsigned int bufferSize, channels, sampleRate;

    if (argc > 0) {
        bufferSize = JSVAL_TO_INT(args.array()[0]);
    } else {
        bufferSize = 2048;
    }

    if (argc > 1) {
        channels = JSVAL_TO_INT(args.array()[1]);
    } else {
        channels = 2;
    }

    if (argc >= 2) {
        sampleRate = JSVAL_TO_INT(args.array()[2]);
    } else {
        sampleRate = 44100;
    }

    switch (bufferSize) {
        case 16:
        case 32:
        case 64:
        case 128:
        case 256:
        case 512:
        case 1024:
        case 2048:
        case 4096:
            // Supported buffer size
            // Multiply by 8 to get the bufferSize in bytes 
            // rather than in samples per buffer
            bufferSize *= 8;
            break;
        default :
            JS_ReportError(cx, "Unsuported buffer size %d. Supported values are : 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 \n", bufferSize);
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
    NativeJSAudio *jaudio = NativeJSAudio::getContext();

    if (jaudio) {
        NativeAudioParameters *params = jaudio->audio->outputParameters;
        if (params->bufferSize != bufferSize ||
            params->channels != channels ||
            params->sampleRate != sampleRate) {
            paramsChanged = true;
        }
    }

    if (!paramsChanged && jaudio) {
        args.rval().set(OBJECT_TO_JSVAL(jaudio->getJSObject()));
        return true;
    }

    if (paramsChanged) {
        JSContext *m_Cx = cx;
        JS_SetPrivate(jaudio->getJSObject(), NULL);
        NJS->unrootObject(jaudio->getJSObject());
        delete jaudio;
    } 

    JSObject *ret = JS_NewObjectForConstructor(cx, &AudioContext_class, vp);
    NativeJSAudio *naudio = NativeJSAudio::getContext(cx, ret, bufferSize, channels, sampleRate);

    if (naudio == NULL) {
        delete naudio;
        JS_ReportError(cx, "Failed to initialize audio context\n");
        return false;
    }

    args.rval().set(OBJECT_TO_JSVAL(ret));

    return true;
}

static JSBool native_audio_run(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JSString *fn;
    JSFunction *nfn;
    jsval *argv = args.array();
    NativeJSAudio *audio = NativeJSAudio::getContext();

    CHECK_INVALID_CTX(audio);

    if ((nfn = JS_ValueToFunction(cx, argv[0])) == NULL ||
        (fn = JS_DecompileFunctionBody(cx, nfn, 0)) == NULL) {
        JS_ReportError(cx, "Failed to read callback function\n");
        return false;
    } 

    char *funStr = JS_EncodeString(cx, fn);

    audio->audio->sharedMsg->postMessage(
            (void *)new NativeAudioNode::CallbackMessage(NativeJSAudio::runCallback, NULL, static_cast<void *>(funStr)), 
            NATIVE_AUDIO_CALLBACK);


    return true;
}

static JSBool native_audio_load(JSContext *cx, unsigned argc, jsval *vp)
{
    JS_ReportError(cx, "Not implemented");
    return false;
}

static JSBool native_audio_createnode(JSContext *cx, unsigned argc, jsval *vp)
{
    int in, out;
    JSObject *ret;
    JSString *name;
    NativeJSAudio *audio;
    NativeJSAudioNode *node;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudio, &AudioContext_class);
    audio = CppObj;

    node = NULL;
    ret = NULL;

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "Suu", &name, &in, &out)) {
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

    JSAutoByteString cname(cx, name);
    ret = JS_NewObjectForConstructor(cx, &AudioNode_class, vp);
    if (!ret) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    JS_SetReservedSlot(ret, 0, JSVAL_NULL);

    try {
        if (strcmp("source", cname.ptr()) == 0) {
            node = new NativeJSAudioNode(ret, cx, NativeAudio::SOURCE, in, out, audio);
            NativeAudioSource *source = static_cast<NativeAudioSource*>(node->node);
            source->eventCallback(NativeJSAudioNode::onEvent, node);
            JS_DefineFunctions(cx, ret, AudioNodeSource_funcs);
            JS_DefineProperties(cx, ret, AudioNodeSource_props);
        } else if (strcmp("custom-source", cname.ptr()) == 0) {
            node = new NativeJSAudioNode(ret, cx, NativeAudio::CUSTOM_SOURCE, in, out, audio);
            JS_DefineProperties(cx, ret, AudioNodeCustom_props);
            JS_DefineFunctions(cx, ret, AudioNodeCustom_funcs);
            JS_DefineFunctions(cx, ret, AudioNodeCustomSource_funcs);
            JS_DefineProperties(cx, ret, AudioNodeCustomSource_props);
        } else if (strcmp("custom", cname.ptr()) == 0) {
            node = new NativeJSAudioNode(ret, cx, NativeAudio::CUSTOM, in, out, audio);
            JS_DefineProperties(cx, ret, AudioNodeCustom_props);
            JS_DefineFunctions(cx, ret, AudioNodeCustom_funcs);
        } else if (strcmp("reverb", cname.ptr()) == 0) {
            node = new NativeJSAudioNode(ret, cx, NativeAudio::REVERB, in, out, audio);
        } else if (strcmp("delay", cname.ptr()) == 0) {
            node = new NativeJSAudioNode(ret, cx, NativeAudio::DELAY, in, out, audio);
        } else if (strcmp("gain", cname.ptr()) == 0) {
            node = new NativeJSAudioNode(ret, cx, NativeAudio::GAIN, in, out, audio);
        } else if (strcmp("target", cname.ptr()) == 0) {                      
            if (audio->target != NULL) {
                args.rval().set(OBJECT_TO_JSVAL(audio->target->getJSObject()));
                return true;
            } else {
                node = new NativeJSAudioNode(ret, cx, NativeAudio::TARGET, in, out, audio);
                audio->target = node;
            }
        } else if (strcmp("stereo-enhancer", cname.ptr()) == 0) {
            node = new NativeJSAudioNode(ret, cx, NativeAudio::STEREO_ENHANCER, in, out, audio);
        } else {
            JS_ReportError(cx, "Unknown node name : %s\n", cname.ptr());
            return false;
        }
    } catch (NativeAudioNodeException *e) {
        delete node;
        JS_ReportError(cx, "Error while creating node : %s\n", e->what());
        return false;
    }

    if (node == NULL || node->node == NULL) {
        delete node;
        JS_ReportError(cx, "Error while creating node : %s\n", cname.ptr());
        return false;
    }

    audio->initNode(node, ret, name);

    args.rval().set(OBJECT_TO_JSVAL(ret));

    return true;
}

static JSBool native_audio_connect(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *link1;
    JSObject *link2;
    NodeLink *nlink1;
    NodeLink *nlink2;
    NativeJSAudio *jaudio;
    
    JSNATIVE_PROLOGUE_CLASS(NativeJSAudio, &AudioContext_class);
    jaudio = CppObj;

    NativeAudio *audio = jaudio->audio;

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "oo", &link1, &link2)) {
        return true;
    }

    nlink1 = (NodeLink *)JS_GetInstancePrivate(cx, link1, &AudioNodeLink_class, args.array());
    nlink2 = (NodeLink *)JS_GetInstancePrivate(cx, link2, &AudioNodeLink_class, args.array());

    if (nlink1 == NULL || nlink2 == NULL) {
        JS_ReportError(cx, "Bad AudioNodeLink\n");
        return false;
    }

    if (nlink1->type == INPUT && nlink2->type == OUTPUT) {
        if (!audio->connect(nlink2, nlink1)) {
            JS_ReportError(cx, "connect() failed (max connection reached)\n");
            return false;
        }
    } else if (nlink1->type == OUTPUT && nlink2->type == INPUT) {
        if (!audio->connect(nlink1, nlink2)) {
            JS_ReportError(cx, "connect() failed (max connection reached)\n");
            return false;
        }
    } else {
        JS_ReportError(cx, "connect() take one input and one output\n");
        return false;
    }

    return true;
}

static JSBool native_audio_disconnect(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *link1;
    JSObject *link2;
    NodeLink *nlink1;
    NodeLink *nlink2;
    NativeJSAudio *jaudio;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudio, &AudioContext_class);
    jaudio = CppObj;

    NativeAudio *audio = jaudio->audio;

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "oo", &link1, &link2)) {
        return true;
    }

    nlink1 = (NodeLink *)JS_GetInstancePrivate(cx, link1, &AudioNodeLink_class, args.array());
    nlink2 = (NodeLink *)JS_GetInstancePrivate(cx, link2, &AudioNodeLink_class, args.array());

    if (nlink1 == NULL || nlink2 == NULL) {
        JS_ReportError(cx, "Bad AudioNodeLink\n");
        return false;
    }

    if (nlink1->type == INPUT && nlink2->type == OUTPUT) {
        audio->disconnect(nlink2, nlink1);
    } else if (nlink1->type == OUTPUT && nlink2->type == INPUT) {
        audio->disconnect(nlink1, nlink2);
    } else {
        JS_ReportError(cx, "disconnect() take one input and one output\n");
        return false;
    }

    return true;
}

static JSBool native_audiothread_print(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JSString *str;
    jsval *argv;
    char *bytes;

    argv = args.array();

    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return true;

    bytes = JS_EncodeString(cx, str);
    if (!bytes)
        return true;

    printf("%s\n", bytes);

    JS_free(cx, bytes);

    return true;
}

static JSBool native_audio_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    NativeJSAudio *jaudio = NativeJSAudio::getContext();

    CHECK_INVALID_CTX(jaudio);

    if (vp.isNumber()) {
        jaudio->audio->setVolume((float)vp.toNumber());
    } 

    return true;
}

static JSBool native_audio_prop_getter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp)
{
    NativeJSAudio *jaudio = NativeJSAudio::getContext();

    CHECK_INVALID_CTX(jaudio);

    NativeAudioParameters *params = jaudio->audio->outputParameters;

    switch(JSID_TO_INT(id)) {
        case AUDIO_PROP_BUFFERSIZE:
            vp.set(INT_TO_JSVAL(params->bufferSize/8));
        break;
        case AUDIO_PROP_CHANNELS:
            vp.set(INT_TO_JSVAL(params->channels));
        break;
        case AUDIO_PROP_SAMPLERATE:
            vp.set(INT_TO_JSVAL(params->sampleRate));
        break;
        case AUDIO_PROP_VOLUME:
            vp.set(DOUBLE_TO_JSVAL(jaudio->audio->getVolume()));
        break;
        default:
            return false;
        break;
    }

    return true;
}

static JSBool native_AudioNode_constructor(JSContext *cx, unsigned argc, jsval *vp)
{
    JS_ReportError(cx, "Illegal constructor");
    return false;
}

void native_audionode_set_internal(JSContext *cx, NativeAudioNode *node, const char *prop, JS::Value *val)
{
    ArgType type;
    void *value;
    int intVal = 0;
    double doubleVal = 0;
    unsigned long size;

    if (val->isInt32()) {
        type = INT;
        size = sizeof(int);
        intVal = val->toInt32();
        value = &intVal;
    } else if (val->isDouble()) {
        type = DOUBLE;
        size = sizeof(double);
        doubleVal = val->toDouble();
        value = &doubleVal;
    } else {
        JS_ReportError(cx, "Unsuported value\n");
        return;
    }

    if (!node->set(prop, type, value, size)) {
        JS_ReportError(cx, "Unknown argument name %s\n", prop);
    } 
}
static JSBool native_audionode_set(JSContext *cx, unsigned argc, jsval *vp)
{
    JSString *name;
    NativeJSAudioNode *jnode;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudioNode, &AudioNode_class);
    jnode = CppObj;

    NativeAudioNode *node = jnode->node;
    if (!JSVAL_IS_PRIMITIVE(args.array()[0])) {
        JSObject *props = JSVAL_TO_OBJECT(args.array()[0]);
        js::AutoIdArray ida(cx, JS_Enumerate(cx, props));
        for (size_t i = 0; i < ida.length(); i++) {
            js::Rooted<jsid> id(cx, ida[i]);
            JS::Value val;
            JSAutoByteString name(cx, JSID_TO_STRING(id));

            if (!JS_GetPropertyById(cx, props, id, &val)) {
                break;
            }
            native_audionode_set_internal(cx, node, name.ptr(), &val);
        }
    } else {
        if (!JS_ConvertArguments(cx, args.length(), args.array(), "S", &name)) {
            return false;
        }

        JSAutoByteString cname(cx, name);
        JS::Value val = args.array()[1];

        native_audionode_set_internal(cx, node, cname.ptr(), &val);
    }

    return true;
}

static JSBool native_audionode_get(JSContext *cx, unsigned argc, jsval *vp)
{
    return true;
}

static JSBool native_audionode_custom_set(JSContext *cx, unsigned argc, jsval *vp)
{
    JSString *name;
    JS::Value val;
    NativeJSAudioNode::Message *msg;
    NativeAudioNodeCustom *node;
    NativeJSAudioNode *jnode;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudioNode, &AudioNode_class);
    jnode = CppObj;

    if (argc == 1 && !JSVAL_IS_PRIMITIVE(args.array()[0])) {
        val = args.array()[0];
        name = NULL;
    } else if (argc == 1) {
        JS_ReportError(cx, "Invalid arguments");
        return false;
    } else {
        if (!JS_ConvertArguments(cx, args.length(), args.array(), "S", &name)) {
            return false;
        }
        val = args.array()[1];
    }

    node = static_cast<NativeAudioNodeCustom *>(jnode->node);

    msg = new NativeJSAudioNode::Message();
    if (name != NULL) {
        msg->name = JS_EncodeString(cx, name);
    }
    msg->jsNode = jnode;

    if (!JS_WriteStructuredClone(cx, args.array()[1], 
            &msg->clone.datap, &msg->clone.nbytes,
            NULL, NULL, JSVAL_VOID)) {
        JS_ReportError(cx, "Failed to write structured clone");

        JS_free(cx, msg->name);
        delete msg;

        return false;
    }

    if (!jnode->nodeObj) {
        node->callback(NativeJSAudioNode::initCustomObject, static_cast<void *>(jnode));
    }
    node->callback(NativeJSAudioNode::setPropCallback, msg);

    return true;
}

#if 0
static JSBool native_audionode_custom_get(JSContext *cx, unsigned argc, jsval *vp)
{
    printf("hello get\n");

    NativeJSAudioNode *jsNode;
    jsval val;
    JSString *name;
    printf("get node\n");

    jsNode = NATIVE_AUDIO_NODE_GETTER(&args.thisv().toObject());

    printf("convert\n");
    if (!JS_ConvertArguments(cx, args.length(), args.array(), "S", &name)) {
        return true;
    }

    printf("str\n");
    JSAutoByteString str(cx, name);

    JS_SetRuntimeThread(JS_GetRuntime(cx));

    printf("get\n");
    JS_GetProperty(jsNode->audio->tcx, jsNode->hashObj, str.ptr(), &val);

    printf("return\n");
    JS_ClearRuntimeThread(JS_GetRuntime(cx));

    args.rval().set(val);

    return true;
}
#endif

static JSBool native_audionode_custom_threaded_set(JSContext *cx, unsigned argc, jsval *vp)
{
    JSString *name;
    JS::Value val;
    NativeJSAudioNode *jnode;
    JSTransferableFunction *fn;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudioNode, &AudioNode_class);
    jnode = CppObj;

    if (argc != 2) {
        JS_ReportError(cx, "set() require two arguments\n");
        return false;
    }

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "S", &name)) {
        return false;
    }

    fn = jnode->m_TransferableFuncs[NativeJSAudioNode::SETTER_FN];
    val = args.array()[1];

    JSAutoByteString str(cx, name);

    JS_SetProperty(cx, jnode->hashObj, str.ptr(), &val);

    if (fn) {
        jsval params[4];
        jsval rval;

        params[0] = STRING_TO_JSVAL(name);
        params[1] = val;
        params[2] = OBJECT_TO_JSVAL(JS_GetGlobalObject(cx));

        fn->call(cx, jnode->nodeObj, 3, params, &rval);
    }

    return true;
}

static JSBool native_audionode_custom_threaded_get(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval val;
    JSString *name;
    NativeJSAudioNode *jnode;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudioNode, &AudioNode_threaded_class);
    jnode = CppObj;

    if (jnode->hashObj == NULL) {
        args.rval().set(JSVAL_NULL);
        return false;
    }

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "S", &name)) {
        return false;
    }

    JSAutoByteString str(cx, name);

    JS_GetProperty(jnode->audio->tcx, jnode->hashObj, str.ptr(), &val);

    args.rval().set(val);

    return true;
}

static JSBool native_audionode_custom_threaded_send(JSContext *cx, unsigned argc, jsval *vp)
{
    uint64_t *datap;
    size_t nbytes;
    NativeJSAudioNode *jnode;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudioNode, &AudioNode_threaded_class);
    jnode = CppObj;

    struct native_thread_msg *msg;

    if (!JS_WriteStructuredClone(cx, args.array()[0], &datap, &nbytes,
        NULL, NULL, JSVAL_VOID)) {
        JS_ReportError(cx, "Failed to write structured clone");
        return false;
    }

    msg = new struct native_thread_msg;

    msg->data   = datap;
    msg->nbytes = nbytes;
    msg->callee = jnode->getJSObject();

    jnode->postMessage(msg, CUSTOM_SOURCE_SEND);

    return true;
}

static JSBool native_audionode_source_open(JSContext *cx, unsigned argc, jsval *vp) 
{
    NativeJSAudioNode *jnode;

    JSNATIVE_PROLOGUE_CLASS(NativeJSAudioNode, &AudioNode_class);
    jnode = CppObj;

    NativeAudioSource *source = (NativeAudioSource *)jnode->node;

    JS::Value src = args.array()[0];

    int ret = -1;

    if (src.isString()) {
        JSAutoByteString csrc(cx, src.toString());
        ret = source->open(csrc.ptr());
    } else if (src.isObject()) {
        JSObject *arrayBuff = src.toObjectOrNull();

        if (!JS_IsArrayBufferObject(arrayBuff)) {
            JS_ReportError(cx, "Data is not an ArrayBuffer\n");
            return false;
        }

        int length;
        uint8_t *data;

        length = JS_GetArrayBufferByteLength(arrayBuff);
        JS_StealArrayBufferContents(cx, arrayBuff, &jnode->arrayContent, &data);

        ret = source->open(data, length);
    }
    
    if (ret < 0) {
        JS_ReportError(cx, "Failed to open stream %d\n", ret);
        return false;
    }

    /*
    int bufferSize = sizeof(uint8_t)*4*1024*1024;
    uint8_t *buffer;

    buffer = (uint8_t *)malloc(bufferSize);
    load("/tmp/foo.wav", buffer, bufferSize);
    int ret = source->open(buffer, bufferSize);
    */

    return true;
}
static JSBool native_audionode_source_play(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeAudioSource *source;

    JSNATIVE_AV_GET_NODE(NativeAudioSource, source);

    source->play();

    return true;
}

static JSBool native_audionode_source_pause(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeAudioSource *source;

    JSNATIVE_AV_GET_NODE(NativeAudioSource, source);

    source->pause();

    return true;
}

static JSBool native_audionode_source_stop(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeAudioSource *source;

    JSNATIVE_AV_GET_NODE(NativeAudioSource, source);

    source->stop();

    return true;
}

static JSBool native_audionode_source_close(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeAudioSource *source;

    JSNATIVE_AV_GET_NODE(NativeAudioSource, source);

    source->close();

    return true;
}

static JSBool native_audionode_source_prop_getter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp)
{
    NativeJSAudioNode *jnode = (NativeJSAudioNode *)JS_GetPrivate(obj);

    CHECK_INVALID_CTX(jnode);

    NativeAudioSource *source = static_cast<NativeAudioSource *>(jnode->node);

    return NativeJSAVSource::propGetter(source, cx, JSID_TO_INT(id), vp);
}

static JSBool native_audionode_source_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    NativeJSAudioNode *jnode = (NativeJSAudioNode *)JS_GetPrivate(obj);

    CHECK_INVALID_CTX(jnode);

    NativeAudioSource *source = static_cast<NativeAudioSource*>(jnode->node);

    return NativeJSAVSource::propSetter(source, JSID_TO_INT(id), vp);
}

static JSBool native_audionode_custom_source_play(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeAudioCustomSource *source;

    JSNATIVE_AV_GET_NODE(NativeAudioCustomSource, source);

    source->play();

    return true;
}

static JSBool native_audionode_custom_source_pause(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeAudioCustomSource *source;

    JSNATIVE_AV_GET_NODE(NativeAudioCustomSource, source);

    source->pause();

    return true;
}

static JSBool native_audionode_custom_source_stop(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeAudioCustomSource *source;

    JSNATIVE_AV_GET_NODE(NativeAudioCustomSource, source);

    source->stop();

    return true;
}

static JSBool native_audionode_custom_source_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    NativeJSAudioNode *jnode = (NativeJSAudioNode *)JS_GetPrivate(obj);

    CHECK_INVALID_CTX(jnode);

    return NativeJSAudioNode::propSetter(jnode, cx, JSID_TO_INT(id), vp);
}

static JSBool native_audionode_custom_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    NativeJSAudioNode *jnode = (NativeJSAudioNode *)JS_GetPrivate(obj);
    JSTransferableFunction *fun;
    NativeAudioNodeCustom *node;
    NativeJSAudioNode::TransferableFunction funID;

    CHECK_INVALID_CTX(jnode);

    node = static_cast<NativeAudioNodeCustom *>(jnode->node);

    switch(JSID_TO_INT(id)) {
        case NODE_CUSTOM_PROP_PROCESS:
            funID = NativeJSAudioNode::PROCESS_FN;
        break;
        case NODE_CUSTOM_PROP_SETTER: 
            funID = NativeJSAudioNode::SETTER_FN;
        break;
        case NODE_CUSTOM_PROP_INIT :
            funID = NativeJSAudioNode::INIT_FN;
        break;
        default:
            return true;
            break;
    }

    fun = new JSTransferableFunction();

    if (!fun->prepare(cx, vp.get())) {
        JS_ReportError(cx, "Failed to read custom node callback function\n");
        vp.set(JSVAL_VOID);
        delete fun;
        return false;
    }

    if (jnode->m_TransferableFuncs[funID] != NULL) {
        delete jnode->m_TransferableFuncs[funID];
    }

    jnode->m_TransferableFuncs[funID] = fun;

    if (JSID_TO_INT(id) == NODE_CUSTOM_PROP_PROCESS) {
        if (!jnode->nodeObj) {
            node->callback(NativeJSAudioNode::initCustomObject, static_cast<void *>(jnode));
        }
        node->setCallback(NativeJSAudioNode::customCallback, static_cast<void *>(jnode));
    }

    return true;
}

static JSBool native_video_prop_getter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp)
{
    NativeJSVideo *v = (NativeJSVideo *)JS_GetPrivate(obj);
    if (v == NULL) {
        return false;
    }

    switch(JSID_TO_INT(id)) {
        case VIDEO_PROP_WIDTH:
            vp.set(INT_TO_JSVAL(v->video->codecCtx->width));
        break;
        case VIDEO_PROP_HEIGHT:
            vp.set(INT_TO_JSVAL(v->video->codecCtx->height));
        break;
        default:
            return NativeJSAVSource::propGetter(v->video, cx, JSID_TO_INT(id), vp);
            break;
    }

    return true;
}

static JSBool native_video_prop_setter(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    NativeJSVideo *v = (NativeJSVideo *)JS_GetPrivate(obj);
    if (v == NULL) {
        return false;
    }

    return NativeJSAVSource::propSetter(v->video, JSID_TO_INT(id), vp);
}

void AudioContext_Finalize(JSFreeOp *fop, JSObject *obj)
{
    NativeJSAudio *audio = (NativeJSAudio *)JS_GetPrivate(obj);
    if (audio != NULL) {
        delete audio;
    }
}

void AudioNode_Finalize(JSFreeOp *fop, JSObject *obj)
{
    NativeJSAudioNode *node = (NativeJSAudioNode *)JS_GetPrivate(obj);
    if (node != NULL) {
        delete node;
    } 
}

NativeJSVideo::NativeJSVideo(JSObject *obj,
    NativeCanvas2DContext *canvasCtx, JSContext *cx) :
    NativeJSExposer<NativeJSVideo>(obj, cx),
    video(NULL), audioNode(NULL), arrayContent(NULL), 
    m_Width(-1), m_Height(-1), m_Left(0), m_Top(0), m_IsDestructing(false), 
    m_CanvasCtx(canvasCtx), cx(cx)
{
    this->video = new NativeVideo((ape_global *)JS_GetContextPrivate(cx));
    this->video->frameCallback(NativeJSVideo::frameCallback, this);
    this->video->eventCallback(NativeJSVideo::onEvent, this);
    m_CanvasCtx->getHandler()->addListener(this);
}

void NativeJSVideo::stopAudio() 
{
    this->video->stopAudio();

    if (this->audioNode) {
        NJS->unrootObject(this->audioNode);
        NativeJSAudioNode *jnode = static_cast<NativeJSAudioNode *>(JS_GetPrivate(this->audioNode));
        JS_SetReservedSlot(jnode->getJSObject(), 0, JSVAL_NULL);
        this->audioNode = NULL;
    } 
}

void NativeJSVideo::onMessage(const NativeSharedMessages::Message &msg)
{
    if (m_IsDestructing) return;

    if (msg.event() == NATIVE_EVENT(NativeCanvasHandler, RESIZE_EVENT) && (m_Width == -1 || m_Height == -1)) {
        this->setSize(m_Width, m_Height);
    } else {
        if (msg.event() == SOURCE_EVENT_PLAY) {
            this->setSize(m_Width, m_Height);
        }
        native_av_thread_message(cx, this->getJSObject(), msg);
    }
}

void NativeJSVideo::onEvent(const struct NativeAVSourceEvent *cev) 
{
    NativeJSVideo *thiz = static_cast<NativeJSVideo *>(cev->custom);
    thiz->postMessage((void *)cev, cev->ev);
}

void NativeJSVideo::frameCallback(uint8_t *data, void *custom)
{
    NativeJSVideo *v = (NativeJSVideo *)custom;
    NativeCanvasHandler *handler = v->m_CanvasCtx->getHandler();
    NativeSkia *surface = v->m_CanvasCtx->getSurface();

    surface->setFillColor(0xFF000000);
    surface->drawRect(0, 0, handler->getWidth(), handler->getHeight(), 0);
    surface->drawPixels(data, v->video->m_Width, v->video->m_Height, v->m_Left, v->m_Top);

    jsval onframe;
    if (JS_GetProperty(v->cx, v->getJSObject(), "onframe", &onframe) &&
        !JSVAL_IS_PRIMITIVE(onframe) &&
        JS_ObjectIsCallable(v->cx, JSVAL_TO_OBJECT(onframe))) {
        jsval params, rval;

        params = OBJECT_TO_JSVAL(v->getJSObject());

        JSAutoRequest ar(v->cx);
        JS_CallFunctionValue(v->cx, v->getJSObject(), onframe, 1, &params, &rval);
    }
}

void NativeJSVideo::setSize(int width, int height)
{
    m_Width = width;
    m_Height = height;

    if (!video->codecCtx) {
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
        int videoWidth = video->codecCtx->width;
        int videoHeight = video->codecCtx->height;

        int maxWidth = native_min(m_Width == -1 ? canvasWidth : m_Width, canvasWidth);
        int maxHeight = native_min(m_Height == -1 ? canvasHeight : m_Height, canvasHeight);
        double ratio = native_max(videoHeight / (double)maxHeight, videoWidth / (double)maxWidth);

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
  
    this->video->setSize(width, height);
}

static JSBool native_video_play(JSContext *cx, unsigned argc, jsval *vp)
{
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    CppObj->video->play();

    return true;
}

static JSBool native_video_pause(JSContext *cx, unsigned argc, jsval *vp)
{
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    CppObj->video->pause();

    return true;
}

static JSBool native_video_stop(JSContext *cx, unsigned argc, jsval *vp)
{
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    CppObj->video->stop();

    return true;
}

static JSBool native_video_close(JSContext *cx, unsigned argc, jsval *vp)
{
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    CppObj->video->close();

    return true;
}

static JSBool native_video_open(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeJSVideo *v;
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);
    v = CppObj;

    JS::Value src = args.array()[0];
    int ret = -1;

    if (src.isString()) {
        JSAutoByteString csrc(cx, src.toString());
        ret = v->video->open(csrc.ptr());
    } else if (src.isObject()) {
        JSObject *arrayBuff = src.toObjectOrNull();

        if (!JS_IsArrayBufferObject(arrayBuff)) {
            JS_ReportError(cx, "Data is not an ArrayBuffer\n");
            return false;
        }

        int length;
        uint8_t *data;

        length = JS_GetArrayBufferByteLength(arrayBuff);
        JS_StealArrayBufferContents(cx, arrayBuff, &v->arrayContent, &data);

        if (v->video->open(data, length) < 0) {
            args.rval().set(JSVAL_FALSE);
            return true;
        }
    }

    args.rval().set(JSVAL_TRUE);

    return true;
}

static JSBool native_video_get_audionode(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeJSAudio *jaudio = NativeJSAudio::getContext();
    NativeJSVideo *v;

    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    v = CppObj;

    if (!jaudio) {
        JS_ReportError(cx, "No Audio context");
        args.rval().set(JSVAL_NULL);
        return false;
    }

    if (v->audioNode) {
        args.rval().set(OBJECT_TO_JSVAL(v->audioNode));
        return true;
    }

    NativeAudioSource *source = v->video->getAudioNode(jaudio->audio);

    if (source != NULL) {
        v->audioNode = JS_NewObjectForConstructor(cx, &AudioNode_class, vp);

        NativeJSAudioNode *node = new NativeJSAudioNode(v->audioNode, cx,
          NativeAudio::SOURCE, static_cast<class NativeAudioNode *>(source), jaudio);

        JSString *name = JS_NewStringCopyN(cx, "video-source", 12);
        jaudio->initNode(node, v->audioNode, name);

        JS_SetReservedSlot(node->getJSObject(), 0, OBJECT_TO_JSVAL(v->getJSObject()));

        args.rval().set(OBJECT_TO_JSVAL(v->audioNode));
    } else {
        args.rval().set(JSVAL_NULL);
    }

    return true;
}

static JSBool native_video_nextframe(JSContext *cx, unsigned argc, jsval *vp)
{
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    CppObj->video->nextFrame();

    return true;
}

static JSBool native_video_prevframe(JSContext *cx, unsigned argc, jsval *vp)
{
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    CppObj->video->prevFrame();

    return true;
}

static JSBool native_video_frameat(JSContext *cx, unsigned argc, jsval *vp)
{
    double time;
    JSBool keyframe;

    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "db", &time, &keyframe)) {
        return true;
    }

    CppObj->video->frameAt(time, keyframe);

    return true;
}

static JSBool native_video_setsize(JSContext *cx, unsigned argc, jsval *vp)
{
    double width;
    double height;
    JSNATIVE_PROLOGUE_CLASS(NativeJSVideo, &Video_class);

    if (argc < 2) {
        JS_ReportError(cx, "Wrong arguments count");
        return false;
    }

    jsval jwidth = args.array()[0];
    jsval jheight = args.array()[1];

    if (JSVAL_IS_STRING(jwidth)) {
        width = -1;
    } else if (JSVAL_IS_INT(jwidth)) {
        width = JSVAL_TO_INT(jwidth);
    } else {
        JS_ReportError(cx, "Wrong argument type for width");
        return false;
    }

    if (JSVAL_IS_STRING(jheight)) {
        height = -1;
    } else if (JSVAL_IS_INT(jheight)) {
        height = JSVAL_TO_INT(jheight);
    } else {
        JS_ReportError(cx, "Wrong argument type for height");
        return false;
    }

    CppObj->setSize(width, height);

    return true;
}

static JSBool native_Video_constructor(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JSObject *ret = JS_NewObjectForConstructor(cx, &Video_class, vp);
    JSObject *canvas;

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "o", &canvas)) {
        return true;
    }

    NativeCanvasHandler *handler = static_cast<class NativeJSCanvas*>(JS_GetInstancePrivate(cx, canvas, &Canvas_class, args.array()))->getHandler();

    if (!handler) {
        JS_ReportError(cx, "Video constructor argument must be Canvas");
        return false;
    }

    NativeCanvasContext *ncc = handler->getContext();
    if (ncc == NULL || ncc->m_Mode != NativeCanvasContext::CONTEXT_2D) {
        JS_ReportError(cx, "Invalid destination canvas (must be backed by a 2D context)");
        return false;
    }
    JSContext *m_Cx = cx;
    NJS->rootObjectUntilShutdown(ret);

    NativeJSVideo *v = new NativeJSVideo(ret, (NativeCanvas2DContext*)ncc, cx);

    JS_DefineFunctions(cx, ret, Video_funcs);
    JS_DefineProperties(cx, ret, Video_props);

    JS_SetPrivate(ret, v);

    JS_SetProperty(cx, ret, "canvas", &(args.array()[0]));

    args.rval().set(OBJECT_TO_JSVAL(ret));

    return true;
}

NativeJSVideo::~NativeJSVideo() 
{
    JSAutoRequest ar(cx);
    m_IsDestructing = true;
    if (this->audioNode) {
        NativeJSAudioNode *node = static_cast<NativeJSAudioNode *>(JS_GetPrivate(this->audioNode));
        NJS->unrootObject(this->audioNode);
        if (node) {
            // This will remove the source from 
            // NativeJSAudio nodes list and NativeAudio sources 
            delete node; 
        }
    } 
    
    if (this->arrayContent != NULL) {
        free(this->arrayContent);
    }

    delete this->video;
}

static void Video_Finalize(JSFreeOp *fop, JSObject *obj) {
    NativeJSVideo *v = (NativeJSVideo *)JS_GetPrivate(obj);

    if (v != NULL) {
        delete v;
    }
}

bool NativeJSAVSource::propSetter(NativeAVSource *source, int id, JSMutableHandleValue vp) 
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

bool NativeJSAVSource::propGetter(NativeAVSource *source, JSContext *cx, int id, JSMutableHandleValue vp)
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
            AVDictionaryEntry *tag = NULL;
            AVDictionary *cmetadata = source->getMetadata();

            if (cmetadata != NULL) {
                JSObject *metadata = JS_NewObject(cx, NULL, NULL, NULL);

                while ((tag = av_dict_get(cmetadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
                    JSString *val = JS_NewStringCopyN(cx, tag->value, strlen(tag->value));

                    JS_DefineProperty(cx, metadata, tag->key, STRING_TO_JSVAL(val), NULL, NULL, 
                            JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT);
                }

                vp.setObject(*metadata);
            } else {
                vp.setUndefined();
            }
        }
        break;
        default:
        break;
    }

    return true;
}

bool NativeJSAudioNode::propSetter(NativeJSAudioNode *jnode, JSContext *cx,
        int id, JSMutableHandleValue vp) 
{
    NativeAudioCustomSource *source = static_cast<NativeAudioCustomSource*>(jnode->node);

    switch(id) {
        case SOURCE_PROP_POSITION:
            if (vp.isNumber()) {
                source->seek(vp.toNumber());
            } 
        break;
        case CUSTOM_SOURCE_PROP_SEEK: {
            JSTransferableFunction *fun = new JSTransferableFunction();
            int funID = NativeJSAudioNode::SEEK_FN;
            if (!fun->prepare(cx, vp.get())) {
                JS_ReportError(cx, "Failed to read custom node callback function\n");
                vp.set(JSVAL_VOID);
                delete fun;
                return false;
            }

            if (jnode->m_TransferableFuncs[funID] != NULL) {
                delete jnode->m_TransferableFuncs[funID];
            }

            jnode->m_TransferableFuncs[funID] = fun;

            source->setSeek(NativeJSAudioNode::seekCallback, jnode);
        }
        break;
        default:
        break;
    }

    return true;
}

void NativeJSAudioNode::seekCallback(NativeAudioCustomSource *node, double seekTime, void *custom)
{
    NativeJSAudioNode *jnode = static_cast<NativeJSAudioNode*>(custom);

    JSTransferableFunction *fn = jnode->m_TransferableFuncs[NativeJSAudioNode::SEEK_FN];
    if (!fn) return;

    jsval params[2];
    jsval rval;

    params[0] = DOUBLE_TO_JSVAL(seekTime);;
    params[1] = OBJECT_TO_JSVAL(JS_GetGlobalObject(jnode->audio->tcx));

    fn->call(jnode->audio->tcx, jnode->nodeObj, 2, params, &rval);
}

void NativeJSAudioNode::registerObject(JSContext *cx)
{
    JSObject *obj;

    obj = JS_InitClass(cx, JS_GetGlobalObject(cx), NULL, 
            &AudioNode_class, native_AudioNode_constructor, 0, 
            AudioNode_props, AudioNode_funcs, NULL, NULL);
}

void NativeJSAudio::registerObject(JSContext *cx)
{
    JS_InitClass(cx, JS_GetGlobalObject(cx), NULL, 
        &Audio_class, native_Audio_constructor, 0, 
        NULL, NULL, NULL, Audio_static_funcs);

    JS_InitClass(cx, JS_GetGlobalObject(cx), NULL, 
        &AudioContext_class, native_Audio_constructor, 0, 
        AudioContext_props, AudioContext_funcs, NULL, NULL);
}

NATIVE_OBJECT_EXPOSE(Video);
