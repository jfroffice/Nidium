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

template <typename T>
class NativeJSExposer
{
  public:
    JSContext *cx;
    JSObject *getJSObject() const {
        return this->jsobj;
    }
    JSObject *jsobj;

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

#define JSOBJ_SET_PROP(where, name, val) JS_DefineProperty(cx, where, \
    (const char *)name, val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | \
        JSPROP_ENUMERATE)

#define JSNATIVE_PROLOGUE(ofclass) \
    JS::CallArgs args = CallArgsFromVp(argc, vp); \
    JS::RootedObject thisobj(cx, JS_THIS_OBJECT(cx, vp)); \
    ofclass *CppObj = (ofclass *)JS_GetPrivate(thisobj);

#endif
