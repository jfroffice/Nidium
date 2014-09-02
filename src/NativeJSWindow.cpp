#include "NativeJSWindow.h"
#include "NativeJS.h"
#include "NativeUIInterface.h"
#include "NativeSkia.h"
#include "NativeUtils.h"
#include "NativeContext.h"
#include "NativeJSNative.h"
#include "NativeJSCanvas.h"
#include "NativeDB.h"
#include "NativeNML.h"
#include "NativeMacros.h"
#include "NativeJSUtils.h"
#include "NativeJSImage.h"

#include <NativeSystemInterface.h>

#include <NativeJSFileIO.h>

static JSBool native_window_prop_set(JSContext *cx, JSHandleObject obj,
    JSHandleId id, JSBool strict, JSMutableHandleValue vp);
static JSBool native_window_prop_get(JSContext *cx, JSHandleObject obj,
    JSHandleId id, JSMutableHandleValue vp);

static JSBool native_window_openFileDialog(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_openDirDialog(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_setSize(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_requestAnimationFrame(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_center(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_setPosition(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_setFrame(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_notify(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_quit(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_close(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_open(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_window_setSystemTray(JSContext *cx, unsigned argc, jsval *vp);

static JSBool native_storage_set(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_storage_get(JSContext *cx, unsigned argc, jsval *vp);


static void Window_Finalize(JSFreeOp *fop, JSObject *obj);
static void Storage_Finalize(JSFreeOp *fop, JSObject *obj);

enum {
    WINDOW_PROP_LEFT,
    WINDOW_PROP_TOP,
    WINDOW_PROP_WIDTH,
    WINDOW_PROP_HEIGHT,
    WINDOW_PROP_TITLE,
    WINDOW_PROP_CURSOR,
    WINDOW_PROP_TITLEBAR_COLOR,
    WINDOW_PROP_TITLEBAR_CONTROLS_OFFSETX,
    WINDOW_PROP_TITLEBAR_CONTROLS_OFFSETY,
    WINDOW_PROP_DEVICE_PIXELRATIO
};

static JSClass window_class = {
    "Window", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Window_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass storage_class = {
    "NidiumStorage", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Storage_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSClass *NativeJSwindow::jsclass = &window_class;

static JSClass mouseEvent_class = {
    "MouseEvent", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass dragEvent_class = {
    "DragEvent", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

#if 0
static JSClass windowEvent_class = {
    "WindowEvent", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
#endif

static JSClass textEvent_class = {
    "TextInputEvent", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass keyEvent_class = {
    "keyEvent", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass NMLEvent_class = {
    "NMLEvent", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};


static JSFunctionSpec storage_funcs[] = {
    JS_FN("set", native_storage_set, 2, 0),
    JS_FN("get", native_storage_get, 1, 0),
    JS_FS_END
};

static JSFunctionSpec window_funcs[] = {
    JS_FN("openFileDialog", native_window_openFileDialog, 2, 0),
    JS_FN("openDirDialog", native_window_openDirDialog, 1, 0),
    JS_FN("setSize", native_window_setSize, 2, 0),
    JS_FN("requestAnimationFrame", native_window_requestAnimationFrame, 1, 0),
    JS_FN("center", native_window_center, 0, 0),
    JS_FN("setPosition", native_window_setPosition, 2, 0),
    JS_FN("setFrame", native_window_setFrame, 4, 0),
    JS_FN("notify", native_window_notify, 2, 0),
    JS_FN("quit", native_window_quit, 0, 0),
    JS_FN("close", native_window_close, 0, 0),
    JS_FN("open", native_window_open, 0, 0),
    JS_FN("setSystemTray", native_window_setSystemTray, 1, 0),
    JS_FS_END
};

static struct native_cursors {
    const char *str;
    NativeUIInterface::CURSOR_TYPE type;
} native_cursors_list[] = {
    {"default",             NativeUIInterface::ARROW},
    {"arrow",               NativeUIInterface::ARROW},
    {"beam",                NativeUIInterface::BEAM},
    {"text",                NativeUIInterface::BEAM},
    {"pointer",             NativeUIInterface::POINTING},
    {"grabbing",            NativeUIInterface::CLOSEDHAND},
    {"drag",                NativeUIInterface::CLOSEDHAND},
    {"hidden",              NativeUIInterface::HIDDEN},
    {"none",                NativeUIInterface::HIDDEN},
    {"col-resize",          NativeUIInterface::RESIZELEFTRIGHT},
    {NULL,                  NativeUIInterface::NOCHANGE},
};

static JSPropertySpec window_props[] = {
    {"devicePixelRatio", WINDOW_PROP_DEVICE_PIXELRATIO, JSPROP_PERMANENT | JSPROP_ENUMERATE | JSPROP_READONLY,
        JSOP_WRAPPER(native_window_prop_get),
        JSOP_NULLWRAPPER},
    {"left", WINDOW_PROP_LEFT, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_WRAPPER(native_window_prop_get),
        JSOP_WRAPPER(native_window_prop_set)},
    {"top", WINDOW_PROP_TOP, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_WRAPPER(native_window_prop_get),
        JSOP_WRAPPER(native_window_prop_set)},
    {"width", WINDOW_PROP_WIDTH, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_WRAPPER(native_window_prop_get),
        JSOP_WRAPPER(native_window_prop_set)},
    {"height", WINDOW_PROP_HEIGHT, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_WRAPPER(native_window_prop_get),
        JSOP_WRAPPER(native_window_prop_set)},
    {"title", WINDOW_PROP_TITLE, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_WRAPPER(native_window_prop_get),
        JSOP_WRAPPER(native_window_prop_set)},
    {"cursor", WINDOW_PROP_CURSOR, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_NULLWRAPPER,
        JSOP_WRAPPER(native_window_prop_set)},
    {"titleBarColor", WINDOW_PROP_TITLEBAR_COLOR, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_NULLWRAPPER,
        JSOP_WRAPPER(native_window_prop_set)},
    {"titleBarControlsOffsetX", WINDOW_PROP_TITLEBAR_CONTROLS_OFFSETX, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_NULLWRAPPER,
        JSOP_WRAPPER(native_window_prop_set)},
    {"titleBarControlsOffsetY", WINDOW_PROP_TITLEBAR_CONTROLS_OFFSETY, JSPROP_PERMANENT | JSPROP_ENUMERATE,
        JSOP_NULLWRAPPER,
        JSOP_WRAPPER(native_window_prop_set)},
    {0, 0, 0, JSOP_NULLWRAPPER, JSOP_NULLWRAPPER}
};

NativeJSwindow::~NativeJSwindow()
{
    if (m_Dragging) {
        /* cleanup drag files */
    }
    if (m_Db) {
        delete m_Db;
    }
};

void NativeJSwindow::onReady(JSObject *layout)
{
    jsval onready, rval, arg[1];
    JS::RootedObject rlayout(cx);

    if (layout) {
        rlayout = layout;
        arg[0] = OBJECT_TO_JSVAL(rlayout);
    } else {
        arg[0] = OBJECT_TO_JSVAL(JS_NewArrayObject(cx, 0, NULL));
    }

    if (JS_GetProperty(cx, this->jsobj, "_onready", &onready) &&
        !JSVAL_IS_PRIMITIVE(onready) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onready))) {

        JS_CallFunctionValue(cx, this->jsobj, onready, 1, arg, &rval);
    }
}

bool NativeJSwindow::onClose()
{
    jsval onclose, rval;

    if (JS_GetProperty(cx, this->jsobj, "_onbeforeclose", &onclose) &&
        !JSVAL_IS_PRIMITIVE(onclose) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onclose))) {

        JS_CallFunctionValue(cx, this->jsobj, onclose, 0, NULL, &rval);

        return (rval.isUndefined() || rval.toBoolean());
    }    

    return true;
}

void NativeJSwindow::assetReady(const NMLTag &tag)
{
#define EVENT_PROP(name, val) JS_DefineProperty(cx, event, name, \
        val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)

    jsval onassetready, rval, jevent;
    JSObject *event;

    event = JS_NewObject(cx, &NMLEvent_class, NULL, NULL);
    jevent = OBJECT_TO_JSVAL(event);

    EVENT_PROP("data", STRING_TO_JSVAL(NativeJSUtils::newStringWithEncoding(cx,
        (const char *)tag.content.data, tag.content.len, "utf8")));
    EVENT_PROP("tag", STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (const char *)tag.tag)));
    EVENT_PROP("id", STRING_TO_JSVAL(JS_NewStringCopyZ(cx, (const char *)tag.id)));

    if (JS_GetProperty(cx, this->jsobj, "_onassetready", &onassetready) &&
        !JSVAL_IS_PRIMITIVE(onassetready) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onassetready))) {

        JS_CallFunctionValue(cx, event, onassetready, 1, &jevent, &rval);
    }
}

void NativeJSwindow::windowFocus()
{
    jsval rval, onfocus;

    if (JS_GetProperty(cx, this->jsobj,
        "_onfocus", &onfocus) &&
        !JSVAL_IS_PRIMITIVE(onfocus) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onfocus))) {

        JS_CallFunctionValue(cx, NULL, onfocus, 0, NULL, &rval);
    }    
}

void NativeJSwindow::windowBlur()
{
    jsval rval, onblur;

    if (JS_GetProperty(cx, this->jsobj,
        "_onblur", &onblur) &&
        !JSVAL_IS_PRIMITIVE(onblur) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onblur))) {

        JS_CallFunctionValue(cx, NULL, onblur, 0, NULL, &rval);
    }   
}

void NativeJSwindow::mouseWheel(int xrel, int yrel, int x, int y)
{
#define EVENT_PROP(name, val) JS_DefineProperty(cx, event, name, \
    val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)
    
    jsval rval, jevent, onwheel;
    JSObject *event;

    JSAutoRequest ar(cx);

    event = JS_NewObject(cx, &mouseEvent_class, NULL, NULL);

    EVENT_PROP("xrel", INT_TO_JSVAL(xrel));
    EVENT_PROP("yrel", INT_TO_JSVAL(yrel));
    EVENT_PROP("x", INT_TO_JSVAL(x));
    EVENT_PROP("y", INT_TO_JSVAL(y));

    jevent = OBJECT_TO_JSVAL(event);

    if (JS_GetProperty(cx, this->jsobj, "_onmousewheel", &onwheel) &&
        !JSVAL_IS_PRIMITIVE(onwheel) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onwheel))) {

        JS_CallFunctionValue(cx, event, onwheel, 1, &jevent, &rval);
    }
#undef EVENT_PROP
}

void NativeJSwindow::keyupdown(int keycode, int mod, int state, int repeat, int location)
{
#define EVENT_PROP(name, val) JS_DefineProperty(cx, event, name, \
    val, NULL, NULL, JSPROP_PERMANENT | JSPROP_ENUMERATE)
    
    JSObject *event;
    jsval jevent, onkeyupdown, rval;

    JSAutoRequest ar(cx);

    event = JS_NewObject(cx, &keyEvent_class, NULL, NULL);

    EVENT_PROP("keyCode", INT_TO_JSVAL(keycode));
    EVENT_PROP("location", INT_TO_JSVAL(location));
    EVENT_PROP("altKey", BOOLEAN_TO_JSVAL(!!(mod & NATIVE_KEY_ALT)));
    EVENT_PROP("ctrlKey", BOOLEAN_TO_JSVAL(!!(mod & NATIVE_KEY_CTRL)));
    EVENT_PROP("shiftKey", BOOLEAN_TO_JSVAL(!!(mod & NATIVE_KEY_SHIFT)));
    EVENT_PROP("metaKey", BOOLEAN_TO_JSVAL(!!(mod & NATIVE_KEY_META)));
    EVENT_PROP("spaceKey", BOOLEAN_TO_JSVAL(keycode == 32));
    EVENT_PROP("repeat", BOOLEAN_TO_JSVAL(!!(repeat)));

    jevent = OBJECT_TO_JSVAL(event);

    if (JS_GetProperty(cx, this->jsobj,
        (state ? "_onkeydown" : "_onkeyup"), &onkeyupdown) &&
        !JSVAL_IS_PRIMITIVE(onkeyupdown) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onkeyupdown))) {

        JS_CallFunctionValue(cx, event, onkeyupdown, 1, &jevent, &rval);
    }
#undef EVENT_PROP
}

