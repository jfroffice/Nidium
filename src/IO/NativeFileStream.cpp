/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "NativeFileStream.h"

#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

NativeFileStream::NativeFileStream(const char *location) :
    Nidium::IO::Stream(location), m_File(location)
{
    /* We don't want the file to close when end of file is reached */
    m_File.setAutoClose(false);
    m_File.setListener(this);
}

void NativeFileStream::onStart(size_t packets, size_t seek)
{
    if (m_DataBuffer.back == NULL) {
        m_DataBuffer.back = buffer_new(packets);
        m_DataBuffer.front = buffer_new(packets);
    }

    m_File.open("r");

    if (seek) {
        m_File.seek(seek);
    }

    m_File.read(packets);
}

const unsigned char *NativeFileStream::onGetNextPacket(size_t *len, int *err)
{
    unsigned char *data;

    if (m_DataBuffer.back == NULL) {
        *err = STREAM_ERROR;
        return NULL;
    }

    if (!this->hasDataAvailable()) {
        /*
            Notify the listener whenever data become available
        */
        m_NeedToSendUpdate = !m_DataBuffer.ended;
        *err = (m_DataBuffer.ended && m_DataBuffer.alreadyRead && !m_PendingSeek ?
            STREAM_END : STREAM_EAGAIN);

        return NULL;
    }

    data = m_DataBuffer.back->data;
    *len = native_min(m_DataBuffer.back->used, m_PacketsSize);
    m_DataBuffer.alreadyRead = true;

    this->swapBuffer();

    if (!m_DataBuffer.ended) {
        m_File.read(m_PacketsSize);
    }

    return data;
}

void NativeFileStream::stop()
{
    /*
        Do nothing
    */
}

void NativeFileStream::getContent()
{
    if (m_File.isOpen()) {
        return;
    }

    m_File.open("r");
    m_File.read(UINT_MAX);
    m_File.close();
}

bool NativeFileStream::getContentSync(char **data, size_t *len, bool mmap)
{
    int err;
    ssize_t slen;

    *data = NULL;

    if (!m_File.openSync("r", &err)) {
        return false;
    }

    if (!mmap) {
        if ((slen = m_File.readSync(m_File.getFileSize(), data, &err)) < 0) {
            *len = 0;
            return false;
        }
    } else {
        if ((slen = m_File.mmapSync(data, &err)) < 0) {
            *len = 0;
            return false;
        }
    }

    *len = slen;

    return true;
}

size_t NativeFileStream::getFileSize() const
{
    return m_File.getFileSize();
}

void NativeFileStream::seek(size_t pos)
{
    if (!m_File.isOpen()) {
        return;
    }

    m_File.seek(pos);
    m_File.read(m_PacketsSize);
    m_PendingSeek = true;
    m_DataBuffer.back->used = 0;
    m_NeedToSendUpdate = true;
    m_DataBuffer.ended = false;
}

void NativeFileStream::onMessage(const Nidium::Core::SharedMessages::Message &msg)
{
    switch (msg.event()) {
        case Nidium::IO::FILE_OPEN_SUCCESS:
            /* do nothing */
            break;
        case Nidium::IO::FILE_OPEN_ERROR:
            this->error(STREAM_ERROR_OPEN, msg.args[0].toInt());
            break;
        case Nidium::IO::FILE_SEEK_ERROR:
            this->error(STREAM_ERROR_SEEK, -1);
            /* fall through */
        case Nidium::IO::FILE_SEEK_SUCCESS:
            m_PendingSeek = false;
            break;
        case Nidium::IO::FILE_READ_ERROR:
            this->error(STREAM_ERROR_READ, msg.args[0].toInt());
            break;
        case Nidium::IO::FILE_READ_SUCCESS:
        {
            if (m_PendingSeek) {
                break;
            }

            /*
                the buffer is automatically detroyed by Nidium::IO::File
                after the return of this function
            */
            buffer *buf = (buffer *)msg.args[0].toPtr();

            if (m_File.eof()) {
                m_DataBuffer.ended = true;
            }

            m_DataBuffer.alreadyRead = false;

            if (m_DataBuffer.back != NULL) {
                m_DataBuffer.back->used = 0;
                if (buf->data != NULL) {
                    /*
                        Feed the backbuffer with the new data (copy)
                    */
                    buffer_append_data(m_DataBuffer.back, buf->data, buf->used);

                    if (m_NeedToSendUpdate) {
                        m_NeedToSendUpdate = false;
                        CREATE_MESSAGE(message_available,
                            Nidium::IO::STREAM_AVAILABLE_DATA);
                        message_available->args[0].set(buf->used);
                        this->notify(message_available);
                    }
                }
            }

            CREATE_MESSAGE(message, Nidium::IO::STREAM_READ_BUFFER);
            message->args[0].set(buf);

            /*
                The underlying object is notified in a sync way
                since it's on the same thread.
            */
            this->notify(message);

            break;
        }
    }
}

