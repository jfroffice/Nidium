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

#ifndef nativejsexposer_h__
#define nativejsexposer_h__

#include <jsapi.h>
#include <jsfriendapi.h>
#include "NativeJS.h"
#include "NativeTaskManager.h"

template <typename T>
class NativeJSExposer
{
  public:
    JSObject *getJSObject() const {
        return m_JSObject;
    }

    JSObject **getJSObjectAddr()  {
        return &m_JSObject;
    }

    JSContext *getJSContext() const {
        return m_Cx;
    }

    void setJSObject(JSObject *obj) {
        m_JSObject = obj;
    }

    void setJSContext(JSContext *cx) {
        m_Cx = cx;
    }

    NativeJSExposer(JSObject *jsobj, JSContext *cx) {
        m_JSObject = jsobj;
        m_Cx = cx;
    }

    static const char *getJSObjectName() { return NULL; }

    static JSObject *getJSGlobalObject(NativeJS *njs) {
        JSObject *jobj;
        const char *name = T::getJSObjectName();

        if ((jobj = njs->jsobjects.get(name)) == NULL) {
            return NULL;
        }

        return jobj;
    }

    static JSObject *getJSGlobalObject(JSContext *cx) {
        return T::getJSGlobalObject(NativeJS::getNativeClass(cx));
    }

    static T* getNativeClass(JSObject *obj, JSContext *cx = NULL)
    {
        if (cx != NULL) {
            return (T *)JS_GetInstancePrivate(cx, obj, T::jsclass, NULL);
        }
        return (T *)JS_GetPrivate(obj);
    }

    static T* getNativeClass(NativeJS *njs) {
        JSObject *obj = T::getJSGlobalObject(njs);
        if (obj == NULL) {
            return NULL;
        }
        return (T *)JS_GetPrivate(obj);
    }

    static T* getNativeClass(JSContext *cx) {
        return T::getNativeClass(NativeJS::getNativeClass(cx));
    }
  protected:
    JSContext *m_Cx;
    JSObject *m_JSObject;
};

#define NATIVE_ASYNC_MAXCALLBACK 4
class NativeJSAsyncHandler : public NativeManaged
{
public:
    NativeJSAsyncHandler(JSContext *ctx) :
        m_Ctx(ctx) {
        memset(m_CallBack, 0, sizeof(m_CallBack));
    }

    virtual ~NativeJSAsyncHandler() {
        if (m_Ctx == NULL) {
            return;
        }

        for (int i = 0; i < NATIVE_ASYNC_MAXCALLBACK; i++) {
            if (m_CallBack[i] != NULL) {
                NativeJS::getNativeClass(m_Ctx)->unrootObject(m_CallBack[i]);
            }
        }
    }

    void setCallback(int idx, JSObject *callback)
    {
        if (idx >= NATIVE_ASYNC_MAXCALLBACK || m_Ctx == NULL) {
            return;
        }

        if (m_CallBack[idx] != NULL) {
            NativeJS::getNativeClass(m_Ctx)->unrootObject(m_CallBack[idx]);
        }

        if (callback) {
            NativeJS::getNativeClass(m_Ctx)->rootObjectUntilShutdown(callback);
        }
        m_CallBack[idx] = callback;
    }

    JSObject *getCallback(int idx) const {
        if (idx >= NATIVE_ASYNC_MAXCALLBACK || m_Ctx == NULL) {
            return NULL;
        }

        return m_CallBack[idx];
    }

    JSContext *getJSContext() const {
        return m_Ctx;
    }

    virtual void onMessage(const NativeSharedMessages::Message &msg)=0;
private:
    JSContext *m_Ctx;
    JSObject *m_CallBack[NATIVE_ASYNC_MAXCALLBACK];
};

/*  TODO: add a way to define whether object life define JSObject life
    (addroot/unroot or if jsobject life define obj life (finalizer))
*/

template <typename T>
class NativeJSObjectMapper
{
public:
    NativeJSObjectMapper(JSContext *cx, const char *name) :
        m_JSCx(cx)
    {
        static JSClass jsclass = {
            NULL, JSCLASS_HAS_PRIVATE,
            JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
        };

        if (jsclass.name == NULL) {
            jsclass.name = name;
        }

        m_JSClass = &jsclass;

        m_JSObj = JS_NewObject(m_JSCx, m_JSClass, NULL, NULL);
        JS_SetPrivate(m_JSObj, static_cast<T *>(this));
        JS_AddObjectRoot(m_JSCx, &m_JSObj);
    }
    virtual ~NativeJSObjectMapper()
    {
        JS_SetPrivate(m_JSObj, NULL);
        JS_RemoveObjectRoot(m_JSCx, &m_JSObj);
    }

