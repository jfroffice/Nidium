#include "NativeHTTP.h"
//#include "ape_http_parser.h"
#include <http_parser.h>
#include <native_netlib.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifndef ULLONG_MAX
# define ULLONG_MAX ((uint64_t) -1) /* 2^64-1 */
#endif

#define HTTP_PREFIX "http://"
#define SOCKET_WRITE_STATIC(data) APE_socket_write(s, \
    (unsigned char *)CONST_STR_LEN(data), APE_DATA_STATIC)

#define SOCKET_WRITE_OWN(data) APE_socket_write(s, (unsigned char *)data, \
    strlen(data), APE_DATA_OWN)

static struct native_http_mime {
    const char *str;
    NativeHTTP::DataType data_type;
} native_mime[] = {
    {"text/plain",              NativeHTTP::DATA_STRING},
    {"application/x-javascript",NativeHTTP::DATA_STRING},
    {"application/octet-stream",NativeHTTP::DATA_STRING},
    {"image/jpeg",              NativeHTTP::DATA_IMAGE},
    {"image/png",               NativeHTTP::DATA_IMAGE},
    {"application/json",        NativeHTTP::DATA_JSON},
    {"text/html",               NativeHTTP::DATA_STRING}, /* TODO: use dom.js */
    {"application/octet-stream",NativeHTTP::DATA_BINARY},
    {NULL,                      NativeHTTP::DATA_END}
};

static int message_begin_cb(http_parser *p);
static int headers_complete_cb(http_parser *p);
static int message_complete_cb(http_parser *p);
static int header_field_cb(http_parser *p, const char *buf, size_t len);
static int header_value_cb(http_parser *p, const char *buf, size_t len);
static int request_url_cb(http_parser *p, const char *buf, size_t len);
static int body_cb(http_parser *p, const char *buf, size_t len);


static http_parser_settings settings =
{
.on_message_begin = message_begin_cb,
.on_header_field = header_field_cb,
.on_header_value = header_value_cb,
.on_url = request_url_cb,
.on_body = body_cb,
.on_headers_complete = headers_complete_cb,
.on_message_complete = message_complete_cb
};

static int message_begin_cb(http_parser *p)
{
    NativeHTTP *nhttp = (NativeHTTP *)p->data;

    nhttp->clearTimeout();

    return 0;
}

static int headers_complete_cb(http_parser *p)
{
    NativeHTTP *nhttp = (NativeHTTP *)p->data;

    if (nhttp->http.headers.tval != NULL) {
        buffer_append_char(nhttp->http.headers.tval, '\0');
    }

    if (p->content_length == ULLONG_MAX) {
        nhttp->http.contentlength = 0;
        nhttp->headerEnded();
        return 0;
    }

    if (p->content_length > HTTP_MAX_CL) {
        return -1;
    }

    if (p->content_length) nhttp->http.data = buffer_new(p->content_length);

    nhttp->http.contentlength = p->content_length;
    nhttp->headerEnded();

    return 0;
}

static int message_complete_cb(http_parser *p)
{
    NativeHTTP *nhttp = (NativeHTTP *)p->data;

    nhttp->requestEnded();

    return 0;
}

static int header_field_cb(http_parser *p, const char *buf, size_t len)
{
    NativeHTTP *nhttp = (NativeHTTP *)p->data;

    switch (nhttp->http.headers.prevstate) {
        case NativeHTTP::PSTATE_NOTHING:
            nhttp->http.headers.list = ape_array_new(16);
        case NativeHTTP::PSTATE_VALUE:
            nhttp->http.headers.tkey = buffer_new(16);
            if (nhttp->http.headers.tval != NULL) {
                buffer_append_char(nhttp->http.headers.tval, '\0');
            }
            break;
        default:
            break;
    }

    nhttp->http.headers.prevstate = NativeHTTP::PSTATE_FIELD;

    if (len != 0) {
        buffer_append_data(nhttp->http.headers.tkey,
            (const unsigned char *)buf, len);
    }

    return 0;
}

static int header_value_cb(http_parser *p, const char *buf, size_t len)
{
    NativeHTTP *nhttp = (NativeHTTP *)p->data;

    switch (nhttp->http.headers.prevstate) {
        case NativeHTTP::PSTATE_NOTHING:
            return -1;
        case NativeHTTP::PSTATE_FIELD:
            nhttp->http.headers.tval = buffer_new(64);
            buffer_append_char(nhttp->http.headers.tkey, '\0');
            ape_array_add_b(nhttp->http.headers.list,
                    nhttp->http.headers.tkey, nhttp->http.headers.tval);
            break;
        default:
            break;
    }

    nhttp->http.headers.prevstate = NativeHTTP::PSTATE_VALUE;

    if (len != 0) {
        buffer_append_data(nhttp->http.headers.tval,
            (const unsigned char *)buf, len);
    }
    return 0;
}