void NativeJSwindow::textInput(const char *data)
{
#define EVENT_PROP(name, val) JS_DefineProperty(cx, event, name, \
    val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)

    JSObject *event;
    jsval jevent, ontextinput, rval;

    JSAutoRequest ar(cx);

    event = JS_NewObject(cx, &textEvent_class, NULL, NULL);

    EVENT_PROP("val",
        STRING_TO_JSVAL(NativeJSUtils::newStringWithEncoding(cx, data,
        strlen(data), "utf8")));

    jevent = OBJECT_TO_JSVAL(event);

    if (JS_GetProperty(cx, this->jsobj, "_ontextinput", &ontextinput) &&
        !JSVAL_IS_PRIMITIVE(ontextinput) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(ontextinput))) {

        JS_CallFunctionValue(cx, event, ontextinput, 1, &jevent, &rval);
    }
}

void NativeJSwindow::mouseClick(int x, int y, int state, int button)
{
#define EVENT_PROP(name, val) JS_DefineProperty(cx, event, name, \
    val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)

    jsval rval, jevent;
    JSObject *event;

    jsval onclick;

    JSAutoRequest ar(cx);

    event = JS_NewObject(cx, &mouseEvent_class, NULL, NULL);

    EVENT_PROP("x", INT_TO_JSVAL(x));
    EVENT_PROP("y", INT_TO_JSVAL(y));
    EVENT_PROP("clientX", INT_TO_JSVAL(x));
    EVENT_PROP("clientY", INT_TO_JSVAL(y));
    EVENT_PROP("which", INT_TO_JSVAL(button));

    jevent = OBJECT_TO_JSVAL(event);

    if (JS_GetProperty(cx, this->jsobj,
        (state ? "_onmousedown" : "_onmouseup"), &onclick) &&
        !JSVAL_IS_PRIMITIVE(onclick) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onclick))) {

        JS_CallFunctionValue(cx, event, onclick, 1, &jevent, &rval);
    }
}


