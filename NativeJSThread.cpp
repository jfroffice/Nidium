/*
    NativeJS Core Library
    Copyright (C) 2013 Anthony Catel <paraboul@gmail.com>
    Copyright (C) 2013 Nicolas Trani <n.trani@weelya.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "NativeJSThread.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <js/OldDebugAPI.h>

#include "NativeJSConsole.h"

extern void reportError(JSContext *cx, const char *message,
    JSErrorReport *report);
static bool native_post_message(JSContext *cx, unsigned argc, JS::Value *vp);
static void Thread_Finalize(JSFreeOp *fop, JSObject *obj);
static bool native_thread_start(JSContext *cx, unsigned argc, JS::Value *vp);

#define NJS (NativeJS::getNativeClass(cx))

static JSClass global_Thread_class = {
    "_GLOBALThread", JSCLASS_GLOBAL_FLAGS | JSCLASS_IS_GLOBAL,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSClass Thread_class = {
    "Thread", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Thread_Finalize,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

template<>
JSClass *NativeJSExposer<NativeJSThread>::jsclass = &Thread_class;

static JSClass messageEvent_class = {
    "ThreadMessageEvent", 0,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSFunctionSpec glob_funcs_threaded[] = {
    JS_FN("send", native_post_message, 1, 0),
    JS_FS_END
};

static JSFunctionSpec Thread_funcs[] = {
    JS_FN("start", native_thread_start, 0, 0),
    JS_FS_END
};

static void Thread_Finalize(JSFreeOp *fop, JSObject *obj)
{
    NativeJSThread *nthread = (NativeJSThread *)JS_GetPrivate(obj);

    if (nthread != NULL) {
        delete nthread;
    }
}

static bool JSThreadCallback(JSContext *cx)
{
    NativeJSThread *nthread;

    if ((nthread = (NativeJSThread *)JS_GetContextPrivate(cx)) == NULL ||
        nthread->markedStop) {
        return false;
    }
    return true;
}

JSObject *_CreateJSGlobal(JSContext *cx)
{
    JS::CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);

    JS::RootedObject glob(cx, JS_NewGlobalObject(cx, &global_Thread_class, nullptr,
                                             JS::DontFireOnNewGlobalHook, options));

    JSAutoCompartment ac(cx, glob);

    JS_InitStandardClasses(cx, glob);
    JS_DefineDebuggerObject(cx, glob);

    JS_DefineFunctions(cx, glob, glob_funcs_threaded);

    JS_FireOnNewGlobalObject(cx, glob);

    return glob;
    //JS::RegisterPerfMeasurement(cx, glob);

    //https://bugzilla.mozilla.org/show_bug.cgi?id=880330
    // context option vs compile option?
}

static void *native_thread(void *arg)
{
    NativeJSThread *nthread = (NativeJSThread *)arg;

    JSRuntime *rt;
    JSContext *tcx;

    if ((rt = JS_NewRuntime(128L * 1024L * 1024L, JS_USE_HELPER_THREADS)) == NULL) {
        printf("Failed to init JS runtime\n");
        return NULL;
    }

    NativeJS::SetJSRuntimeOptions(rt);

    if ((tcx = JS_NewContext(rt, 8192)) == NULL) {
        printf("Failed to init JS context\n");
        return NULL;
    }

    JS_SetGCParameterForThread(tcx, JSGC_MAX_CODE_CACHE_BYTES, 16 * 1024 * 1024);
    JS::RootedValue rval(tcx, JSVAL_VOID);

    JS_SetStructuredCloneCallbacks(rt, NativeJS::jsscc);
    JS_SetInterruptCallback(rt, JSThreadCallback);

    nthread->jsRuntime = rt;
    nthread->jsCx      = tcx;

    /*
        repportError read the runtime private to use the logger
    */
    JS_SetRuntimePrivate(rt, nthread->njs);

    JS_BeginRequest(tcx);

    JS::RootedObject gbl(tcx, _CreateJSGlobal(tcx));

    JS_SetErrorReporter(tcx, reportError);

    js::SetDefaultObjectForContext(tcx, gbl);

    NativeJSconsole::registerObject(tcx);

    JSAutoByteString str(tcx, nthread->jsFunction);
    char *scoped = new char[strlen(str.ptr()) + 128];

    /*
        JS_CompileFunction takes a function body.
        This is a hack in order to catch the arguments name, etc...

        function() {
            (function (a) {
                this.send("hello" + a);
            }).apply(this, Array.prototype.slice.apply(arguments));
        };
    */
    sprintf(scoped, "return %c%s%s", '(', str.ptr(), ").apply(this, Array.prototype.slice.apply(arguments));");

    /* Hold the parent cx */
    JS_SetContextPrivate(tcx, nthread);

    JS::CompileOptions options(tcx);
    options.setFileAndLine(nthread->m_CallerFileName, nthread->m_CallerLineno)
           .setUTF8(true);

    JS::RootedFunction cf(tcx);
    cf = JS::CompileFunction(tcx, gbl, options, NULL, 0, NULL, scoped, strlen(scoped));
    delete[] scoped;

    if (cf == NULL) {
        printf("Cant compile function\n");
        JS_EndRequest(tcx);
        return NULL;
    }

    JS::AutoValueVector arglst(tcx);
    arglst.resize(nthread->params.argc);

    for (size_t i = 0; i < nthread->params.argc; i++) {
        JS::RootedValue arg(tcx);
        JS_ReadStructuredClone(tcx,
                    nthread->params.argv[i],
                    nthread->params.nbytes[i],
                    JS_STRUCTURED_CLONE_VERSION, &arg, NULL, NULL);

        arglst[i] = arg;
        JS_ClearStructuredClone(nthread->params.argv[i], nthread->params.nbytes[i], NULL, NULL);
    }

    if (JS_CallFunction(tcx, gbl, cf, arglst, &rval) == false) {
    }

    JS_EndRequest(tcx);

    free(nthread->params.argv);
    free(nthread->params.nbytes);

    nthread->onComplete(rval);

    JS_DestroyContext(tcx);
    JS_DestroyRuntime(rt);

    nthread->jsRuntime = NULL;
    nthread->jsCx = NULL;

    return NULL;
}