static int request_url_cb(http_parser *p, const char *buf, size_t len)
{
    printf("Request URL cb\n");
    return 0;
}

static int body_cb(http_parser *p, const char *buf, size_t len)
{
    NativeHTTP *nhttp = (NativeHTTP *)p->data;

    if (nhttp->http.data == NULL) {
        nhttp->http.data = buffer_new(2048);
    }

    if (nhttp->http.data->used+len > HTTP_MAX_CL) {
        return -1;
    }

    if (len != 0) {
        buffer_append_data(nhttp->http.data,
            (const unsigned char *)buf, len);
    }

    nhttp->onData(nhttp->http.data->used - len, len);

    return 0;
}


static void native_http_connected(ape_socket *s, ape_global *ape)
{
    NativeHTTP *nhttp = (NativeHTTP *)s->ctx;

    if (nhttp == NULL) return;

    nhttp->http.headers.list = NULL;
    nhttp->http.headers.tkey = NULL;
    nhttp->http.headers.tval = NULL;
    nhttp->http.data = NULL;
    nhttp->http.ended = 0;
    nhttp->http.headers.prevstate = NativeHTTP::PSTATE_NOTHING;

    http_parser_init(&nhttp->http.parser, HTTP_RESPONSE);
    nhttp->http.parser.data = nhttp;

    SOCKET_WRITE_STATIC("GET ");
    SOCKET_WRITE_OWN(nhttp->path);
    SOCKET_WRITE_STATIC(" HTTP/1.1\nHost: ");
    SOCKET_WRITE_OWN(nhttp->host);
    SOCKET_WRITE_STATIC("\nUser-Agent: Nativestudio/1.0\nConnection: close\n\n");
}

static void native_http_disconnect(ape_socket *s, ape_global *ape)
{
    NativeHTTP *nhttp = (NativeHTTP *)s->ctx;

    if (nhttp == NULL) return;

    nhttp->clearTimeout();

    http_parser_execute(&nhttp->http.parser, &settings,
        NULL, 0);

    nhttp->currentSock = NULL;

    if (!nhttp->http.ended) {
        nhttp->delegate->onError(NativeHTTP::ERROR_DISCONNECTED);
    }
}

static void native_http_read(ape_socket *s, ape_global *ape)
{
    size_t nparsed;
    NativeHTTP *nhttp = (NativeHTTP *)s->ctx;

    if (nhttp == NULL) return;

    nparsed = http_parser_execute(&nhttp->http.parser, &settings,
        (const char *)s->data_in.data, (size_t)s->data_in.used);

    if (nparsed != s->data_in.used) {
        printf("Parser returned %ld with error %s\n", nparsed,
            http_errno_description(HTTP_PARSER_ERRNO(&nhttp->http.parser)));

        nhttp->delegate->onError(NativeHTTP::ERROR_RESPONSE);

        APE_socket_shutdown_now(s);
    }
}

NativeHTTP::NativeHTTP(const char *url, ape_global *n) :
    ptr(NULL), net(n), currentSock(NULL),
    host(NULL), path(NULL), port(0),
    err(0), timeout(HTTP_DEFAULT_TIMEOUT), timeoutTimer(0), delegate(NULL)
{
    size_t url_len = strlen(url);
    char *durl = (char *)malloc(sizeof(char) * (url_len+1));

    memcpy(durl, url, url_len+1);

    host = (char *)malloc(url_len+1);
    path = (char *)malloc(url_len+1);
    memset(host, 0, url_len+1);
    memset(path, 0, url_len+1);

    if (ParseURI(durl, url_len, host, &port, path) == -1) {
        err = 1;
        memset(host, 0, url_len+1);
        memset(path, 0, url_len+1);
        port = 0;
    }

    http.data = NULL;
    http.headers.tval = NULL;
    http.headers.tkey = NULL;
    http.headers.list = NULL;
    http.ended = 0;
    http.contentlength = 0;

    native_http_data_type = DATA_NULL;

    free(durl);
}

void NativeHTTP::setPrivate(void *ptr)
{
    this->ptr = ptr;
}

void *NativeHTTP::getPrivate()
{
    return this->ptr;
}

void NativeHTTP::onData(size_t offset, size_t len)
{
    this->delegate->onProgress(offset, len,
        &this->http, this->native_http_data_type);
}

