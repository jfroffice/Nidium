/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "Binding/JSConsole.h"

#include <string.h>
#include <math.h>

#if 0
#ifdef NIDIUM_PRODUCT_SERVER
#include "Server/Context.h"
#include "Server/Server.h"

using Nidium::Server::Context;
using Nidium::Server::Worker;
#endif
#endif

namespace Nidium {
namespace Binding {

// {{{ Preamble

static bool nidium_console_log(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_console_write(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_console_hide(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_console_show(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_console_clear(JSContext *cx, unsigned argc, JS::Value *vp);

static JSClass console_class = {
    "Console", 0,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSFunctionSpec console_funcs[] = {
    JS_FN("log", nidium_console_log, 0, NIDIUM_JS_FNPROPS),
    JS_FN("write", nidium_console_write, 0, NIDIUM_JS_FNPROPS),
    JS_FN("info", nidium_console_log, 0, NIDIUM_JS_FNPROPS),
    JS_FN("error", nidium_console_log, 0, NIDIUM_JS_FNPROPS),
    JS_FN("warn", nidium_console_log, 0, NIDIUM_JS_FNPROPS),
    JS_FN("hide", nidium_console_hide, 0, NIDIUM_JS_FNPROPS),
    JS_FN("show", nidium_console_show, 0, NIDIUM_JS_FNPROPS),
    JS_FN("clear", nidium_console_clear, 0, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

// }}}

// {{{ Implementation

static bool nidium_console_hide(JSContext *cx, unsigned argc,
    JS::Value *vp)
{

#if 0
#ifdef NIDIUM_PRODUCT_UI
    if (NativeContext::GetObject(cx)->getUI()->getConsole()) {
        NativeContext::GetObject(cx)->getUI()->getConsole()->hide();
    }
#endif
#endif
    return true;
}

static bool nidium_console_show(JSContext *cx, unsigned argc,
    JS::Value *vp)
{
#if 0
#ifdef NIDIUM_PRODUCT_UI
    NativeContext::GetObject(cx)->getUI()->getConsole(true)->show();
#endif
#endif
    return true;
}

static bool nidium_console_clear(JSContext *cx, unsigned argc,
    JS::Value *vp)
{
    NidiumJS *js = NidiumJS::GetObject(cx);

    js->logclear();
#if 0
#ifdef NIDIUM_PRODUCT_UI
    if (NativeContext::GetObject(cx)->getUI()->getConsole()) {
        NativeContext::GetObject(cx)->getUI()->getConsole()->clear();
    }
#endif
#endif
    return true;
}

static bool nidium_console_log(JSContext *cx, unsigned argc,
    JS::Value *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    unsigned i;
    char *bytes;
    const char *filename_parent;
    unsigned lineno;

    JS::AutoFilename filename;
    JS::DescribeScriptedCaller(cx, &filename, &lineno);

    NidiumJS *js = NidiumJS::GetObject(cx);
    filename_parent = filename.get();
    if (filename_parent == NULL) {
        filename_parent = "(null)";
    }

    const char *fname = strrchr(filename_parent, '/');
    if (fname != NULL) {
        filename_parent = &fname[1];
    }

    for (i = 0; i < args.length(); i++) {

        JS::RootedString str(cx, JS::ToString(cx, args[i]));
        if (!str)
            return false;
        bytes = JS_EncodeStringToUTF8(cx, str);
        if (!bytes)
            return false;
        if (i) {
            js->log(" ");
        } else {
#if 0
#ifdef NIDIUM_PRODUCT_SERVER
            Context *nctx = Context::GetObject(cx);

            if (!nctx->isREPL()) {
                js->logf("(worker %d) [%s:%d] ", nctx->getWorker()->getIdentifier(), filename_parent, lineno);
            }
#else
            js->logf("[%s:%d] ", filename_parent, lineno);
#endif
#endif
        }
        js->log(bytes);

        JS_free(cx, bytes);
    }
    js->log("\n");

    args.rval().setUndefined();

    return true;
}

static bool nidium_console_write(JSContext *cx, unsigned argc,
    JS::Value *vp)
{
    NidiumJS *js = NidiumJS::GetObject(cx);
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    NIDIUM_JS_CHECK_ARGS("write", 1);

    JS::RootedString str(cx, args[0].toString());
    if (!str) {
        JS_ReportError(cx, "Bad argument");
        return false;
    }

    JSAutoByteString cstr;

    cstr.encodeUtf8(cx, str);

    js->log(cstr.ptr());

    args.rval().setUndefined();
    return true;
}
// }}}

// {{{ Registration
void JSConsole::RegisterObject(JSContext *cx)
{
    JS::RootedObject consoleObj(cx, JS_DefineObject(cx,
        JS::CurrentGlobalOrNull(cx), "console",
        &console_class, NULL, 0));

    JS_DefineFunctions(cx, consoleObj, console_funcs);
}
// }}}

} // namespace Binding
} // namespace Nidium

