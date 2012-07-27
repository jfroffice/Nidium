/**
 **   Copyright (c) 2012 All Right Reserved, Troll Face Studio
 **
 **   Authors :
 **       * Anthony Catel <a.catel@trollfacestudio.com>
 **/

#include "NativeJS.h"
#include "NativeSkia.h"
#include "NativeSkGradient.h"
#include <stdio.h>
#include <jsapi.h>

enum {
    CANVAS_PROP_FILLSTYLE = 1,
    CANVAS_PROP_STROKESTYLE,
    CANVAS_PROP_LINEWIDTH,
    CANVAS_PROP_GLOBALALPHA,
    CANVAS_PROP_LINECAP,
    CANVAS_PROP_LINEJOIN
};

#define NSKIA ((class NativeSkia *)JS_GetContextPrivate(cx))

static JSClass global_class = {
    "_GLOBAL", JSCLASS_GLOBAL_FLAGS | JSCLASS_IS_GLOBAL,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass canvas_class = {
    "Canvas", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSClass canvasGradient_class = {
    "CanvasGradient", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

jsval gfunc = JSVAL_VOID;

/******** Natives ********/
static JSBool Print(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_fillRect(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_strokeRect(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_clearRect(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_fillText(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_beginPath(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_moveTo(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_lineTo(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_fill(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_stroke(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_closePath(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_arc(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_quadraticCurveTo(JSContext *cx, unsigned argc,
    jsval *vp);
static JSBool native_canvas_bezierCurveTo(JSContext *cx, unsigned argc,
    jsval *vp);
static JSBool native_canvas_rotate(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_scale(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_save(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_restore(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_translate(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_transform(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_setTransform(JSContext *cx, unsigned argc,
    jsval *vp);
static JSBool native_canvas_clip(JSContext *cx, unsigned argc, jsval *vp);
static JSBool native_canvas_createLinearGradient(JSContext *cx,
    unsigned argc, jsval *vp);
static JSBool native_canvasGradient_addColorStop(JSContext *cx,
    unsigned argc, jsval *vp);
static JSBool native_canvas_requestAnimationFrame(JSContext *cx,
    unsigned argc, jsval *vp);

/*************************/

/******** Setters ********/

static JSBool native_canvas_prop_set(JSContext *cx, JSHandleObject obj,
    JSHandleId id, JSBool strict, jsval *vp);


static JSPropertySpec canvas_props[] = {
    {"fillStyle", CANVAS_PROP_FILLSTYLE, JSPROP_PERMANENT, NULL,
        native_canvas_prop_set},
    {"strokeStyle", CANVAS_PROP_STROKESTYLE, JSPROP_PERMANENT, NULL,
        native_canvas_prop_set},
    {"lineWidth", CANVAS_PROP_LINEWIDTH, JSPROP_PERMANENT, NULL,
        native_canvas_prop_set},
    {"globalAlpha", CANVAS_PROP_GLOBALALPHA, JSPROP_PERMANENT, NULL,
        native_canvas_prop_set},
    {"lineCap", CANVAS_PROP_LINECAP, JSPROP_PERMANENT, NULL,
        native_canvas_prop_set},
    {"lineJoin", CANVAS_PROP_LINEJOIN, JSPROP_PERMANENT, NULL,
        native_canvas_prop_set},
    {NULL}
};
/*************************/



static JSFunctionSpec glob_funcs[] = {
    
    JS_FN("echo", Print, 0, 0),

    JS_FS_END
};

static JSFunctionSpec gradient_funcs[] = {
    
    JS_FN("addColorStop", native_canvasGradient_addColorStop, 2, 0),

    JS_FS_END
};

static JSFunctionSpec canvas_funcs[] = {
    
    JS_FN("fillRect", native_canvas_fillRect, 4, 0),
    JS_FN("fillText", native_canvas_fillText, 3, 0),
    JS_FN("strokeRect", native_canvas_strokeRect, 4, 0),
    JS_FN("clearRect", native_canvas_clearRect, 4, 0),
    JS_FN("beginPath", native_canvas_beginPath, 0, 0),
    JS_FN("moveTo", native_canvas_moveTo, 2, 0),
    JS_FN("lineTo", native_canvas_lineTo, 2, 0),
    JS_FN("fill", native_canvas_fill, 0, 0),
    JS_FN("stroke", native_canvas_stroke, 0, 0),
    JS_FN("closePath", native_canvas_closePath, 0, 0),
    JS_FN("clip", native_canvas_clip, 0, 0),
    JS_FN("arc", native_canvas_arc, 5, 0),
    JS_FN("quadraticCurveTo", native_canvas_quadraticCurveTo, 4, 0),
    JS_FN("bezierCurveTo", native_canvas_bezierCurveTo, 4, 0),
    JS_FN("rotate", native_canvas_rotate, 1, 0),
    JS_FN("scale", native_canvas_scale, 2, 0),
    JS_FN("save", native_canvas_save, 0, 0),
    JS_FN("restore", native_canvas_restore, 0, 0),
    JS_FN("translate", native_canvas_translate, 2, 0),
    JS_FN("transform", native_canvas_transform, 6, 0),
    JS_FN("setTransform", native_canvas_setTransform, 6, 0),
    JS_FN("createLinearGradient", native_canvas_createLinearGradient, 4, 0),
    JS_FN("requestAnimationFrame", native_canvas_requestAnimationFrame, 1, 0),
    JS_FS_END
};

static void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stdout, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}

static JSBool
PrintInternal(JSContext *cx, unsigned argc, jsval *vp, FILE *file)
{
    jsval *argv;
    unsigned i;
    JSString *str;
    char *bytes;

    argv = JS_ARGV(cx, vp);
    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return false;
        bytes = JS_EncodeString(cx, str);
        if (!bytes)
            return false;
        fprintf(file, "%s%s", i ? " " : "", bytes);
        JS_free(cx, bytes);
    }

    fputc('\n', file);
    fflush(file);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return true;
}

static JSBool
Print(JSContext *cx, unsigned argc, jsval *vp)
{
    return PrintInternal(cx, argc, vp, stdout);
}

/* TODO: do not change the value when a wrong type is set */
static JSBool native_canvas_prop_set(JSContext *cx, JSHandleObject obj,
    JSHandleId id, JSBool strict, jsval *vp)
{

    switch(JSID_TO_INT(id)) {
        case CANVAS_PROP_FILLSTYLE:
        {
            if (JSVAL_IS_STRING(*vp)) {
                JSAutoByteString colorName(cx, JSVAL_TO_STRING(*vp));
                NSKIA->setFillColor(colorName.ptr());
            } else if (!JSVAL_IS_PRIMITIVE(*vp) && 
                JS_InstanceOf(cx, JSVAL_TO_OBJECT(*vp),
                    &canvasGradient_class, NULL)) {

                NativeSkGradient *gradient = (class NativeSkGradient *)
                                            JS_GetPrivate(JSVAL_TO_OBJECT(*vp));

                NSKIA->setFillColor(gradient);

            } else {
                *vp = JSVAL_VOID;

                return JS_TRUE;                
            }
        }
        break;
        case CANVAS_PROP_STROKESTYLE:
        {
            if (JSVAL_IS_STRING(*vp)) {
                JSAutoByteString colorName(cx, JSVAL_TO_STRING(*vp));
                NSKIA->setStrokeColor(colorName.ptr());
            } else if (!JSVAL_IS_PRIMITIVE(*vp) && 
                JS_InstanceOf(cx, JSVAL_TO_OBJECT(*vp),
                    &canvasGradient_class, NULL)) {

                NativeSkGradient *gradient = (class NativeSkGradient *)
                                            JS_GetPrivate(JSVAL_TO_OBJECT(*vp));

                NSKIA->setStrokeColor(gradient);

            } else {
                *vp = JSVAL_VOID;

                return JS_TRUE;                
            }    
        }
        break;
        case CANVAS_PROP_LINEWIDTH:
        {
            double ret;
            if (!JSVAL_IS_NUMBER(*vp)) {
                *vp = JSVAL_VOID;
                return JS_TRUE;
            }
            JS_ValueToNumber(cx, *vp, &ret);
            NSKIA->setLineWidth(ret);
        }
        break;
        case CANVAS_PROP_GLOBALALPHA:
        {
            double ret;
            if (!JSVAL_IS_NUMBER(*vp)) {
                *vp = JSVAL_VOID;
                return JS_TRUE;
            }
            JS_ValueToNumber(cx, *vp, &ret);
            NSKIA->setGlobalAlpha(ret);
        }
        break;
        case CANVAS_PROP_LINECAP:
        {
            if (!JSVAL_IS_STRING(*vp)) {
                *vp = JSVAL_VOID;

                return JS_TRUE;
            }
            JSAutoByteString lineCap(cx, JSVAL_TO_STRING(*vp));
            NSKIA->setLineCap(lineCap.ptr());                
        }
        break;
        case CANVAS_PROP_LINEJOIN:
        {
            if (!JSVAL_IS_STRING(*vp)) {
                *vp = JSVAL_VOID;

                return JS_TRUE;
            }
            JSAutoByteString lineJoin(cx, JSVAL_TO_STRING(*vp));
            NSKIA->setLineJoin(lineJoin.ptr());                
        }
        break;        
        default:
            break;
    }


    return JS_TRUE;
}

static JSBool native_canvas_fillRect(JSContext *cx, unsigned argc, jsval *vp)
{
    double x, y, width, height;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dddd", &x, &y, &width, &height)) {
        return JS_TRUE;
    }

    NSKIA->drawRect(x, y, width+x, height+y, 0);

    return JS_TRUE;
}

static JSBool native_canvas_strokeRect(JSContext *cx, unsigned argc, jsval *vp)
{
    double x, y, width, height;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dddd", &x, &y, &width, &height)) {
        return JS_TRUE;
    }

    NSKIA->drawRect(x, y, width+x, height+y, 1);

    return JS_TRUE;
}

static JSBool native_canvas_clearRect(JSContext *cx, unsigned argc, jsval *vp)
{
    int x, y, width, height;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "iiii", &x, &y, &width, &height)) {
        return JS_TRUE;
    }

    NSKIA->clearRect(x, y, width+x, height+y);

    return JS_TRUE;
}

static JSBool native_canvas_fillText(JSContext *cx, unsigned argc, jsval *vp)
{
    int x, y, maxwidth;
    JSString *str;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "Sii/i",
            &str, &x, &y, &maxwidth)) {
        return JS_TRUE;
    }

    JSAutoByteString text(cx, str);

    NSKIA->drawText(text.ptr(), x, y);

    return JS_TRUE;
}


static JSBool native_canvas_beginPath(JSContext *cx, unsigned argc, jsval *vp)
{
    NSKIA->beginPath();

    return JS_TRUE;
}

static JSBool native_canvas_moveTo(JSContext *cx, unsigned argc, jsval *vp)
{
    double x, y;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dd", &x, &y)) {
        return JS_TRUE;
    }

    NSKIA->moveTo(x, y);

    return JS_TRUE;
}

static JSBool native_canvas_lineTo(JSContext *cx, unsigned argc, jsval *vp)
{
    double x, y;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dd", &x, &y)) {
        return JS_TRUE;
    }

    NSKIA->lineTo(x, y);

    return JS_TRUE;
}

static JSBool native_canvas_fill(JSContext *cx, unsigned argc, jsval *vp)
{
    NSKIA->fill();

    return JS_TRUE;
}

static JSBool native_canvas_stroke(JSContext *cx, unsigned argc, jsval *vp)
{
    NSKIA->stroke();

    return JS_TRUE;
}
static JSBool native_canvas_closePath(JSContext *cx, unsigned argc, jsval *vp)
{
    NSKIA->closePath();

    return JS_TRUE;
}

static JSBool native_canvas_clip(JSContext *cx, unsigned argc, jsval *vp)
{
    NSKIA->clip();

    return JS_TRUE;
}

static JSBool native_canvas_arc(JSContext *cx, unsigned argc, jsval *vp)
{
    int x, y, radius;
    double startAngle, endAngle;
    JSBool CCW = JS_FALSE;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "iiidd/b", &x, &y,
        &radius, &startAngle, &endAngle, &CCW)) {
        return JS_TRUE;
    }

    NSKIA->arc(x, y, radius, startAngle, endAngle, CCW);

    return JS_TRUE;
}

static JSBool native_canvas_quadraticCurveTo(JSContext *cx, unsigned argc,
    jsval *vp)
{
    int x, y, cpx, cpy;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "iiii", &cpx, &cpy,
        &x, &y)) {
        return JS_TRUE;
    }

    NSKIA->quadraticCurveTo(cpx, cpy, x, y);

    return JS_TRUE;
}

static JSBool native_canvas_bezierCurveTo(JSContext *cx, unsigned argc,
    jsval *vp)
{
    double x, y, cpx, cpy, cpx2, cpy2;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dddddd", &cpx, &cpy,
        &cpx2, &cpy2, &x, &y)) {
        return JS_TRUE;
    }

    NSKIA->bezierCurveTo(cpx, cpy, cpx2, cpy2, x, y);

    return JS_TRUE;
}

static JSBool native_canvas_rotate(JSContext *cx, unsigned argc, jsval *vp)
{
    double angle;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "d", &angle)) {
        return JS_TRUE;
    }

    NSKIA->rotate(angle);

    return JS_TRUE;
}

static JSBool native_canvas_scale(JSContext *cx, unsigned argc, jsval *vp)
{
    double x, y;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dd", &x, &y)) {
        return JS_TRUE;
    }

    NSKIA->scale(x, y);

    return JS_TRUE;
}

static JSBool native_canvas_translate(JSContext *cx, unsigned argc, jsval *vp)
{
    double x, y;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dd", &x, &y)) {
        return JS_TRUE;
    }

    NSKIA->translate(x, y);

    return JS_TRUE;
}

static JSBool native_canvas_transform(JSContext *cx, unsigned argc, jsval *vp)
{
    double scalex, skewx, skewy, scaley, translatex, translatey;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dddddd",
        &scalex, &skewx, &skewy, &scaley, &translatex, &translatey)) {
        return JS_TRUE;
    }

    NSKIA->transform(scalex, skewx, skewy, scaley,
        translatex, translatey, 0);

    return JS_TRUE;
}

