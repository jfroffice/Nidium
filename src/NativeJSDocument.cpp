#include "NativeJSDocument.h"
#include "NativeJS.h"
#include <jsapi.h>

#define NJSDOC_GETTER(obj) ((class NativeJSdocument *)JS_GetPrivate(obj))

static void Document_Finalize(JSFreeOp *fop, JSObject *obj);

static JSPropertySpec document_props[] = {
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static JSClass document_class = {
    "NativeDocument", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Document_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSClass *NativeJSdocument::jsclass = &document_class;

static void Document_Finalize(JSFreeOp *fop, JSObject *obj)
{
    NativeJSdocument *jdoc = NativeJSdocument::getNativeClass(obj);

    if (jdoc != NULL) {
        delete jdoc;
    }
}

bool NativeJSdocument::populateStyle(JSContext *cx, const char *data,
    size_t len, const char *filename)
{
    JS::Value ret;
    if (!NativeJS::LoadScriptReturn(cx, data, len, filename, &ret)) {
        return false;
    }
    JSObject *jret = ret.toObjectOrNull();

    NativeJS::copyProperties(cx, jret, this->stylesheet);

    return true;
}

void NativeJSdocument::registerObject(JSContext *cx)
{
    JSObject *documentObj;
    NativeJSdocument *jdoc = new NativeJSdocument();
    NativeJS *njs = NativeJS::getNativeClass(cx);

    documentObj = JS_DefineObject(cx, JS_GetGlobalObject(cx),
        NativeJSdocument::getJSObjectName(),
        &document_class , NULL, JSPROP_PERMANENT | JSPROP_ENUMERATE);

    JS_SetPrivate(documentObj, jdoc);
    jdoc->jsobj = documentObj;

    /* We have to root it since the user can replace the document object */
    njs->rootObjectUntilShutdown(documentObj);
    njs->jsobjects.set(NativeJSdocument::getJSObjectName(), documentObj);

    jdoc->stylesheet = JS_NewObject(cx, NULL, NULL, NULL);
    jsval obj = OBJECT_TO_JSVAL(jdoc->stylesheet);
    JS_SetProperty(cx, documentObj, "stylesheet", &obj);
    //JS_DefineFunctions(cx, documentObj, document_funcs);
    JS_DefineProperties(cx, documentObj, document_props);
}