bool NativeJSwindow::dragEvent(const char *name, int x, int y)
{
    jsval rval, jevent, ondragevent;
    JSObject *event;

    JSAutoRequest ar(cx);

    event = JS_NewObject(cx, &dragEvent_class, NULL, NULL);


    EVENT_PROP("x", INT_TO_JSVAL(x));
    EVENT_PROP("y", INT_TO_JSVAL(y));
    EVENT_PROP("clientX", INT_TO_JSVAL(x));
    EVENT_PROP("clientY", INT_TO_JSVAL(y));

    if (m_DragedFiles) {
        EVENT_PROP("files", OBJECT_TO_JSVAL(this->m_DragedFiles));
    }

    jevent = OBJECT_TO_JSVAL(event);

    if (JS_GetProperty(cx, this->jsobj, name, &ondragevent) &&
        !JSVAL_IS_PRIMITIVE(ondragevent) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(ondragevent))) {

        if (!JS_CallFunctionValue(cx, event, ondragevent, 1, &jevent, &rval)) {
            printf("Failed to exec func\n");
            return false;
        }

        return rval.toBoolean();
    }    

    return false;
}

bool NativeJSwindow::dragBegin(int x, int y, const char * const *files, size_t nfiles)
{
#define EVENT_PROP(name, val) JS_DefineProperty(cx, event, name, \
    val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)

    if (m_Dragging) {
        return false;
    }

    m_Dragging = true;

    m_DragedFiles = JS_NewArrayObject(cx, (int)nfiles, NULL);

    for (int i = 0; i < nfiles; i++) {
        jsval val = OBJECT_TO_JSVAL(NativeJSFileIO::generateJSObject(cx, files[i]));
        JS_SetElement(cx, m_DragedFiles, i, &val);         
    }

    NativeJS::getNativeClass(cx)->rootObjectUntilShutdown(m_DragedFiles);

    return this->dragEvent("_onFileDragEnter", x, y);

}

