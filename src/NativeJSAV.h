#ifndef nativejsav_h__
#define nativejsav_h__

#include "NativeJSExposer.h"
#include <NativeAudio.h>
#include <NativeAudioNode.h>

#include "NativeVideo.h"
#include "NativeSkia.h"

static int NATIVE_AV_THREAD_MESSAGE_CALLBACK = -1;

enum {
    NODE_EV_PROP_DATA, 
    NODE_EV_PROP_SIZE,
    NODE_CUSTOM_PROP_ONBUFFER,
    NODE_CUSTOM_PROP_ONINIT,
    NODE_CUSTOM_PROP_ONSET,
    AUDIO_PROP_VOLUME,
    VIDEO_PROP_WIDTH,
    VIDEO_PROP_HEIGHT,
    VIDEO_PROP_ONFRAME,
    VIDEO_PROP_CANVAS,
    SOURCE_PROP_POSITION,
    SOURCE_PROP_DURATION,
    SOURCE_PROP_METADATA,
    SOURCE_PROP_BITRATE
};

class NativeJS;
class NativeJSAudioNode;

struct NativeJSAVMessageCallback {
    JSObject *callee;
    int ev;
    int arg1;
    int arg2;

    NativeJSAVMessageCallback(JSObject *callee, int ev, int arg1, int arg2)
        : callee(callee), ev(ev), arg1(arg1), arg2(arg2) {};
};

class NativeJSAVSource
{
    public:
        static inline int open(NativeAVSource *source, JSContext *cx, unsigned argc, jsval *vp);
        static inline int play();
        static inline int pause();
        static inline int stop();

        static inline void propSetter(NativeAVSource *source, int id, JSMutableHandleValue vp);
        static inline void propGetter(NativeAVSource *source, JSContext *ctx, int id, JSMutableHandleValue vp);
};

static void NativeJSAVEventCbk(const struct NativeAVSourceEvent *ev);

class NativeJSAudio: public NativeJSExposer<NativeJSAudio>
{
    public :
        static NativeJSAudio *getContext(JSContext *cx, JSObject *obj, int bufferSize, int channels, int sampleRate);
        static NativeJSAudio *getContext();

        struct Nodes {
            NativeJSAudioNode *curr;

            Nodes *prev;
            Nodes *next;

            Nodes(NativeJSAudioNode *curr, Nodes *prev, Nodes *next) 
                : curr(curr), prev(prev), next(next) {}
            Nodes() 
                : curr(NULL), prev(NULL), next(NULL) {}
        };

        NativeAudio *audio;

        Nodes *nodes;

        pthread_t threadIO;

        pthread_cond_t shutdownCond;
        pthread_mutex_t shutdownLock;
        bool shutdowned;

        JSObject *jsobj;
        JSObject *gbl;

        JSRuntime *rt;
        JSContext *tcx;
        const char *fun;

        bool createContext();
        bool run(char *str);
        static void ctxCallback(NativeAudioNode *node, void *custom);
        static void runCallback(NativeAudioNode *node, void *custom);
        static void shutdownCallback(NativeAudioNode *dummy, void *custom);
        void unroot();

        static void registerObject(JSContext *cx);

        ~NativeJSAudio();
    private : 
        NativeJSAudio();
        static NativeJSAudio *instance;
};

class NativeJSAudioNode: public NativeJSExposer<NativeJSAudioNode>
{
    public :
        NativeJSAudioNode(NativeAudio::Node type, int in, int out, NativeJSAudio *audio) 
            :  audio(audio), type(type), bufferFn(NULL), bufferObj(NULL), bufferStr(NULL), onSetStr(NULL), onSetFn(NULL),
               initStr(NULL), nodeObj(NULL), hashObj(NULL), finalized(false), arrayContent(NULL) 
        { 

            try {
                this->node = audio->audio->createNode(type, in, out);
            } catch (NativeAudioNodeException *e) {
                throw;
            }

            if (type == NativeAudio::CUSTOM) {
                pthread_cond_init(&this->shutdownCond, NULL);
                pthread_mutex_init(&this->shutdownLock, NULL);
            }


            this->add();
        }

        NativeJSAudioNode(NativeAudio::Node type, NativeAudioNode *node, NativeJSAudio *audio) 
            :  audio(audio), node(node), type(type), bufferFn(NULL), bufferObj(NULL), bufferStr(NULL), 
               onSetStr(NULL), onSetFn(NULL), initStr(NULL), nodeObj(NULL), hashObj(NULL), finalized(false), arrayContent(NULL) 
        { 
            this->add();
        }

        ~NativeJSAudioNode();

        struct Message {
            NativeJSAudioNode *jsNode;
            char *name;
            struct {
                uint64_t *datap;
                size_t nbytes;
            } clone;
        };

        // Common
        NativeJS *njs;
        NativeJSAudio *audio;
        NativeAudioNode *node;
        JSObject *jsobj;
        NativeAudio::Node type;

        // Custom node
        static void customCallback(const struct NodeEvent *ev);
        static void customInitCallback(NativeAudioNode *node, void *custom);
        static void setPropCallback(NativeAudioNode *node, void *custom);
        static void onSetCallback(NativeAudioNode *node, void *custom);
        static void shutdownCallback(NativeAudioNode *node, void *custom);
        bool createHashObj();
        JSFunction *bufferFn;
        JSObject *bufferObj;
        JSObject *onSetObj;
        JSFunction *onSetFn;
        const char *bufferStr;
        const char *onSetStr;
        const char *initStr;
        JSObject *nodeObj;
        JSObject *hashObj;
        bool finalized;

        pthread_cond_t shutdownCond;
        pthread_mutex_t shutdownLock;

        // Source node
        void *arrayContent;
        static void eventCbk(const struct NativeAVSourceEvent *cev);

        static void registerObject(JSContext *cx);
    private : 
        void add();
};

class NativeJSVideo : public NativeJSExposer<NativeJSVideo>
{
    public :
        NativeJSVideo(NativeSkia *nskia, JSContext *cx);

        NativeVideo *video;

        JSObject *audioNode;
        void *arrayContent;

        int width;
        int height;

        void stopAudio();

        static void registerObject(JSContext *cx);
        static void frameCallback(uint8_t *data, void *custom);
        static void eventCbk(const struct NativeAVSourceEvent *cev);
        
        ~NativeJSVideo();
    private :
        NativeSkia *nskia;
        JSContext *cx;
};

#endif
