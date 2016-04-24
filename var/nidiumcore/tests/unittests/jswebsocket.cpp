/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <Binding/JSWebSocket.h>

TEST(JSWebsocket, Simple)
{
    ape_global * g_ape = APE_init();
    Nidium::Binding::NidiumJS njs(g_ape);
    bool success;

    JS::RootedObject globObj(njs.cx, JS::CurrentGlobalOrNull(njs.cx));
    JS::RootedValue rval(njs.cx, JSVAL_VOID);
    success = JS_GetProperty(njs.cx, globObj, "WebSocketServer", &rval);
    EXPECT_TRUE(JSVAL_IS_VOID(rval) == true);
    success = JS_GetProperty(njs.cx, globObj, "WebSocketServerClient", &rval);
    EXPECT_TRUE(JSVAL_IS_VOID(rval) == true);

    Nidium::Binding::JSWebSocketServer::RegisterObject(njs.cx);

    rval = JSVAL_VOID;
    success = JS_GetProperty(njs.cx, globObj, "WebSocketServer", &rval);
    EXPECT_TRUE(success == true);
    EXPECT_TRUE(JSVAL_IS_VOID(rval) == false);

    rval = JSVAL_VOID;
    success = JS_GetProperty(njs.cx, globObj, "WebSocketServerClient", &rval);
    EXPECT_TRUE(success == true);
    EXPECT_TRUE(JSVAL_IS_VOID(rval) == true);

    ape_running = 0;
    APE_loop_run(g_ape);
    APE_destroy(g_ape);
}

TEST(JSWebsocket, Init)
{
    ape_global * g_ape = APE_init();
    Nidium::Binding::NidiumJS njs(g_ape);

    JS::RootedObject globObj(njs.cx, JS::CurrentGlobalOrNull(njs.cx));
    Nidium::Binding::JSWebSocketServer nws(globObj, njs.cx, "0.0.0.0", 8888);

    EXPECT_TRUE(nws.getJSObject() == globObj);
    EXPECT_TRUE(nws.getJSContext() == njs.cx);
}