void NativeJSwindow::dragLeave()
{
    if (!m_Dragging) {
        return;
    }
    this->dragEvent("_onFileDragLeave", 0, 0);
    this->dragEnd();
}

bool NativeJSwindow::dragUpdate(int x, int y)
{
    if (!m_Dragging) {
        return false;
    }
    bool drag = this->dragEvent("_onFileDrag", x, y);
    return drag;
}

bool NativeJSwindow::dragDroped(int x, int y)
{
    if (!m_Dragging) {
        return false;
    }

    return this->dragEvent("_onFileDrop", x, y);
}

void NativeJSwindow::dragEnd()
{
    if (!m_Dragging) {
        return;
    }

    NativeJS::getNativeClass(cx)->unrootObject(m_DragedFiles);

    m_DragedFiles = NULL;
    m_Dragging = false;
}

void NativeJSwindow::resized(int width, int height)
{
    NativeContext::getNativeClass(cx)->sizeChanged(width, height);
}

void NativeJSwindow::mouseMove(int x, int y, int xrel, int yrel)
{
#define EVENT_PROP(name, val) JS_DefineProperty(cx, event, name, \
    val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)
    
    jsval rval, jevent, onmove;
    JSObject *event;

    NativeCanvasHandler *rootHandler = NativeContext::getNativeClass(this->cx)->getRootHandler();

    rootHandler->mousePosition.x = x;
    rootHandler->mousePosition.y = y;
    rootHandler->mousePosition.xrel += xrel;
    rootHandler->mousePosition.yrel += yrel;

    rootHandler->mousePosition.consumed = false;
    
    event = JS_NewObject(cx, &mouseEvent_class, NULL, NULL);

    EVENT_PROP("x", INT_TO_JSVAL(x));
    EVENT_PROP("y", INT_TO_JSVAL(y));
    EVENT_PROP("xrel", INT_TO_JSVAL(xrel));
    EVENT_PROP("yrel", INT_TO_JSVAL(yrel));
    EVENT_PROP("clientX", INT_TO_JSVAL(x));
    EVENT_PROP("clientY", INT_TO_JSVAL(y));

    jevent = OBJECT_TO_JSVAL(event);

    if (JS_GetProperty(cx, this->jsobj, "_onmousemove", &onmove) &&
        !JSVAL_IS_PRIMITIVE(onmove) && 
        JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(onmove))) {

        JS_CallFunctionValue(cx, event, onmove, 1, &jevent, &rval);
    }

}

static void Window_Finalize(JSFreeOp *fop, JSObject *obj)
{
    NativeJSwindow *jwin = NativeJSwindow::getNativeClass(obj);

    if (jwin != NULL) {
        delete jwin;
    }
}