    JSObject *getJSObject() const {
        return m_JSObj;
    }
    /*
        TODO : need to check against instance
    */
    static T *getObject(JSObject *jsobj) {
        return (T *)JS_GetPrivate(jsobj);
    }
protected:
    JSClass *m_JSClass;
    JSObject *m_JSObj;
    JSContext *m_JSCx;
};

typedef bool (*register_module_t)(JSContext *cx, JSObject *exports);

#define NativeJSObj(cx) (NativeJS::getNativeClass(cx))

#define NATIVE_OBJECT_EXPOSE(name) \
    void NativeJS ## name::registerObject(JSContext *cx) \
    { \
        JS_InitClass(cx, JS_GetGlobalObject(cx), NULL, &name ## _class, \
            native_ ## name ## _constructor, \
            0, NULL, NULL, NULL, NULL); \
    }

#define NATIVE_OBJECT_EXPOSE_NOT_INST(name) \
    void NativeJS ## name::registerObject(JSContext *cx) \
    { \
        JSObject *name ## Obj; \
        name ## Obj = JS_DefineObject(cx, JS_GetGlobalObject(cx), #name, \
            &name ## _class , NULL, 0); \
        JS_DefineFunctions(cx, name ## Obj, name ## _funcs); \
        JS_DefineProperties(cx, name ## Obj, name ## _props); \
    }

#define NATIVE_REGISTER_MODULE(constructor) \
    extern "C" bool __NativeRegisterModule(JSContext *cx, JSObject *exports) \
    { \
        return constructor(cx, exports); \
    }

#define NATIVE_CHECK_ARGS(fnname, minarg) \
    if (argc < minarg) { \
                         \
        char numBuf[12];  \
        snprintf(numBuf, sizeof numBuf, "%u", argc);  \
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,  \
                             fnname, numBuf, (argc > 1 ? "s" : ""));  \
        return false;  \
    }


#define JSOBJ_SET_PROP_FLAGS(where, name, val, flags) JS_DefineProperty(m_Cx, where, \
    (const char *)name, val, NULL, NULL, flags)

#define JSOBJ_SET_PROP(where, name, val) JSOBJ_SET_PROP_FLAGS(where, name, val, \
        JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)

#define JSOBJ_CALLFUNCNAME(where, name, argc, argv) \
    { \
        JS::Value oncallback, rval; \
        if (JS_GetProperty(cx, where, name, &oncallback) && \
            JS_TypeOfValue(cx, oncallback) == JSTYPE_FUNCTION) { \
            JS_CallFunctionValue(cx, where, oncallback, \
                argc, argv, &rval); \
        } \
    }
#define JSOBJ_SET_PROP_CSTR(where, name, val) JSOBJ_SET_PROP(where, name, STRING_TO_JSVAL(JS_NewStringCopyZ(m_Cx, val)))
#define JSOBJ_SET_PROP_STR(where, name, val) JSOBJ_SET_PROP(where, name, STRING_TO_JSVAL(val))
#define JSOBJ_SET_PROP_INT(where, name, val) JSOBJ_SET_PROP(where, name, INT_TO_JSVAL(val))

#define JSNATIVE_PROLOGUE(ofclass) \
    JS::CallArgs args = CallArgsFromVp(argc, vp); \
    JS::RootedObject thisobj(cx, JS_THIS_OBJECT(cx, vp)); \
    ofclass *CppObj = (ofclass *)JS_GetPrivate(thisobj);

#define JSNATIVE_PROLOGUE_CLASS(ofclass, fclass) \
    JS::CallArgs args = CallArgsFromVp(argc, vp); \
    (void)args;\
    JS::RootedObject thisobj(cx, JS_THIS_OBJECT(cx, vp)); \
    if (!thisobj) { \
        JS_ReportError(cx, "Illegal invocation"); \
        return false; \
    } \
    ofclass *CppObj = (ofclass *)JS_GetInstancePrivate(cx, thisobj, fclass, NULL); \
    if (!CppObj) { \
        JS_ReportError(cx, "Illegal invocation"); \
        return false; \
    }

#define JS_INITOPT() JS::Value __curopt;

#define JSGET_OPT(obj, name) if (obj && JS_GetProperty(cx, obj, name, &__curopt) && __curopt != JSVAL_VOID && __curopt != JSVAL_NULL)
#define JSGET_OPT_TYPE(obj, name, type) if (obj && JS_GetProperty(cx, obj, name, &__curopt) && __curopt != JSVAL_VOID && __curopt != JSVAL_NULL && __curopt.is ## type())

#endif
