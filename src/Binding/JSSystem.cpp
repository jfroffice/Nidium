/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "Binding/JSSystem.h"

#if 0
#ifdef NIDIUM_PRODUCT_UI
#include <SystemInterface.h>
#endif
#endif


#include <sys/stat.h>

#ifdef __linux__
  #include <sys/time.h>
  #include <sys/resource.h>
  #include <linux/stat.h>
#endif

namespace Nidium {
namespace Binding {

// {{{ Preamble
static bool nidium_system_getOpenFileStats(JSContext *cx, unsigned argc,
    JS::Value *vp);

static JSClass system_class = {
    "System", 0,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

static JSFunctionSpec system_funcs[] = {
    JS_FN("getOpenFileStats", nidium_system_getOpenFileStats, 0, NIDIUM_JS_FNPROPS),
#if 0
#ifdef NIDIUM_PRODUCT_UI
    JS_FN("language", nidium_nidium_language, 0, NIDIUM_JS_FNPROPS),
#endif
#endif
    JS_FS_END
};
// }}}

// {{{ Implementation
static bool nidium_system_getOpenFileStats(JSContext *cx, unsigned argc,
    JS::Value *vp)
{
    struct rlimit rl;
    struct stat   stats;
    JSContext *m_Cx = cx;

    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
        JS_ReportWarning(cx, "Couldnt get limits values");
        args.rval().setNull();
        return true;
    }

    JS::RootedObject ret(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));

    NIDIUM_JSOBJ_SET_PROP_INT(ret, "cur", (int) rl.rlim_cur);
    NIDIUM_JSOBJ_SET_PROP_INT(ret, "max", (int) rl.rlim_max);

    int fdcounter = 0, sockcounter = 0, othercount = 0;

    for (size_t i = 0; i <= rl.rlim_cur; i++) {
        if (fstat(i, &stats) == 0) {
            fdcounter++;
            if ((stats.st_mode & S_IFMT) == S_IFSOCK) {
                sockcounter++;
            } else {
                othercount++;
            }
        } else if (errno != EBADF) {
            fdcounter++;
        }

    }

    NIDIUM_JSOBJ_SET_PROP_INT(ret, "open", fdcounter);
    NIDIUM_JSOBJ_SET_PROP_INT(ret, "sockets", sockcounter);
    NIDIUM_JSOBJ_SET_PROP_INT(ret, "files", othercount);

    args.rval().setObjectOrNull(ret);

    return true;
}

#if 0
#ifdef NIDIUM_PRODUCT_UI
static bool nidium_nidium_language(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    SystemInterface* interface = SystemInterface::GetInstance();
    const char *clang = interface->getLanguage();

    args.rval().setString(JS_NewStringCopyZ(cx, clang));

    return true;
}
#endif
#endif
// }}}

// {{{ Registration
void JSSystem::RegisterObject(JSContext *cx)
{
    JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
    JS::RootedObject systemObj(cx, JS_DefineObject(cx, global,
        "System", &system_class , nullptr, 0));
    JS_DefineFunctions(cx, systemObj, system_funcs);
}
// }}}

} // namespace Binding
} // namespce Nidium