static void Storage_Finalize(JSFreeOp *fop, JSObject *obj)
{

}

static JSBool native_window_prop_get(JSContext *cx, JSHandleObject obj,
    JSHandleId id, JSMutableHandleValue vp)
{
    NativeUIInterface *NUI = NativeContext::getNativeClass(cx)->getUI();

    switch(JSID_TO_INT(id)) {
        case WINDOW_PROP_DEVICE_PIXELRATIO:
            /* TODO: Actual value */
            vp.setInt32(1);
            break;
        case WINDOW_PROP_LEFT:
        {
            int x;
            NUI->getWindowPosition(&x, NULL);
            vp.set(INT_TO_JSVAL(x));
            break;
        }
        case WINDOW_PROP_TOP:
        {
            int y;
            NUI->getWindowPosition(NULL, &y);
            vp.set(INT_TO_JSVAL(y));
            break;
        }
        case WINDOW_PROP_WIDTH:
            vp.set(INT_TO_JSVAL(NUI->getWidth()));
            break;
        case WINDOW_PROP_HEIGHT:
            vp.set(INT_TO_JSVAL(NUI->getHeight()));
            break;
        case WINDOW_PROP_TITLE:
        {
            const char *title =  NUI->getWindowTitle();
            JSString *str = NativeJSUtils::newStringWithEncoding(cx, title,
                strlen(title), "utf8");
            vp.set(STRING_TO_JSVAL(str));
        }
        break;
    }

    return true;
}

static JSBool native_window_prop_set(JSContext *cx, JSHandleObject obj,
    JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    NativeUIInterface *NUI = NativeContext::getNativeClass(cx)->getUI();
    switch(JSID_TO_INT(id)) {
        case WINDOW_PROP_LEFT:
        {
            double dval;
            if (!JSVAL_IS_NUMBER(vp)) {
                return true;
            }
            
            JS_ValueToNumber(cx, vp, &dval);

            NUI->setWindowPosition((int)dval, NATIVE_WINDOWPOS_UNDEFINED_MASK);

            break;
        }

        case WINDOW_PROP_TOP:
        {
            double dval;
            if (!JSVAL_IS_NUMBER(vp)) {
                return true;
            }
            
            JS_ValueToNumber(cx, vp, &dval);

            NUI->setWindowPosition(NATIVE_WINDOWPOS_UNDEFINED_MASK, (int)dval);

            break;
        }
        case WINDOW_PROP_WIDTH:
        {
            double dval;
            if (!JSVAL_IS_NUMBER(vp)) {
                return true;
            }
            
            JS_ValueToNumber(cx, vp, &dval);
            NativeContext::getNativeClass(cx)->setWindowSize((int)dval, NUI->getHeight());

            break;
        }
        case WINDOW_PROP_HEIGHT:
        {
            double dval;
            if (!JSVAL_IS_NUMBER(vp)) {
                return true;
            }

            JS_ValueToNumber(cx, vp, &dval);
            NativeContext::getNativeClass(cx)->setWindowSize((int)NUI->getWidth(), (int)dval);

            break;
        }
        case WINDOW_PROP_TITLE:
        {
            if (!JSVAL_IS_STRING(vp)) {
                return true;
            }
            JSAutoByteString title(cx, JSVAL_TO_STRING(vp));      
            NUI->setWindowTitle(title.ptr());
            break;      
        }
        case WINDOW_PROP_CURSOR:
        {
            if (!JSVAL_IS_STRING(vp)) {
                return true;
            }
            JSAutoByteString type(cx, JSVAL_TO_STRING(vp));

            for (int i = 0; native_cursors_list[i].str != NULL; i++) {
                if (strncasecmp(native_cursors_list[i].str, type.ptr(),
                    strlen(native_cursors_list[i].str)) == 0) {
                    NUI->setCursor(native_cursors_list[i].type);
                    break;
                }
            }

            break;
        }
        case WINDOW_PROP_TITLEBAR_COLOR:
        {
            if (!JSVAL_IS_STRING(vp)) {
                return true;
            }
            JSAutoByteString color(cx, JSVAL_TO_STRING(vp));
            uint32_t icolor = NativeSkia::parseColor(color.ptr());

            NUI->setTitleBarRGBAColor(
                (icolor & 0x00FF0000) >> 16,
                (icolor & 0x0000FF00) >> 8,
                 icolor & 0x000000FF,
                 icolor >> 24);

            break;
        }
        case WINDOW_PROP_TITLEBAR_CONTROLS_OFFSETX:
        {
            double dval, oval;
            if (!JSVAL_IS_NUMBER(vp)) {
                return true;
            }
            JS_ValueToNumber(cx, vp, &dval);
            jsval offsety;

            if (JS_GetProperty(cx, obj.get(), "titleBarControlsOffsetY", &offsety) == false) {
                offsety = DOUBLE_TO_JSVAL(0);
            }

            JS_ValueToNumber(cx, offsety, &oval);

            NUI->setWindowControlsOffset(dval, oval);
            break;
        }
        case WINDOW_PROP_TITLEBAR_CONTROLS_OFFSETY:
        {
            double dval, oval;
            if (!JSVAL_IS_NUMBER(vp)) {
                return true;
            }
            JS_ValueToNumber(cx, vp, &dval);
            jsval offsetx;
            if (JS_GetProperty(cx, obj.get(), "titleBarControlsOffsetX", &offsetx) == false) {
                offsetx = DOUBLE_TO_JSVAL(0);
            }
            JS_ValueToNumber(cx, offsetx, &oval);

            NUI->setWindowControlsOffset(oval, dval);
            break;
        }
        default:
            break;
    }
    return true;
}