static JSBool native_canvas_setTransform(JSContext *cx, unsigned argc, jsval *vp)
{
    double scalex, skewx, skewy, scaley, translatex, translatey;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dddddd",
        &scalex, &skewx, &skewy, &scaley, &translatex, &translatey)) {
        return JS_TRUE;
    }

    NSKIA->transform(scalex, skewx, skewy, scaley,
        translatex, translatey, 1);

    return JS_TRUE;
}

static JSBool native_canvas_save(JSContext *cx, unsigned argc, jsval *vp)
{
    NSKIA->save();

    return JS_TRUE;
}

static JSBool native_canvas_restore(JSContext *cx, unsigned argc, jsval *vp)
{
    NSKIA->restore();

    return JS_TRUE;
}

static JSBool native_canvas_createLinearGradient(JSContext *cx,
    unsigned argc, jsval *vp)
{
    JSObject *linearObject;
    double x1, y1, x2, y2;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dddd",
        &x1, &y2, &x2, &y2)) {
        return JS_TRUE;
    }

    linearObject = JS_NewObject(cx, &canvasGradient_class, NULL, NULL);

    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(linearObject));

    JS_SetPrivate(linearObject,
        new NativeSkGradient(x1, y1, x2, y2));

    JS_DefineFunctions(cx, linearObject, gradient_funcs);

    return JS_TRUE;
}

