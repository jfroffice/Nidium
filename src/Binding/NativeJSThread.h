/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef nativejsthread_h__
#define nativejsthread_h__

#include <pthread.h>

#include "Core/NativeSharedMessages.h"
#include "Core/NativeMessages.h"
#include "JSExposer.h"

class NativeJS;

class NativeJSThread : public Nidium::Binding::JSExposer<NativeJSThread>, public NativeMessages
{
  public:
    NativeJSThread(JS::HandleObject obj, JSContext *cx);
    ~NativeJSThread();
    static void registerObject(JSContext *cx);
    void onComplete(JS::HandleValue vp);
    void onMessage(const NativeSharedMessages::Message &msg);

    JS::PersistentRootedString jsFunction;

    JSRuntime *jsRuntime;
    JSContext *jsCx;
    JSObject *jsObject;
    NativeJS *njs;
    struct {
        int argc;
        uint64_t **argv;
        size_t *nbytes;
    } params;
    bool markedStop;

    pthread_t threadHandle;

    char *m_CallerFileName;
    uint32_t m_CallerLineno;
};

enum {
    NATIVE_THREAD_MESSAGE = 0,
    NATIVE_THREAD_COMPLETE = 1
};

#endif

