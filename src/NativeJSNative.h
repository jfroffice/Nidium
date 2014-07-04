#ifndef nativejsnative_h__
#define nativejsnative_h__

#include "NativeJSExposer.h"

class NativeSkia;
class NativeCanvasHandler;

class NativeJSNative : public NativeJSExposer<NativeJSNative>
{
  public:
     NativeJSNative(){};
    ~NativeJSNative(){};

    static void registerObject(JSContext *cx);

    static const char *getJSObjectName() {
        return "native";
    }

    static JSClass *jsclass;
};

#endif
