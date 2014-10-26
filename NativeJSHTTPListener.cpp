/*
    NativeJS Core Library
    Copyright (C) 2014 Anthony Catel <paraboul@gmail.com>

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

#include "NativeJSHTTPListener.h"
#include "NativeJSUtils.h"

static void HTTPListener_Finalize(JSFreeOp *fop, JSObject *obj);

static JSBool native_httpresponse_write(JSContext *cx,
    unsigned argc, jsval *vp);
static JSBool native_httpresponse_end(JSContext *cx,
    unsigned argc, jsval *vp);
static JSBool native_httpresponse_writeHead(JSContext *cx,
    unsigned argc, jsval *vp);

static JSClass HTTPListener_class = {
    "HTTPListener", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, HTTPListener_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass HTTPRequest_class = {
    "HTTPRequest", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

/*
    TODO: write is for response
*/
static JSFunctionSpec HTTPResponse_funcs[] = {
    JS_FN("write", native_httpresponse_write, 1, 0),
    JS_FN("end", native_httpresponse_end, 0, 0),
    JS_FN("writeHead", native_httpresponse_writeHead, 1, 0),
    JS_FS_END
};

static JSPropertySpec HTTPRequest_props[] = {
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

static void HTTPListener_Finalize(JSFreeOp *fop, JSObject *obj)
{
    NativeJSHTTPListener *server = (NativeJSHTTPListener *)JS_GetPrivate(obj);

    if (server != NULL) {
        delete server;
    }
}

NativeJSHTTPResponse::NativeJSHTTPResponse(JSContext *cx, uint16_t code) :
        NativeHTTPResponse(code),
        NativeJSObjectMapper(cx, "HTTPResponse")
{
    JS_DefineFunctions(cx, m_JSObj, HTTPResponse_funcs);
}

NativeJSHTTPListener::NativeJSHTTPListener(uint16_t port, const char *ip) :
    NativeHTTPListener(port, ip)

{

}

NativeJSHTTPListener::~NativeJSHTTPListener()
{

}


void NativeJSHTTPListener::onClientDisconnect(NativeHTTPClientConnection *client)
{

}

void NativeJSHTTPListener::onData(NativeHTTPClientConnection *client,
    const char *buf, size_t len)
{
    // on progress
}

bool NativeJSHTTPListener::onEnd(NativeHTTPClientConnection *client)
{
    JS::Value oncallback, rval;
    buffer *k, *v;
    NativeJSHTTPClientConnection *subclient = static_cast<NativeJSHTTPClientConnection *>(client);

    JSObject *objrequest = JS_NewObject(cx, &HTTPRequest_class, NULL, NULL);
    JSObject *headers = JS_NewObject(cx, NULL, NULL, NULL);

    if (client->getHTTPState()->headers.list) {
        APE_A_FOREACH(client->getHTTPState()->headers.list, k, v) {
            JSString *jstr = JS_NewStringCopyN(cx, (char *)v->data,
                v->used-1);
            JSOBJ_SET_PROP_FLAGS(headers, k->data,
                STRING_TO_JSVAL(jstr), JSPROP_ENUMERATE);
        }
    }

    buffer *url = client->getHTTPState()->url;
    if (url) {
        JSString *jsurl = JS_NewStringCopyN(cx, (char *)url->data, url->used);
        JSOBJ_SET_PROP(objrequest, "url", STRING_TO_JSVAL(jsurl));
    }

    buffer *data = client->getHTTPState()->data;
    if (client->getHTTPState()->parser.method == HTTP_POST) {
        JS::Value strdata;
        if (data == NULL || data->used == 0) {
            strdata = JS_GetEmptyStringValue(cx);
        } else {
            NativeJSUtils::strToJsval(cx, (const char *)data->data,
                data->used, &strdata, "utf8");
        }

        JSOBJ_SET_PROP(objrequest, "data", strdata);
    }

    JS::Value method;
    switch (client->getHTTPState()->parser.method) {
        case HTTP_POST:
            method.setString(JS_NewStringCopyN(cx, CONST_STR_LEN("POST")));
            break;
        case HTTP_GET:
            method.setString(JS_NewStringCopyN(cx, CONST_STR_LEN("GET")));
            break;
        case HTTP_PUT:
            method.setString(JS_NewStringCopyN(cx, CONST_STR_LEN("PUT")));
            break;
        default:
            method.setString(JS_NewStringCopyN(cx, CONST_STR_LEN("UNKOWN")));
            break;
    }

    JSOBJ_SET_PROP(objrequest, "method", method);
    JSOBJ_SET_PROP(objrequest, "headers", OBJECT_TO_JSVAL(headers));
    JSOBJ_SET_PROP(objrequest, "client", OBJECT_TO_JSVAL(subclient->getJSObject()));

    if (JS_GetProperty(cx, this->jsobj, "onrequest", &oncallback) &&
        JS_TypeOfValue(cx, oncallback) == JSTYPE_FUNCTION) {
        JS::Value arg[2];
        arg[0].setObjectOrNull(objrequest);
        arg[1].setObjectOrNull(static_cast<NativeJSHTTPResponse*>(client->getResponse())->getJSObject());
        JS_CallFunctionValue(cx, this->jsobj, oncallback,
            2, arg, &rval);
    }

    return false;
}

static JSBool native_HTTPListener_constructor(JSContext *cx,
    unsigned argc, jsval *vp)
{
    uint16_t port;
    JSString *ip_bind = NULL;
    JSBool reuseport = false;
    NativeJSHTTPListener *listener;

    if (!JS_IsConstructing(cx, vp)) {
        JS_ReportError(cx, "Bad constructor");
        return false;
    }

    JSObject *ret = JS_NewObjectForConstructor(cx, &HTTPListener_class, vp);

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "c/bS",
        &port, &reuseport, &ip_bind)) {
        return false;
    }

    if (ip_bind) {
        JSAutoByteString cip(cx, ip_bind);
        listener = new NativeJSHTTPListener(port, cip.ptr());
    } else {
        listener = new NativeJSHTTPListener(port);
    }

    if (!listener->start((bool)reuseport)) {
        JS_ReportError(cx, "HTTPListener() couldn't listener on %d", port);
        delete listener;
        return false;
    }

    listener->cx = cx;
    listener->jsobj = ret;

    JS_SetPrivate(ret, listener);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(ret));

    NativeJSObj(cx)->rootObjectUntilShutdown(ret);

    return true;
}

