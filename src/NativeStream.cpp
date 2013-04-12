#include <stdlib.h>
#include <string.h>
#include "NativeStream.h"
#include "NativeHTTP.h"
#include "NativeFileIO.h"
#include <ape_base64.h>
#include "NativeUtils.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

static struct _native_stream_interfaces {
    const char *str;
    NativeStream::StreamInterfaces interface_type;
} native_stream_interfaces[] = {
    {"http://",      NativeStream::INTERFACE_HTTP},
    {"file://",      NativeStream::INTERFACE_FILE},
    {"private://",   NativeStream::INTERFACE_PRIVATE},
    {"data:",        NativeStream::INTERFACE_DATA},
    {NULL,           NativeStream::INTERFACE_UNKNOWN}
};

NativeStream::NativeStream(ape_global *net, const char *location) :
    packets(0), needToSendUpdate(false)
{
    this->interface = NULL;
    this->location  = strdup(location);
    this->net = net;
    this->IInterface = INTERFACE_UNKNOWN;
    this->delegate  = NULL;

    dataBuffer.back = NULL;
    dataBuffer.front = NULL;
    dataBuffer.alreadyRead = true;
    dataBuffer.fresh = true;

    mapped.addr = NULL;
    mapped.fd   = 0;
    mapped.size = 0;

    this->getInterface();
}

NativeIStreamer *NativeStream::getInterface()
{
    if (interface) {
        return interface;
    }

    for (int i = 0; native_stream_interfaces[i].str != NULL; i++) {
        int len = strlen(native_stream_interfaces[i].str);
        if (strncasecmp(native_stream_interfaces[i].str, (const char *)location,
                        len) == 0) {
            
            setInterface(native_stream_interfaces[i].interface_type, len);

            return interface;
        }
    }

    /* Default : use file */
    setInterface(INTERFACE_FILE, 0);

    return interface;
}

void NativeStream::setInterface(StreamInterfaces interface, int path_offset)
{
    this->IInterface = interface;

    switch(interface) {
        case INTERFACE_HTTP:
            this->interface = new NativeHTTP(
                new NativeHTTPRequest(this->location), this->net);
            break;
        case INTERFACE_PRIVATE:
        {
#define FRAMEWORK_LOCATION "./falcon/"
            char *flocation = (char *)malloc(sizeof(char) *
                    (strlen(&this->location[path_offset]) + sizeof(FRAMEWORK_LOCATION) + 1));
            sprintf(flocation, FRAMEWORK_LOCATION "%s", &this->location[path_offset]);
            this->interface = new NativeFileIO(&flocation[path_offset],
                                this, this->net);
            break;
#undef FRAMEWORK_LOCATION
        }
        case INTERFACE_FILE:
            this->interface = new NativeFileIO(&this->location[path_offset],
                                this, this->net);
            break;
        default:
            break;
    }
}

static int NativeStream_data_getContent(void *arg)
{
    NativeStream *stream = (NativeStream *)arg;
    const char *data = strchr(stream->location, ',');
    if (data != NULL && data[1] != '\0') {
        int len = strlen(&data[1]);
        unsigned char *out = (unsigned char *)malloc(sizeof(char) * (len + 1));
        int res = base64_decode(out, &data[1], len + 1);
        if (res > 0 && stream->delegate) {
            stream->delegate->onGetContent((const char *)out, res);
        }
        free(out);
    }
    return 0;
}

void NativeStream::start(size_t packets)
{
    this->packets = packets;

    needToSendUpdate = true;

    switch(IInterface) {
        case INTERFACE_FILE:
        {
            if (dataBuffer.back == NULL) {
                dataBuffer.back = buffer_new(packets);
                dataBuffer.front    = buffer_new(packets);
            }
            NativeFileIO *file = static_cast<NativeFileIO *>(this->interface);
            file->open("r");
            break;
        }
        case INTERFACE_HTTP:
        {
            if (dataBuffer.back == NULL) {
                dataBuffer.back     = buffer_new(0);
                dataBuffer.front    = buffer_new(0);
            }
            mapped.fd = open("/tmp/nativesstream.data",
                O_RDWR | O_CREAT | O_TRUNC, S_IRUSR| S_IWUSR);
            if (mapped.fd == 0) {
                return;
            }

            NativeHTTP *http = static_cast<NativeHTTP *>(this->interface);
            http->request(this);            
            break;
        }
        default:
            break;
    }
}

/* Sync read in memory the current packet and start grabbing the next one
   data remains readable until the next getNextPacket().
*/
const unsigned char *NativeStream::getNextPacket(size_t *len, int *err)
{
    unsigned char *data;

    if (dataBuffer.back == NULL) {
        *len = 0;
        *err = STREAM_ERROR;
        return NULL;
    }
    needToSendUpdate = true;

    if (!this->hasDataAvailable()) {
        *len = 0;
        *err = STREAM_EAGAIN;
        return NULL;
    }

    data = dataBuffer.back->data;

    *err = 0;
    *len = native_min(dataBuffer.back->used, this->getPacketSize());
    
    this->swapBuffer();

    dataBuffer.alreadyRead = true;

    switch(IInterface) {
        case INTERFACE_FILE:
        {
            NativeFileIO *file = static_cast<NativeFileIO *>(this->interface);
            file->read(this->getPacketSize());
            break;
        }
        default:
            break;
    }

    return data;
}

