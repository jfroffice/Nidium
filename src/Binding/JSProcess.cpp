/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "Binding/JSProcess.h"

#include <ape_netlib.h>

#include <pwd.h>
#include <grp.h>

#include "Core/Path.h"
#include "Binding/JSUtils.h"

using Nidium::Core::Path;
using Nidium::Binding::JSUtils;

namespace Nidium {
namespace Binding {

// {{{ Preamble

static void Process_Finalize(JSFreeOp *fop, JSObject *obj);
static bool nidium_process_getowner(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_process_setowner(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_process_setSignalHandler(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_process_exit(JSContext *cx, unsigned argc, JS::Value *vp);
static bool nidium_process_cwd(JSContext *cx, unsigned argc, JS::Value *vp);

static JSClass Process_class = {
    "NidiumProcess", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Process_Finalize,
    nullptr, nullptr, nullptr, nullptr, JSCLASS_NO_INTERNAL_MEMBERS
};

JSClass *JSProcess::jsclass = &Process_class;

template<>
JSClass *JSExposer<JSProcess>::jsclass = &Process_class;

static JSFunctionSpec Process_funcs[] = {
    JS_FN("getOwner", nidium_process_getowner, 1, NIDIUM_JS_FNPROPS),
    JS_FN("setOwner", nidium_process_setowner, 1, NIDIUM_JS_FNPROPS),
    JS_FN("setSignalHandler", nidium_process_setSignalHandler, 1, NIDIUM_JS_FNPROPS),
    JS_FN("exit", nidium_process_exit, 1, NIDIUM_JS_FNPROPS),
    JS_FN("cwd", nidium_process_cwd, 0, NIDIUM_JS_FNPROPS),
    JS_FS_END
};

static int ape_kill_handler(int code, ape_global *ape)
{
    NidiumJS *njs = NidiumJS::GetObject();
    JSContext *cx = njs->m_Cx;
    JS::RootedValue     rval(cx);

    JSProcess *jProcess = JSProcess::GetObject(njs);

    JS::RootedValue func(cx, jProcess->m_SignalFunction);

    if (func.isObject() && JS_ObjectIsCallable(cx, func.toObjectOrNull())) {
        JS_CallFunctionValue(cx, JS::NullPtr(), func, JS::HandleValueArray::empty(), &rval);

        return rval.isBoolean() ? !rval.toBoolean() : false;
    }

    return false;
}

// }}}

// {{{ Implementation
static bool nidium_process_getowner(JSContext *cx, unsigned argc, JS::Value *vp)
{
    int uid = getuid();
    int gid = getgid();
    struct passwd *userInfo = getpwuid(uid);
    struct group *groupInfo = getgrgid(gid);

    NIDIUM_JS_PROLOGUE_CLASS_NO_RET(JSProcess, &Process_class);

    if (!userInfo || !groupInfo) {
        JS_ReportError(cx, "Failed to retrieve process owner");
        return false;
    }

    JS::RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));

    JS::RootedString userStr(cx, JS_NewStringCopyZ(cx, userInfo->pw_name));
    JS::RootedString groupStr(cx, JS_NewStringCopyZ(cx, groupInfo->gr_name));

    JS_DefineProperty(cx, obj, "uid", uid, 
            JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE);
    JS_DefineProperty(cx, obj, "gid", gid, 
            JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE);
    JS_DefineProperty(cx, obj, "user", userStr, 
            JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE);
    JS_DefineProperty(cx, obj, "group", groupStr, 
            JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE);

    JS::RootedValue val(cx);
    val.setObjectOrNull(obj);

    args.rval().set(val);

    return true;
}

static bool nidium_process_setowner(JSContext *cx, unsigned argc, JS::Value *vp)
{
    struct passwd *userInfo = nullptr;
    struct group *groupInfo = nullptr;

    NIDIUM_JS_PROLOGUE_CLASS_NO_RET(JSProcess, &Process_class);
    NIDIUM_JS_CHECK_ARGS("setOwner", 1);

    if (args[0].isNumber()) {
        int uid = args[0].toInt32();

        userInfo = getpwuid(uid);

        if (userInfo == nullptr) {
            if (errno == 0) {
                JS_ReportError(cx,
                        "User ID \"%d\" not found", uid);
            } else {
                JS_ReportError(cx,
                        "Error retrieving user ID \"%d\" error : %s", 
                        uid, strerror(errno));
            }
            return false;
        }
    } else if (args[0].isString()) {
        JS::RootedString user(cx, args[0].toString());
        JSAutoByteString cuser;
        cuser.encodeUtf8(cx, user);

        userInfo = getpwnam(cuser.ptr());

        if (userInfo == nullptr) {
            if (errno == 0) {
                JS_ReportError(cx,
                        "User name \"%s\" not found", cuser.ptr());
            } else {
                JS_ReportError(cx,
                        "Error retrieving user name \"%s\" error : %s", 
                        cuser.ptr(), strerror(errno));
            }
            return false;
        }
    } else {
        JS_ReportError(cx, "Invalid first argument (Number or String expected)");
        return false;
    }

    if (argc > 1 && args[1].isNumber()) {
        int gid = args[0].toInt32();
    
        groupInfo = getgrgid(gid);

        if (groupInfo == nullptr) {
            if (errno == 0) {
                JS_ReportError(cx,
                        "Group ID \"%d\" not found", gid);
            } else {
                JS_ReportError(cx,
                        "Error retrieving group ID \"%d\" error : %s", 
                        gid, strerror(errno));
            }
            return false;
        }
    } else if (argc > 1) {
        JS::RootedString group(cx, args[1].toString());
        JSAutoByteString cgroup;

        cgroup.encodeUtf8(cx, group);
        groupInfo = getgrnam(cgroup.ptr());

        if (groupInfo == nullptr) {
            if (errno == 0) {
                JS_ReportError(cx,
                        "Group name \"%s\" not found", cgroup.ptr());
            } else {
                JS_ReportError(cx,
                        "Error retrieving group name \"%s\" error : %s", 
                        cgroup.ptr(), strerror(errno));
            }
            return false;
        }
    }

    /* 
        When dropping privileges from root, the setgroups call will
        remove any extraneous groups. If we don't call this, then
        even though our uid has dropped, we may still have groups
        that enable us to do super-user things. This will fail if we
        aren't root, so don't bother checking the return value, this
        is just done as an optimistic privilege dropping function.
    */
    setgroups(0, NULL);

    if (groupInfo != nullptr && setgid(groupInfo->gr_gid) != 0) {
        JS_ReportError(cx, "Failed to set group ID to \"%d\" error : %s", 
                groupInfo->gr_gid,
                strerror(errno));
        return false;
    }

    if (setuid(userInfo->pw_uid) != 0) {
        JS_ReportError(cx, "Failed to set user ID to \"%d\", error : %s", 
                userInfo->pw_uid,
                strerror(errno));
        return false;
    }

    if (groupInfo != nullptr) {
        if (initgroups(userInfo->pw_name, groupInfo->gr_gid) == 0) {
            JS_ReportError(cx, 
                    "Failed to initialize supplementary group access list. Error : %s",
                    strerror(errno));
            return false;
        }
    }

    return true;
}

static bool nidium_process_setSignalHandler(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSProcess, &Process_class);
    NIDIUM_JS_CHECK_ARGS("setSignalHandler", 1);

    JS::RootedValue func(cx);

    if (!JS_ConvertValue(cx, args[0], JSTYPE_FUNCTION, &func)) {
        JS_ReportWarning(cx, "setSignalHandler: bad callback");
        return true;
    }

    CppObj->m_SignalFunction = func;

    return true;
}

static bool nidium_process_exit(JSContext *cx, unsigned argc, JS::Value *vp)
{
    NIDIUM_JS_PROLOGUE_CLASS(JSProcess, &Process_class);

    int code = 0;

    if (argc > 0 && args[0].isInt32()) {
        code = args[0].toInt32();
    }

    quick_exit(code);

    return true;
}

static bool nidium_process_cwd(JSContext *cx, unsigned argc, JS::Value *vp)
{
    Path cur(Path::GetCwd());
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    if (cur.dir() == NULL) {
        args.rval().setUndefined();
        return true;
    }

    JS::RootedString res(cx, JS_NewStringCopyZ(cx, cur.dir()));

    args.rval().setString(res);

    return true;
}


static void Process_Finalize(JSFreeOp *fop, JSObject *obj)
{
    JSProcess *jProcess = JSProcess::GetObject(obj);

    if (jProcess != NULL) {
        delete jProcess;
    }
}

// }}}

// {{{ Registration

void JSProcess::RegisterObject(JSContext *cx, char **argv, int argc, int workerId)
{
    NidiumJS *njs = NidiumJS::GetObject(cx);
    JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
    JS::RootedObject ProcessObj(cx, JS_DefineObject(cx, global, JSProcess::GetJSObjectName(),
        &Process_class , NULL, JSPROP_PERMANENT | JSPROP_ENUMERATE | JSPROP_READONLY));

    JSProcess *jProcess = new JSProcess(ProcessObj, cx);

    JS_SetPrivate(ProcessObj, jProcess);

    njs->m_JsObjects.set(JSProcess::GetJSObjectName(), ProcessObj);

    JS_DefineFunctions(cx, ProcessObj, Process_funcs);

    JS::RootedObject jsargv(cx, JS_NewArrayObject(cx, argc));

    for (int i = 0; i < argc; i++) {
        JS::RootedString jelem(cx, JS_NewStringCopyZ(cx, argv[i]));
        JS_SetElement(cx, jsargv, i, jelem);
    }

    JS::RootedValue jsargv_v(cx, OBJECT_TO_JSVAL(jsargv));
    JS_SetProperty(cx, ProcessObj, "argv", jsargv_v);

    JS::RootedValue workerid_v(cx, JS::Int32Value(workerId));
    JS_SetProperty(cx, ProcessObj, "workerId", workerid_v);

    NidiumJS::GetNet()->kill_handler = ape_kill_handler;
    jProcess->m_SignalFunction.set(JS::NullHandleValue);

}

// }}}

} // namespace Binding
} // namespace Nidium