static JSBool native_canvasGradient_addColorStop(JSContext *cx,
    unsigned argc, jsval *vp)
{
    double position;
    JSString *color;
    JSObject *caller = JS_THIS_OBJECT(cx, vp);
    NativeSkGradient *gradient;

    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "dS",
        &position, &color)) {
        return JS_TRUE;
    }

    if ((gradient = (NativeSkGradient *)JS_GetPrivate(caller)) != NULL) {
        JSAutoByteString colorstr(cx, color);

        gradient->addColorStop(position, colorstr.ptr());
    }

    return JS_TRUE;
}

static JSBool native_canvas_requestAnimationFrame(JSContext *cx,
    unsigned argc, jsval *vp)
{

    if (!JS_ConvertValue(cx, JS_ARGV(cx, vp)[0], JSTYPE_FUNCTION, &gfunc)) {
        return JS_TRUE;
    }

    return JS_TRUE;
}

void NativeJS::callFrame()
{
    jsval rval;
    if (gfunc != JSVAL_VOID) {
        JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), gfunc, 0, NULL, &rval);
    }
}

NativeJS::NativeJS()
{
    JSRuntime *rt;
    JSObject *gbl;

    JS_SetCStringsAreUTF8();

    printf("New JS runtime\n");

    if ((rt = JS_NewRuntime(1024L * 1024L * 128L)) == NULL) {
        printf("Failed to init JS runtime\n");
        return;
    }

    if ((cx = JS_NewContext(rt, 8192)) == NULL) {
        printf("Failed to init JS context\n");
        return;     
    }

    JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_METHODJIT | JSOPTION_TYPE_INFERENCE);
    JS_SetVersion(cx, JSVERSION_LATEST);

    JS_SetErrorReporter(cx, reportError);

    gbl = JS_NewGlobalObject(cx, &global_class, NULL);


    if (!JS_InitStandardClasses(cx, gbl))
        return;

    JS_SetGlobalObject(cx, gbl);
    JS_DefineFunctions(cx, gbl, glob_funcs);

    LoadCanvasObject();

    nskia = new NativeSkia();

    JS_SetContextPrivate(cx, nskia);

    animationframeCallbacks = ape_new_pool(sizeof(ape_pool_t), 8);
}

NativeJS::~NativeJS()
{
    JSRuntime *rt;
    rt = JS_GetRuntime(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    delete nskia;
}

int NativeJS::LoadScript(const char *filename)
{
    printf("Load script\n");
    JSObject *gbl = JS_GetGlobalObject(cx);

    JSScript *script = JS_CompileUTF8File(cx, gbl, filename);

    if (script == NULL) {
        return 0;
    }

    JS_ExecuteScript(cx, gbl, script, NULL);

    return 1;
}

void NativeJS::LoadCanvasObject()
{
    JSObject *gbl = JS_GetGlobalObject(cx);
    JSObject *canvasObj;

    canvasObj = JS_DefineObject(cx, gbl, "canvas", &canvas_class, NULL, 0);
    JS_DefineFunctions(cx, canvasObj, canvas_funcs);

    JS_DefineProperties(cx, canvasObj, canvas_props);
}