void NativeStream::swapBuffer()
{
    buffer *tmp = dataBuffer.back;
    dataBuffer.back = dataBuffer.front;
    dataBuffer.front = tmp;
    dataBuffer.fresh = true;

    if (dataBuffer.front->used > this->getPacketSize()) {
        dataBuffer.back->data = &dataBuffer.front->data[this->getPacketSize()];
        dataBuffer.back->used = dataBuffer.front->used - this->getPacketSize();
        dataBuffer.back->size = dataBuffer.back->used;
        dataBuffer.fresh = false;

        if (dataBuffer.back->used > this->getPacketSize()) {
            dataBuffer.alreadyRead = false;
        }
    }
}

void NativeStream::getContent()
{
    ape_global *ape = this->net;

    switch(IInterface) {
        case INTERFACE_HTTP:
        {
            NativeHTTP *http = static_cast<NativeHTTP *>(this->interface);
            http->request(this);
            break;
        }
        case INTERFACE_FILE:
        case INTERFACE_PRIVATE:
        {
            NativeFileIO *file = static_cast<NativeFileIO *>(this->interface);
            file->open("r");
            break;
        }
        case INTERFACE_DATA:
        {
            /* async dispatch so that onload callback
            can be triggered even if declared after reading */
            timer_dispatch_async(NativeStream_data_getContent, this);
            break;
        }
        default:
            break;
    }
}

/* File methods */
void NativeStream::onNFIOOpen(NativeFileIO *NFIO)
{
    if (this->delegate) {
        size_t packetSize = this->getPacketSize();
        NFIO->read(packetSize == 0 ? NFIO->filesize : packetSize);
    }
}

void NativeStream::onNFIOError(NativeFileIO *NFIO, int err)
{

}

void NativeStream::onNFIORead(NativeFileIO *NFIO, unsigned char *data, size_t len)
{
    if (this->delegate) {
        this->delegate->onGetContent((const char *)data, len);
        if (needToSendUpdate) {
            dataBuffer.alreadyRead = false;
            needToSendUpdate = false;
            dataBuffer.back->used = 0;
            buffer_append_data(dataBuffer.back, data, len);
            this->delegate->onAvailableData(len);
        }
    }
}

void NativeStream::onNFIOWrite(NativeFileIO *NFIO, size_t written)
{

}
/****************/

/* HTTP methods */
void NativeStream::onRequest(NativeHTTP::HTTPData *h, NativeHTTP::DataType)
{
    if (this->delegate) {
        this->delegate->onGetContent((const char *)h->data->data,
            h->data->used);
        printf("Request ended\n");
    }
}

void NativeStream::onProgress(size_t offset, size_t len,
    NativeHTTP::HTTPData *h, NativeHTTP::DataType)
{
    if (!this->delegate) return;

    if (mapped.fd) {
        NativeHTTP *http = static_cast<NativeHTTP *>(this->interface);
        ssize_t written = 0;
        retry:
        if ((written = write(mapped.fd, &h->data->data[offset], len)) < 0) {
            if (errno == EINTR) {
                goto retry;
            }
            http->resetData();
            printf("NativeStream Error : Failed to write streamed content (%d)\n", errno);
            return;
        }

        /* Reset the data buffer, so that data doesnt grow in memory */
        http->resetData();

        if (dataBuffer.fresh) {
            printf("Current buffer updated (%ld)\n", len);
            dataBuffer.back->data = &((unsigned char *)mapped.addr)[mapped.size];
            dataBuffer.back->size = 0;
            dataBuffer.fresh = false;
        }

        dataBuffer.back->size += written;
        dataBuffer.back->used = dataBuffer.back->size;

        mapped.size += written;

        if (dataBuffer.back->used >= this->getPacketSize() && needToSendUpdate) {
            dataBuffer.alreadyRead = false;
            needToSendUpdate = false;

            this->delegate->onAvailableData(this->getPacketSize());
        }

        //printf("char : %c\n", ((char *)mapped.addr)[32]);
    }
}

void NativeStream::onError(NativeHTTP::HTTPError err)
{
    if (!this->delegate) return;

    this->delegate->onGetContent(NULL, 0);
}

void NativeStream::onHeader()
{
    NativeHTTP *http = static_cast<NativeHTTP *>(this->interface);
    if (this->mapped.fd && http->http.contentlength) {
        mapped.addr = mmap(NULL, http->http.contentlength,
            PROT_READ, MAP_SHARED, mapped.fd, 0);
        printf("Got a header of size : %lld\n", http->http.contentlength);
        printf("File mapped at address : %p\n", mapped.addr);
    }
}

/**********/

NativeStream::~NativeStream()
{
    free(location);

    buffer_destroy(dataBuffer.back);
    buffer_destroy(dataBuffer.front);

    if (this->interface) {
        delete this->interface;
    }
}
