/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "Server/Server.h"

using namespace Nidium::Server;

int main(int argc, char **argv)
{
    return Server::Start(argc, argv);
}

