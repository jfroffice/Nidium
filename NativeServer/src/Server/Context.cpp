/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#define _HAVE_SSL_SUPPORT 1
#include "Server/Context.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <IO/FileStream.h>
#include <Net/HTTPStream.h>

#include "linenoise.h"

using Nidium::Core::Path;
using Nidium::Core::TaskManager;
using Nidium::Core::Messages;
using Nidium::Binding::NidiumJS;

namespace Nidium {
namespace Server {


// {{{ Context
Context::Context(ape_global *net, Worker *worker,
    bool jsstrict, bool runInREPL) :
    Core::Context(net), m_Worker(worker), m_RunInREPL(runInREPL)
{
    char cwd[PATH_MAX];

    memset(&cwd[0], '\0', sizeof(cwd));
    if (getcwd(cwd, sizeof(cwd)-1) != NULL) {
        strcat(cwd, "/");

        Path::CD(cwd);
        Path::Chroot("/");
    } else {
        fprintf(stderr, "[Warn] Failed to get current working directory\n");
    }

    m_JS->setStrictMode(jsstrict);
    m_JS->setPath(Path::GetCwd());

}

void Context::log(const char *str)
{
    linenoisePause();
    Core::Context::log(str);
    linenoiseResume();
}

Context::~Context()
{

}
// }}}

} // namespace Server
} // namespace Nidium