struct _nativeopenfile
{
    JSContext *cx;
    jsval cb;
};

static void native_window_openfilecb(void *_nof, const char *lst[], uint32_t len)
{
    struct _nativeopenfile *nof = (struct _nativeopenfile *)_nof;
    jsval rval;

    jsval cb = nof->cb;

    JSObject *arr = JS_NewArrayObject(nof->cx, len, NULL);

    for (int i = 0; i < len; i++) {
        jsval val = OBJECT_TO_JSVAL(NativeJSFileIO::generateJSObject(nof->cx, lst[i]));
        JS_SetElement(nof->cx, arr, i, &val);        
    }

    jsval jarr = OBJECT_TO_JSVAL(arr);

    JS_CallFunctionValue(nof->cx, JS_GetGlobalObject(nof->cx), cb, 1, &jarr, &rval);

    JS_RemoveValueRoot(nof->cx, &nof->cb);
    free(nof);

}

static JSBool native_window_setSize(JSContext *cx, unsigned argc, jsval *vp)
{
    double w, h;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dd", &w, &h)) {
        return false;
    }

    NativeContext::getNativeClass(cx)->setWindowSize(w, h);

    return true;
}

static JSBool native_window_openDirDialog(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval callback;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "v", &callback)) {
        return false;
    }

    struct _nativeopenfile *nof = (struct _nativeopenfile *)malloc(sizeof(*nof));
    nof->cb = callback;
    nof->cx = cx;

    JS_AddValueRoot(cx, &nof->cb);

    NativeContext::getNativeClass(cx)->getUI()->openFileDialog(
        NULL,
        native_window_openfilecb, nof,
        NativeUIInterface::kOpenFile_CanChooseDir);

    return true;
}

/* TODO: leak if the user click "cancel" */
static JSBool native_window_openFileDialog(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *types;
    jsval callback;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "ov", &types, &callback)) {
        return false;
    }

    if (!JSVAL_IS_NULL(OBJECT_TO_JSVAL(types)) && !JS_IsArrayObject(cx, types)) {
        JS_ReportError(cx, "First parameter must be an array or null");
        return false;
    }
    uint32_t len = 0;

    char **ctypes = NULL;

    if (!JSVAL_IS_NULL(OBJECT_TO_JSVAL(types))) {
        JS_GetArrayLength(cx, types, &len);

        ctypes = (char **)malloc(sizeof(char *) * (len+1));
        memset(ctypes, 0, sizeof(char *) * (len+1));
        int j = 0;
        for (int i = 0; i < len; i++) {
            jsval val;
            JS_GetElement(cx, types, i, &val);

            if (JSVAL_IS_STRING(val)) {
                JSString *str = JSVAL_TO_STRING(val);
                ctypes[j] = JS_EncodeString(cx, str);
                j++;
            }
        }
        ctypes[j] = NULL;
    }

    struct _nativeopenfile *nof = (struct _nativeopenfile *)malloc(sizeof(*nof));
    nof->cb = callback;
    nof->cx = cx;

    JS_AddValueRoot(cx, &nof->cb);

    NativeContext::getNativeClass(cx)->getUI()->openFileDialog(
        (const char **)ctypes,
        native_window_openfilecb, nof,
        NativeUIInterface::kOpenFile_CanChooseFile | NativeUIInterface::kOpenFile_AlloMultipleSelection);

    if (ctypes) {
        for (int i = 0; i < len; i++) {
            JS_free(cx, ctypes[i]);
        }
        free(ctypes);
    }
    return true;
}

static JSBool native_window_requestAnimationFrame(JSContext *cx, unsigned argc, jsval *vp)
{
    NATIVE_CHECK_ARGS("requestAnimationFrame", 1);
    JS::CallArgs args = CallArgsFromVp(argc, vp);
    jsval cb;

    if (!JS_ConvertValue(cx, args.array()[0], JSTYPE_FUNCTION, &cb)) {
        return true;
    }

    NativeJSwindow::getNativeClass(cx)->addFrameCallback(cb);

    return true;
}

