/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "NativeNFSStream.h"

#include <native_netlib.h>
#include <ape_buffer.h>

#include "Core/NativeUtils.h"

#include "NativeNFS.h"

#include "Binding/NativeJS.h"

#ifdef NATIVE_EMBED_PRIVATE
  #include NATIVE_EMBED_PRIVATE
#endif
NativeNFSStream::NativeNFSStream(const char *location) :
    Nidium::IO::Stream(location)
{
    static NativeNFS *nfs = NULL;

#ifdef NATIVE_EMBED_PRIVATE
    if (nfs == NULL) {
        nfs = new NativeNFS(private_bin, sizeof(private_bin));
    }
#endif
    m_NFS = nfs;
    m_File.pos = 0;
}

void NativeNFSStream::onStart(size_t packets, size_t seek)
{
    m_File.data = (const unsigned char *)m_NFS->readFile(m_Location, &m_File.len);
    m_File.pos = 0;

    if (m_File.data == NULL) {
        this->error(STREAM_ERROR_OPEN, 0);

        return;
    }

    CREATE_MESSAGE(message_available,
        Nidium::IO::STREAM_AVAILABLE_DATA);
    message_available->args[0].set(native_min(packets, m_File.len));

    this->notify(message_available);

    buffer buf;
    buffer_init(&buf);

    buf.data = (unsigned char *)m_File.data;
    buf.size = buf.used = m_File.len;

    CREATE_MESSAGE(message, Nidium::IO::STREAM_READ_BUFFER);
    message->args[0].set(&buf);

    /*
        The underlying object is notified in a sync way
        since it's on the same thread.
    */
    this->notify(message);
}

const unsigned char *NativeNFSStream::onGetNextPacket(size_t *len, int *err)
{
    const unsigned char *data;

    ssize_t byteLeft = m_File.len - m_File.pos;

    if (byteLeft <= 0) {
        *err = STREAM_END;
        return NULL;
    }

    data = m_File.data + m_File.pos;
    *len = native_min(m_PacketsSize, byteLeft);

    m_File.pos += *len;

    return data;
}

void NativeNFSStream::stop()
{
    /*
        Do nothing
    */
}

int NativeNFSStream_getContent(void *arg)
{
    ((NativeNFSStream *)arg)->_getContent();

    return 0;
}

void NativeNFSStream::getContent()
{
    ape_global *ape = NativeJS::getNet();
    timer_dispatch_async_unprotected(NativeNFSStream_getContent, this);
}

void NativeNFSStream::_getContent()
{
    m_File.data = (const unsigned char *)m_NFS->readFile(m_Location, &m_File.len);
    m_File.pos = 0;

    if (m_File.data == NULL) {
        this->error(STREAM_ERROR_OPEN, 0);

        return;
    }

    buffer buf;
    buffer_init(&buf);

    buf.data = (unsigned char *)m_File.data;
    buf.size = buf.used = m_File.len;

    CREATE_MESSAGE(message, Nidium::IO::STREAM_READ_BUFFER);
    message->args[0].set(&buf);

    this->notify(message);
}

bool NativeNFSStream::getContentSync(char **data, size_t *len, bool mmap)
{
    *data = NULL;

    const char *ret = m_NFS->readFile(m_Location, len);

    if (!ret) {
        *len = 0;
        return false;
    }

    if (!mmap) {
        *data = (char *)malloc(*len + 1);
        memcpy(*data, ret, *len);
        (*data)[*len] = '\0';
    } else {
        /* /!\ data is not null terminated */
        *data = (char *)ret;
    }

    return true;
}

size_t NativeNFSStream::getFileSize() const
{
    return m_File.len;
}

void NativeNFSStream::seek(size_t pos)
{
    if (pos > m_File.len) {
        this->error(STREAM_ERROR_SEEK, -1);
        return;
    }
    m_File.pos = pos;
}

