#ifndef nativejsconsole_h__
#define nativejsconsole_h__

#include "NativeJSExposer.h"


class NativeJSconsole : public NativeJSExposer<NativeJSconsole>
{
  public:
    static void registerObject(JSContext *cx);
};

#endif