static JSBool native_window_center(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeContext::getNativeClass(cx)->getUI()->centerWindow();

    return true;
}

static JSBool native_window_setPosition(JSContext *cx, unsigned argc, jsval *vp)
{
    NATIVE_CHECK_ARGS("setPosition", 2);

    JS::CallArgs args = CallArgsFromVp(argc, vp);

    int x = (args[0].isUndefined() || args[0].isNull()) ?
        NATIVE_WINDOWPOS_UNDEFINED_MASK : args[0].toInt32();

    int y = (args[1].isUndefined() || args[1].isNull()) ?
        NATIVE_WINDOWPOS_UNDEFINED_MASK : args[1].toInt32();

    NativeContext::getNativeClass(cx)->getUI()->setWindowPosition(x, y);

    return true;
}

static JSBool native_window_notify(JSContext *cx, unsigned argc, jsval *vp)
{
    NATIVE_CHECK_ARGS("notify", 2);

    JSString *title, *body;
    JSBool sound = false;

    JS::CallArgs args = CallArgsFromVp(argc, vp);

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "SS/b", &title, &body, &sound)) {
        return false;
    }

    JSAutoByteString ctitle;
    ctitle.encodeUtf8(cx, title);

    JSAutoByteString cbody;
    cbody.encodeUtf8(cx, body);

    NativeSystemInterface::getInstance()->sendNotification(ctitle.ptr(), cbody.ptr(), sound);

    return true;
}

static JSBool native_window_quit(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeUIInterface *NUI = NativeContext::getNativeClass(cx)->getUI();

    NUI->quit();

    return true;
}

static JSBool native_window_close(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeUIInterface *NUI = NativeContext::getNativeClass(cx)->getUI();

    NUI->hideWindow();

    return true;
}

static JSBool native_window_open(JSContext *cx, unsigned argc, jsval *vp)
{
    NativeUIInterface *NUI = NativeContext::getNativeClass(cx)->getUI();

    NUI->showWindow();

    return true;
}

static JSBool native_window_setSystemTray(JSContext *cx, unsigned argc, jsval *vp)
{
    NATIVE_CHECK_ARGS("notify", 1);
    JS::CallArgs args = CallArgsFromVp(argc, vp);
    NativeUIInterface *NUI = NativeContext::getNativeClass(cx)->getUI();

    JS::Value jobj = args[0];

    if (jobj.isNull() || !jobj.isObject()) {

        NUI->disableSysTray();

        return true;
    }

    JS_INITOPT();

    JSGET_OPT_TYPE(jobj.toObjectOrNull(), "icon", Object) {
        JSObject *jsimg = __curopt.toObjectOrNull();
        if (NativeJSImage::JSObjectIs(cx, jsimg)) {
            
        }
    }

    return true;
}

static JSBool native_window_setFrame(JSContext *cx, unsigned argc, jsval *vp)
{
    NATIVE_CHECK_ARGS("setFrame", 4);

    JS::CallArgs args = CallArgsFromVp(argc, vp);

    int x = 0, y = 0;

    if (args[0].isString()) {
        JSString *xstr = args[0].toString();
        JSAutoByteString cxstr(cx, xstr);

        if (strcmp(cxstr.ptr(), "center") == 0) {
            x = NATIVE_WINDOWPOS_CENTER_MASK;
        }

    } else if (args[0].isNumber()) {
        x = args[0].toInt32();
    } else {
        JS_ReportError(cx, "setFrame() invalid position");
        return false;
    }
    if (args[1].isString()) {
        JSString *ystr = args[1].toString();
        JSAutoByteString cystr(cx, ystr);

        if (strcmp(cystr.ptr(), "center") == 0) {
            y = NATIVE_WINDOWPOS_CENTER_MASK;
        }

    } else if (args[1].isNumber()) {
        y = args[1].toInt32();
    } else {
        JS_ReportError(cx, "setFrame() invalid position");
        return false;
    }

    NativeContext::getNativeClass(cx)->setWindowFrame(x, y,
        args[2].toInt32(), args[3].toInt32());

    return true;
}

void NativeJSwindow::addFrameCallback(jsval &cb)
{
    struct _requestedFrame *frame = new struct _requestedFrame;
    frame->next = this->m_RequestedFrame;
    frame->cb = cb;

    m_RequestedFrame = frame;

    JS_AddValueRoot(this->cx, &frame->cb);
}

