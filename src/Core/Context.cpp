/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "Core/Context.h"
#include "Core/Path.h"
#include "Core/TaskManager.h"
#include "Core/Messages.h"
#include "Net/HTTPStream.h"
#include "IO/FileStream.h"

using namespace Nidium::Binding;
using namespace Nidium::Core;
using namespace Nidium::IO;
using namespace Nidium::Net;

class NidiumJS *g_nidiumjs = nullptr;

namespace Nidium {
namespace Core {

Context::Context(ape_global *ape) : m_APECtx(ape)
{
    m_JS = g_nidiumjs = new NidiumJS(ape, this);

    Path::RegisterScheme(SCHEME_DEFINE("file://", FileStream, false), true);
    Path::RegisterScheme(SCHEME_DEFINE("http://", HTTPStream, true));
    Path::RegisterScheme(SCHEME_DEFINE("https://", HTTPStream, true));

    TaskManager::CreateManager();
    Messages::InitReader(ape);

    m_JS->loadGlobalObjects();

    m_PingTimer = APE_timer_create(ape, 1, Ping, (void *)m_JS);
}

int Context::Ping(void *arg)
{
    static uint64_t framecount = 0;
    NidiumJS *js               = static_cast<NidiumJS *>(arg);

    if (++framecount % 1000 == 0) {
        js->gc();
    }

    return 8;
}

void Context::logFlush()
{
    m_LogBuffering = false;

    if (m_Logbuffer.length()) {
        this->postMessage(strdup(m_Logbuffer.c_str()), kContextMessage_log);
        m_Logbuffer.clear();
        m_Logbuffer.shrink_to_fit();
    }
}

void Context::log(const char *str)
{
    if (m_LogBuffering) {
        m_Logbuffer += str;

        return;
    }
    this->postMessage(strdup(str), kContextMessage_log);
}

void Context::vlog(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    this->vlog(format, args);
    va_end(args);
}

void Context::vlog(const char *format, va_list args)
{
    char *buff;

    vasprintf(&buff, format, args);

    this->log(buff);

    free(buff);
}

Context::~Context()
{
    APE_timer_destroy(m_APECtx, m_PingTimer);
    destroyJS();

    /*
        We need to cleanup the message queue beforehand
        because the Messages base classe would try to call "delMessageForDest"
        while the object was already cleaned up by ~DestroyReader()
    */
    Messages::cleanupMessages();

    Messages::DestroyReader();
}

void Context::onMessage(const SharedMessages::Message &msg)
{
    switch (msg.event()) {
        case kContextMessage_log:
        {
            const char *str = (char *)msg.dataPtr();
            fwrite(str, 1, strlen(str), stdout);

            free(msg.dataPtr());
        }
        default:
        break;
    }
}

void Context::onMessageLost(const SharedMessages::Message &msg)
{
    switch (msg.event()) {
        case kContextMessage_log:
        {
            free(msg.dataPtr());
        }
        default:
        break;
    }
}


} // namespace Core
} // namespace Nidium
