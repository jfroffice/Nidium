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

#ifndef nativehttplistener_h__
#define nativehttplistener_h__

#include <native_netlib.h>
#include <ape_array.h>
#include <http_parser.h>
#include <stdio.h>
#include <NativeMessages.h>
#include <NativeEvents.h>

#define HTTP_MAX_CL 1024L*1024L*1024L*2L
#define HTTP_DEFAULT_TIMEOUT 15000

class NativeMessages;
class NativeHTTPClientConnection;

class NativeHTTPListener : public NativeEvents
{
public:
    static const uint8_t EventID = 3;

    enum Events {
        NONE_EVENT = 1
    };

    NativeHTTPListener(uint16_t port, const char *ip = "0.0.0.0");
    virtual ~NativeHTTPListener();
    bool start();
    void stop();

    ape_socket *getSocket() const {
        return m_Socket;
    }

    /*
        Subclasses can override this in order to set
        their own NativeHTTPClientConnection subclass.

        This method must set a NativeHTTPClientConnection on client->ctx.
        NativeHTTPClientConnection is then automatically
        deleted when the socket disconnect.

    */
    virtual void onClientConnect(ape_socket *client, ape_global *ape);

    /*
        Callbacks for subclasses
    */
    virtual void onClientConnect(NativeHTTPClientConnection *client){};
    virtual void onClientDisconnect(NativeHTTPClientConnection *client){};
    virtual void onData(NativeHTTPClientConnection *client, const char *buf, size_t len){};

    /* return true to close the connection */
    virtual bool onEnd(NativeHTTPClientConnection *client){
        return true;
    };

private:
    ape_socket *      m_Socket;
    char *            m_IP;
    uint16_t          m_Port;
};

class NativeHTTPClientConnection
{
public:
    NativeHTTPClientConnection(NativeHTTPListener *httpserver,
        ape_socket *socket);
    virtual ~NativeHTTPClientConnection();

    enum PrevState {
        PSTATE_NOTHING,
        PSTATE_FIELD,
        PSTATE_VALUE
    };
    struct HTTPData {
        http_parser parser;
        struct {
            ape_array_t *list;
            buffer *tkey;
            buffer *tval;
            PrevState prevstate;
        } headers;
        int ended;
        uint64_t contentlength;
        buffer *data;
    };

    struct HTTPData *getHTTPState() {
        return &m_HttpState;
    }

    ape_socket *getSocket() const {
        return m_SocketClient;
    }

    NativeHTTPListener *getHTTPListener() const {
        return m_HTTPListener;
    }

    const buffer &getData() const {
        return *m_HttpState.data;
    }

    void resetData() {
        if (m_HttpState.data == NULL) {
            return;
        }
        m_HttpState.data->used = 0;
    }

    void onRead(buffer *buf, ape_global *ape);

    virtual void onHeaderEnded(){};
    virtual void onDisconnect(ape_global *ape){};
    virtual void onUpgrade(const char *to){};
    virtual void onContent(const char *data, size_t len){};

    virtual void close();
protected:
    struct HTTPData m_HttpState;
    ape_socket *m_SocketClient;
    NativeHTTPListener *m_HTTPListener;
};

#endif