void NativeJSwindow::callFrameCallbacks(double ts, bool garbage)
{
    jsval rval, arg[1];
    struct _requestedFrame *frame = m_RequestedFrame;

    /* Set to NULL so that callbacks can "fork" the new chain */
    m_RequestedFrame = NULL;

    arg[0] = JS_NumberValue(ts/1000000L);

    while (frame != NULL) {
        if (!garbage) {
            JS_CallFunctionValue(this->cx, JS_GetGlobalObject(this->cx),
                frame->cb, 1, arg, &rval);
        }

        JS_RemoveValueRoot(this->cx, &frame->cb);

        struct _requestedFrame *tmp = frame->next;
        delete frame;
        frame = tmp;
    }
}

void NativeJSwindow::initDataBase()
{
    NativeNML *nml = NativeContext::getNativeClass(this->cx)->getNML();
    if (!nml) {
        NLOG("[Notice] Unable to create window.storage (no NML provided)");
        return;
    }

    m_Db = new NativeDB(nml->getIdentifier());

    if (m_Db->ok()) {
        this->createStorage();
    } else {
        NLOG("[Notice] Unable to create window.storage (empty identifier tag in NML?)");
    }
}

void NativeJSwindow::createMainCanvas(int width, int height)
{
    JSObject *canvas;

    canvas = NativeJSCanvas::generateJSObject(this->cx, width,
        height, &m_handler);

    NativeContext::getNativeClass(cx)->getRootHandler()->addChild(m_handler);

    JS_DefineProperty(this->cx, this->jsobj, "canvas",
        OBJECT_TO_JSVAL(canvas), NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT);
}

void NativeJSwindow::createStorage()
{
    JS::RootedObject obj(this->cx, this->jsobj);
    JS::RootedObject storage(cx, JS_NewObject(cx, &storage_class, NULL, NULL));

    JS_DefineFunctions(this->cx, storage, storage_funcs);

    jsval jsstorage = OBJECT_TO_JSVAL(storage.get());

    JS_SetProperty(this->cx, obj.get(), "storage", &jsstorage);
}

JSBool native_storage_set(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    NATIVE_CHECK_ARGS("set", 2);
    if (!args[0].isString()) {
        JS_ReportError(cx, "set() : key must be a string");
        return false;
    }

    JSAutoByteString key(cx, args[0].toString());

    if (!NativeJSwindow::getNativeClass(cx)->getDataBase()->
        insert(key.ptr(), cx, args[1])) {

        JS_ReportError(cx, "Cant insert data in storage");
        return false;
    }

    args.rval().setBoolean(true);

    return true; 
}

JSBool native_storage_get(JSContext *cx, unsigned argc, jsval *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    NATIVE_CHECK_ARGS("get", 1);
    if (!args[0].isString()) {
        JS_ReportError(cx, "get() : key must be a string");
        return false;
    }

    JSAutoByteString key(cx, args[0].toString());
    NativeDB *db = NativeJSwindow::getNativeClass(cx)->getDataBase();

    std::string data;

    if (!db->get(key.ptr(), data)) {
        args.rval().setUndefined();
        return true;
    }
    jsval ret;

    uint64_t *aligned_data;

    /*
        ReadStructuredClone requires 8-bytes aligned memory
    */
    if (((uintptr_t)data.data() & 7) == 0) {
        aligned_data = (uint64_t *)data.data();
    } else {
        if (posix_memalign((void **)&aligned_data, 8, data.length()) != 0) {
            return false;
        }

        memcpy(aligned_data, data.data(), data.length());
    }

    if (!JS_ReadStructuredClone(cx, aligned_data, data.length(),
        JS_STRUCTURED_CLONE_VERSION, &ret, NULL, NULL)) {

        JS_ReportError(cx, "Unable to read internal data");
        return false;
    }

    args.rval().set(ret);

    if ((void *)aligned_data != data.data()) {
        free(aligned_data);
    }

    return true;
}

void NativeJSwindow::registerObject(JSContext *cx, int width, int height)
{
    NativeJSwindow *jwin = new NativeJSwindow();
    
    JSObject *windowObj;
    windowObj = JS_DefineObject(cx, JS_GetGlobalObject(cx),
        NativeJSwindow::getJSObjectName(),
        &window_class , NULL,
        JSPROP_PERMANENT | JSPROP_ENUMERATE | JSPROP_READONLY);

    jwin->jsobj = windowObj;
    jwin->cx = cx;
    JS_SetPrivate(windowObj, jwin);

    jwin->initDataBase();
    
    jwin->createMainCanvas(width, height);

    NativeJS::getNativeClass(cx)->jsobjects.set(
        NativeJSwindow::getJSObjectName(), windowObj);

    JS_DefineFunctions(cx, windowObj, window_funcs);
    JS_DefineProperties(cx, windowObj, window_props);

    jsval val;

    val = DOUBLE_TO_JSVAL(0);
    JS_SetProperty(cx, windowObj, "titleBarControlsOffsetX", &val);

    val = DOUBLE_TO_JSVAL(0);
    JS_SetProperty(cx, windowObj, "titleBarControlsOffsetY", &val);
}

