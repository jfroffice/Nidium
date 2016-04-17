/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef nativewebsocket_h__
#define nativewebsocket_h__

#include <ape_websocket.h>

#include "HTTPServer.h"

#define NATIVEWEBSOCKET_PING_INTERVAL 5000 /* ms */

class NativeWebSocketListener : public Nidium::Net::HTTPServer
{
public:
    static const uint8_t EventID = 4;

    enum Events {
        SERVER_CONNECT = 1,
        SERVER_FRAME,
        SERVER_CLOSE
    };

    NativeWebSocketListener(uint16_t port, const char *ip = "0.0.0.0");
    virtual void onClientConnect(ape_socket *client, ape_global *ape);

    virtual bool onEnd(Nidium::Net::HTTPClientConnection *client) override {
        return false;
    };
};

class NativeWebSocketClientConnection : public Nidium::Net::HTTPClientConnection
{
public:
    NativeWebSocketClientConnection(Nidium::Net::HTTPServer *httpserver,
        ape_socket *socket);
    ~NativeWebSocketClientConnection();

    virtual void onFrame(const char *data, size_t len, bool binary);

    /* TODO: support "buffering" detection + ondrain()
        (need ape_websocket.c modification)
    */
    void write(unsigned char *data, size_t len,
        bool binary = false,
        ape_socket_data_autorelease type = APE_DATA_COPY);

    virtual void onHeaderEnded();
    virtual void onDisconnect(ape_global *ape);
    virtual void onUpgrade(const char *to);
    virtual void onContent(const char *data, size_t len);

    virtual void setData(void *data) {
        m_Data = data;
    }
    virtual void *getData() const {
        return m_Data;
    }
    virtual void close();
    void ping();

    static int pingTimer(void *arg);
private:
    websocket_state m_WSState;
    bool m_Handshaked;
    uint64_t m_PingTimer;

    void *m_Data;
};

#endif