static bool native_thread_start(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NativeJSThread *nthread;
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject caller(cx, JS_THIS_OBJECT(cx, vp));

    if (JS_InstanceOf(cx, caller, &Thread_class, &args) == false) {
        return true;
    }

    if ((nthread = (NativeJSThread *)JS_GetPrivate(caller)) == NULL) {
        return true;
    }

    nthread->params.argv = (argc ?
        (uint64_t **)malloc(sizeof(*nthread->params.argv) * argc) : NULL);
    nthread->params.nbytes = (argc ?
        (size_t *)malloc(sizeof(*nthread->params.nbytes) * argc) : NULL);

    for (int i = 0; i < (int)argc; i++) {

        if (!JS_WriteStructuredClone(cx, args[i],
            &nthread->params.argv[i], &nthread->params.nbytes[i],
            NULL, NULL, JS::NullHandleValue)) {

            return false;
        }
    }

    nthread->params.argc = argc;

    /* TODO: check if already running */
    pthread_create(&nthread->threadHandle, NULL,
                            native_thread, nthread);

    nthread->njs->rootObjectUntilShutdown(caller);

    return true;
}

void NativeJSThread::onMessage(const NativeSharedMessages::Message &msg)
{
#define EVENT_PROP(name, val) JS_DefineProperty(m_Cx, event, name, \
    val, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)
    struct native_thread_msg *ptr;
    char prop[16];
    int ev = msg.event();

    JS::RootedValue jscbk(m_Cx);
    JS::AutoValueArray<1> jevent(m_Cx);
    JS::RootedValue rval(m_Cx);

    JS::RootedObject event(m_Cx);

    ptr = static_cast<struct native_thread_msg *>(msg.dataPtr());

    memset(prop, 0, sizeof(prop));

    if (ev == NATIVE_THREAD_MESSAGE) {
        strcpy(prop, "onmessage");
    } else if (ev == NATIVE_THREAD_COMPLETE) {
        strcpy(prop, "oncomplete");
    }

    JS::RootedValue inval(m_Cx, JSVAL_NULL);

    if (!JS_ReadStructuredClone(m_Cx, ptr->data, ptr->nbytes,
        JS_STRUCTURED_CLONE_VERSION, &inval, NULL, NULL)) {

        printf("Failed to read input data (readMessage)\n");

        delete ptr;
        return;
    }
    JS::RootedObject callee(m_Cx, ptr->callee);
    if (JS_GetProperty(m_Cx, callee, prop, &jscbk) &&

        jscbk.isPrimitive() && JS_ObjectIsCallable(m_Cx, &jscbk.toObject())) {

        event = JS_NewObject(m_Cx, &messageEvent_class, JS::NullPtr(), JS::NullPtr());

        //JS_DefineProperty(JSContext *cx, JS::HandleObject obj, const char *name, JS::HandleValue value, unsigned int attrs)
        EVENT_PROP("data", inval);

        jevent[0].setObject(*event);

        JS_CallFunctionValue(m_Cx, event, jscbk, jevent, &rval);

    }
    JS_ClearStructuredClone(ptr->data, ptr->nbytes, NULL, NULL);

    delete ptr;
