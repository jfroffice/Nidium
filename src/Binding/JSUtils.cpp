/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "Binding/JSUtils.h"

#include <string.h>
#include <strings.h>

#include <jsfriendapi.h>
#include <js/CharacterEncoding.h>

#include "Binding/NidiumJS.h"

namespace Nidium {
namespace Binding {

// {{{ String conversions
bool JSUtils::StrToJsval(JSContext *cx, const char *buf, size_t len, JS::MutableHandleValue ret,
    const char *encoding)
{
    ret.setNull();

    if (encoding) {

        JS::RootedString str(cx, JSUtils::NewStringWithEncoding(cx, buf, len, encoding));
        if (!str) {
            ret.set(JS_GetEmptyStringValue(cx));
            return false;
        }

        ret.setString(str);

    } else {
        JS::RootedObject arrayBuffer(cx, JS_NewArrayBuffer(cx, len));

        if (arrayBuffer == NULL) {
            JS_ReportOutOfMemory(cx);
            return false;
        } else {
            uint8_t *adata = JS_GetArrayBufferData(arrayBuffer);
            memcpy(adata, buf, len);

            ret.setObject(*arrayBuffer);
        }
    }

    return true;
}

char16_t *JSUtils::Utf8ToUtf16(JSContext *cx,
  const char *str, size_t len, size_t *outputlen)
{
    return JS::LossyUTF8CharsToNewTwoByteCharsZ(cx,
      JS::UTF8Chars(str, len), outputlen).get();
}

JSString *JSUtils::NewStringWithEncoding(JSContext *cx, const char *buf,
        size_t len, const char *encoding)
{

    if (encoding != NULL && strcasecmp(encoding, "utf8") == 0) {
        size_t jlen = 0;

        Core::PtrAutoDelete<char16_t *> content(JSUtils::Utf8ToUtf16(cx, buf, len, &jlen), free);

        if (content.ptr() == NULL) {
            JS_ReportError(cx, "Could not decode string to utf8");
            return NULL;
        }

        JS::RootedString str(cx, JS_NewUCString(cx, content.ptr(), jlen));
        if (str.get() == NULL) {
            return NULL;
        }

        content.disable(); /* JS_NewUCString took ownership */

        return str;

    }

    return JS_NewStringCopyN(cx, buf, len);
}
// }}}

// {{{ JS
char * JSUtils::CurrentJSCaller(JSContext *cx)
{
    if (cx == NULL) {
        /* lookup in the TLS */
        NidiumJS *js = NidiumJS::GetObject();
        if (!js || (cx = js->getJSContext()) == NULL) {
            return NULL;
        }
    }

    unsigned lineno;

    JS::AutoFilename af;
    JS::DescribeScriptedCaller(cx, &af, &lineno);

    return strdup(af.get());
}
// }}}

} // namespace Binding
} // namespace Nidium

