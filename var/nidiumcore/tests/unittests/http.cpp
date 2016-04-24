/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <Net/HTTP.h>

TEST(Http, Request)
{
    char * found = NULL;
    const char * cache = "no-cache";
    const char * key = "Pragma";
    const char * dat = "test=true";
    Nidium::Net::HTTPRequest na("http://nidium.com/new.html");

    na.setHeader(key, cache);
    found = (char*) na.getHeader(key);
    EXPECT_TRUE(strcmp(found, cache) == 0);

    found = NULL;
    na.recycle();
    found = (char*)na.getHeader(key);
    EXPECT_TRUE(found == NULL);

    EXPECT_EQ(na.getPort(), 80);
    EXPECT_TRUE(na.isSSL() == false);
    EXPECT_TRUE(na.isValid() == true);
    EXPECT_TRUE(strcmp(na.getHost(), "nidium.com") == 0);
    EXPECT_TRUE(strcmp(na.getPath(), "/new.html") == 0);

    na.setPath("/newest.html");
    EXPECT_TRUE(strcmp(na.getPath(), "/newest.html") == 0);

    na.setData((char*)dat, strlen(dat));
    EXPECT_TRUE(strcmp(na.getData(), dat) == 0);
    EXPECT_EQ(na.getDataLength(), strlen(dat));

    na.resetURL("https://nidium.org:8080");
    EXPECT_TRUE(strcmp(na.getHost(), "nidium.org") == 0);
    EXPECT_EQ(na.getPort(), 8080);
    EXPECT_TRUE(na.isSSL() == true);
    EXPECT_TRUE(na.isValid() == true);
    EXPECT_TRUE(strcmp(na.getPath(), "/") == 0);
}

TEST(Http, Http)
{
    Nidium::Net::HTTPRequest *request;
    ape_global *g_ape = APE_init();
    Nidium::Net::HTTP nm(g_ape);
    nm.http.headers.list = ape_array_new(1); // this should happen in the constructor....

    EXPECT_TRUE(nm.net == g_ape);
    EXPECT_TRUE(nm.m_CurrentSock == NULL);
    EXPECT_EQ(nm.err, 0);

    EXPECT_EQ(nm.m_Timeout, HTTP_DEFAULT_TIMEOUT);
    EXPECT_EQ(nm.m_TimeoutTimer, 0);
    EXPECT_TRUE(nm.delegate == NULL);
    EXPECT_TRUE(nm.nidium_http_data_type == Nidium::Net::HTTP::DATA_NULL);

    nm.parsing(true);
    EXPECT_TRUE(nm.isParsing() == true);
    nm.parsing(false);
    EXPECT_TRUE(nm.isParsing() == false);

    EXPECT_EQ(nm.getFileSize(), 0);
    EXPECT_EQ(nm.getStatusCode(), 0);

    EXPECT_TRUE(nm.hasPendingError() == false);

    nm.setMaxRedirect(2);

    request = nm.getRequest();
    EXPECT_TRUE(request == NULL);

    nm.setPendingError(Nidium::Net::HTTP::ERROR_RESPONSE);

    EXPECT_TRUE(nm.hasPendingError() == true);
    EXPECT_TRUE(nm.canDoRequest() == false);
    nm.clearPendingError();
    EXPECT_TRUE(nm.hasPendingError() == false);
    nm.canDoRequest(false);
    EXPECT_TRUE(nm.canDoRequest() == false);
    nm.canDoRequest(true);
    EXPECT_TRUE(nm.canDoRequest() == true);

    //keepalive
    EXPECT_TRUE(nm.isKeepAlive() == true);
}

