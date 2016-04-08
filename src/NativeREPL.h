/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef nativerepl_h__
#define nativerepl_h__

#include <native_netlib.h>
#include <NativeMessages.h>

#include <ape_buffer.h>

class NativeJS;

class NativeREPL : public NativeMessages
{
public:
    NativeREPL(NativeJS *js);
    ~NativeREPL();
    void onMessage(const NativeSharedMessages::Message &msg);
    void onMessageLost(const NativeSharedMessages::Message &msg);
private:
    pthread_t m_ThreadHandle;
    NativeJS *m_JS;

    buffer *m_Buffer;
};

#endif

