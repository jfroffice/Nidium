/*
    NativeJS Core Library
    Copyright (C) 2013 Anthony Catel <paraboul@gmail.com>
    Copyright (C) 2013 Nicolas Trani <n.trani@weelya.com>

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

#ifndef nativemessages_h__
#define nativemessages_h__

#include <NativeSharedMessages.h>

typedef struct _ape_global ape_global;

class NativeMessages
{
public:
    virtual ~NativeMessages()=0;
    virtual void onMessage(const NativeSharedMessages::Message &msg);
    virtual void onMessageLost(const NativeSharedMessages::Message &msg);
    void postMessage(void *dataptr, int event);
    void postMessage(uint64_t dataint, int event);
    void postMessage(NativeSharedMessages::Message *msg);
    static void initReader(ape_global *ape);
    static void destroyReader();

    NativeSharedMessages *getSharedMessages();
};

#endif