void NativeHTTP::headerEnded()
{
#define REQUEST_HEADER(header) ape_array_lookup(http.headers.list, \
    CONST_STR_LEN(header "\0"))

    if (http.headers.list != NULL) {
        buffer *content_type;

        if ((content_type = REQUEST_HEADER("Content-Type")) != NULL &&
            content_type->used > 3) {
            int i;

            for (i = 0; native_mime[i].str != NULL; i++) {
                if (strncasecmp(native_mime[i].str, (const char *)content_type->data,
                    strlen(native_mime[i].str)) == 0) {
                    native_http_data_type = native_mime[i].data_type;
                    break;
                }
            }
        }

    }
#undef REQUEST_HEADER
}

void NativeHTTP::stopRequest()
{
    this->clearTimeout();
    
    if (!http.ended) {
        if (http.headers.list) {
            ape_array_destroy(http.headers.list);
        }

        if (http.data) buffer_destroy(http.data);

        http.data = NULL;
        http.headers.tval = NULL;
        http.headers.tkey = NULL;
        http.headers.list = NULL;

        if (currentSock) {
            APE_socket_shutdown_now(currentSock);
        }

        this->delegate->onError(ERROR_TIMEOUT);
    }
}

void NativeHTTP::requestEnded()
{
    if (!http.ended) {
        http.ended = 1;

        delegate->onRequest(&http, native_http_data_type);

        if (http.headers.list) {
            ape_array_destroy(http.headers.list);
        }

        if (http.data) {
            buffer_destroy(http.data);
        }
        http.data = NULL;
        http.headers.tval = NULL;
        http.headers.tkey = NULL;
        http.headers.list = NULL;

        if (currentSock) {
            APE_socket_shutdown(currentSock);
        }
    } 
}

static int NativeHTTP_handle_timeout(void *arg)
{
    ((NativeHTTP *)arg)->stopRequest();

    return 0;
}

void NativeHTTP::clearTimeout()
{
    if (this->timeoutTimer) {
        clear_timer_by_id(&net->timersng, this->timeoutTimer, 1);
        this->timeoutTimer = 0;
    }
}

int NativeHTTP::request(NativeHTTPDelegate *delegate)
{
    ape_socket *socket;

    if ((socket = APE_socket_new(APE_SOCKET_PT_TCP, 0, net)) == NULL) {
        printf("[Socket] Cant load socket (new)\n");
        this->delegate->onError(ERROR_SOCKET);
        return 0;
    }

    if (APE_socket_connect(socket, port, host) == -1) {
        printf("[Socket] Cant connect (0)\n");
        this->delegate->onError(ERROR_SOCKET);
        return 0;
    }

    socket->callbacks.on_connected = native_http_connected;
    socket->callbacks.on_read = native_http_read;
    socket->callbacks.on_disconnect = native_http_disconnect;

    http.ended = 0;
    socket->ctx = this;
    this->delegate = delegate;
    this->currentSock = socket;
    delegate->httpref = this;

    if (timeout) {
        ape_timer *ctimer;
        ctimer = add_timer(&net->timersng, timeout,
            NativeHTTP_handle_timeout, this);

        ctimer->flags &= ~APE_TIMER_IS_PROTECTED;
        timeoutTimer = ctimer->identifier;
    }

    return 1;
}

NativeHTTP::~NativeHTTP()
{
    free(host);
    free(path);

    if (currentSock != NULL) {
        currentSock->ctx = NULL;

        APE_socket_shutdown_now(currentSock);
    }

    if (timeoutTimer) {
        this->clearTimeout();
    }

    if (!http.ended) {
        if (http.headers.list) ape_array_destroy(http.headers.list);
        if (http.data) buffer_destroy(http.data);
    }
}


int NativeHTTP::ParseURI(char *url, size_t url_len, char *host,
    u_short *port, char *file)
{
    char *p;
    const char *p2;
    int len;

    len = strlen(HTTP_PREFIX);
    if (strncasecmp(url, HTTP_PREFIX, len)) {
        return -1;
    }

    url += len;
    
    memcpy(host, url, (url_len-len));

    p = strchr(host, '/');
    if (p != NULL) {
        *p = '\0';
        p2 = p + 1;
    } else {
        p2 = NULL;
    }
    if (file != NULL) {
        /* Generate request file */
        if (p2 == NULL)
            p2 = "";
        sprintf(file, "/%s", p2);
    }

    p = strchr(host, ':');

    if (p != NULL) {
        *p = '\0';
        *port = atoi(p + 1);

        if (*port == 0)
            return -1;
    } else
        *port = 80;

    return 0;
}
