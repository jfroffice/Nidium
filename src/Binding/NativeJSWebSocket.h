/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef nativejswebsocket_h__
#define nativejswebsocket_h__

#include "Core/NativeMessages.h"
#include "Net/NativeWebSocket.h"
#include "JSExposer.h"

class NativeJSWebSocketServer : public Nidium::Binding::JSExposer<NativeJSWebSocketServer>,
                                public NativeMessages
{
public:
    NativeJSWebSocketServer(JS::HandleObject obj, JSContext *cx,
        const char *host, unsigned short port);
    ~NativeJSWebSocketServer();
    bool start();
    static void registerObject(JSContext *cx);
    void onMessage(const NativeSharedMessages::Message &msg);
private:
    NativeWebSocketListener *m_WebSocketServer;

    JSObject *createClient(NativeWebSocketClientConnection *client);
};

#endif