#undef EVENT_PROP
}

static bool native_Thread_constructor(JSContext *cx, unsigned argc, JS::Value *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject ret(cx, JS_NewObjectForConstructor(cx, &Thread_class, args));

    JS::RootedScript parent(cx);

    NativeJSThread *nthread = new NativeJSThread(ret, cx);
    JS::RootedFunction nfn(cx);

    if ((nfn = JS_ValueToFunction(cx, args[0])) == NULL ||
        (nthread->jsFunction = JS_DecompileFunction(cx, nfn, 0)) == NULL) {
        printf("Failed to read Threaded function\n");
        return true;
    }

    nthread->jsObject 	= ret;
    nthread->njs 		= NJS;

    JS::AutoFilename af;
    JS::DescribeScriptedCaller(cx, &af, &nthread->m_CallerLineno);

    nthread->m_CallerFileName = strdup(af.get());

    JS_SetPrivate(ret, nthread);

    args.rval().setObject(*ret);

    JS_DefineFunctions(cx, ret, Thread_funcs);

    return true;
}

void NativeJSThread::onComplete(JS::HandleValue vp)
{
    struct native_thread_msg *msg = new struct native_thread_msg;

    if (!JS_WriteStructuredClone(jsCx, vp, &msg->data, &msg->nbytes,
        NULL,
        NULL,
        JS::NullHandleValue)) {

        msg->data = NULL;
        msg->nbytes = 0;
    }

    msg->callee = jsObject;

    this->postMessage(msg, NATIVE_THREAD_COMPLETE);

    njs->unrootObject(jsObject);
}

static bool native_post_message(JSContext *cx, unsigned argc, JS::Value *vp)
{
    uint64_t *datap;
    size_t nbytes;

    NativeJSThread *nthread = (NativeJSThread *)JS_GetContextPrivate(cx);

    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    NATIVE_CHECK_ARGS("postMessage", 1);

    if (nthread == NULL || nthread->markedStop) {
        JS_ReportError(cx, "thread.send() Could not retrieve thread (or marked for stopping)");
        return false;
    }

    struct native_thread_msg *msg;

    if (!JS_WriteStructuredClone(cx, args[0], &datap, &nbytes,
        NULL, NULL, JS::NullHandleValue)) {
        JS_ReportError(cx, "Failed to write strclone");
        /* TODO: exception */
        return false;
    }

    msg = new struct native_thread_msg;

    msg->data   = datap;
    msg->nbytes = nbytes;
    msg->callee = nthread->jsObject;

    //nthread->njs->messages->postMessage(msg, NATIVE_THREAD_MESSAGE);
    nthread->postMessage(msg, NATIVE_THREAD_MESSAGE);

    return true;
}

NativeJSThread::~NativeJSThread()
{

    this->markedStop = true;
    if (this->jsRuntime) {
        JS_RequestInterruptCallback(this->jsRuntime);
        pthread_join(this->threadHandle, NULL);
    }

    if (m_CallerFileName) {
        free(m_CallerFileName);
    }
}

NativeJSThread::NativeJSThread(JS::HandleObject obj, JSContext *cx) :
    NativeJSExposer<NativeJSThread>(obj, cx),
    jsFunction(cx), jsRuntime(NULL), jsCx(NULL),
    jsObject(NULL), njs(NULL), markedStop(false), m_CallerFileName(NULL)
{
    /* cx hold the main context (caller) */
    /* jsCx hold the newly created context (along with jsRuntime) */
    cx = NULL;
}

NATIVE_OBJECT_EXPOSE(Thread)