static JSBool native_HTTPRequest_class_constructor(JSContext *cx,
    unsigned argc, jsval *vp)
{
    JS_ReportError(cx, "Illegal constructor");
    return false;
}

static JSBool native_httpresponse_write(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject caller(cx, JS_THIS_OBJECT(cx, vp));

    NATIVE_CHECK_ARGS("write", 1);

    NativeJSHTTPResponse *resp = NativeJSHTTPResponse::getObject(caller);
    if (!resp) {
        return true;
    }

    if (args[0].isString()) {
        JSAutoByteString jsdata;
        jsdata.encodeUtf8(cx, args[0].toString());

        resp->sendChunk(jsdata.ptr(), jsdata.length(), APE_DATA_COPY);

    }  else if (args[0].isObject()) {
        JSObject *objdata = args[0].toObjectOrNull();
        if (!objdata || !JS_IsArrayBufferObject(objdata)) {
            JS_ReportError(cx, "write() invalid data (must be either a string or an ArrayBuffer)");
            return false;            
        }
        uint32_t len = JS_GetArrayBufferByteLength(objdata);
        uint8_t *data = JS_GetArrayBufferData(objdata);

        resp->sendChunk((char *)data, len, APE_DATA_COPY);
    } else {
        JS_ReportError(cx, "write() only accepts String or ArrayBuffer");
        return false;
    }
    
    return true;
}

static JSBool native_httpresponse_end(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject caller(cx, JS_THIS_OBJECT(cx, vp));

    NativeJSHTTPResponse *resp = NativeJSHTTPResponse::getObject(caller);
    if (!resp) {
        return true;
    }

    if (args.length() > 0) {
        if (args[0].isString()) {
            JSAutoByteString jsdata;
            jsdata.encodeUtf8(cx, args[0].toString());
            
            resp->sendChunk(jsdata.ptr(), jsdata.length(), APE_DATA_COPY, true);
        } else if (args[0].isObject()) {
            JSObject *objdata = args[0].toObjectOrNull();
            if (!objdata || !JS_IsArrayBufferObject(objdata)) {
                JS_ReportError(cx, "end() invalid data (must be either a string or an ArrayBuffer)");
                return false;            
            }
            uint32_t len = JS_GetArrayBufferByteLength(objdata);
            uint8_t *data = JS_GetArrayBufferData(objdata);

            resp->sendChunk((char *)data, len, APE_DATA_COPY, true);
        }
    }

    resp->end();

    return true;
}

static JSBool native_httpresponse_writeHead(JSContext *cx, unsigned argc, jsval *vp)
{
    uint16_t statuscode;
    JSObject *headers = NULL;

    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject caller(cx, JS_THIS_OBJECT(cx, vp));

    if (!JS_ConvertArguments(cx, args.length(), args.array(), "c/o",
        &statuscode, &headers)) {
        return false;
    }

    NativeJSHTTPResponse *resp = NativeJSHTTPResponse::getObject(caller);
    if (!resp) {
        return true;
    }

    if (resp->isHeadersAlreadySent()) {
        return true;
    }

    resp->setStatusCode(statuscode);

    if (args.length() >= 2 && !args[1].isPrimitive()) {

        JS::RootedId idp(cx);
        JS::RootedObject iterator(cx);

        iterator = JS_NewPropertyIterator(cx, headers);

        while (JS_NextProperty(cx, iterator, idp.address()) && !JSID_IS_VOID(idp)) {
            if (!JSID_IS_STRING(idp)) {
                continue;
            }
            JSString *key = JSID_TO_STRING(idp);
            JS::Value val;

            if (!JS_GetPropertyById(cx, headers, idp, &val) || !val.isString()) {
                continue;
            }

            JSAutoByteString ckey(cx, key);
            JSAutoByteString cval(cx, val.toString());

            resp->setHeader(ckey.ptr(), cval.ptr());
        }
    }

    resp->sendHeaders(true);

    return true;
}

void NativeJSHTTPListener::registerObject(JSContext *cx)
{
    JS_InitClass(cx, JS_GetGlobalObject(cx), NULL, &HTTPListener_class,
        native_HTTPListener_constructor,
        0, NULL, NULL, NULL, NULL);
/*
    TODO: how to init a class from a NativeJSObjectMapper derived class
*/
    /*JS_InitClass(cx, JS_GetGlobalObject(cx), NULL, &HTTPRequest_class,
                native_HTTPRequest_class_constructor,
                0, HTTPRequest_props, HTTPRequest_funcs, NULL, NULL);*/
}
