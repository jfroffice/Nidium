/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef av_av_h__
#define av_av_h__

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <ape_netlib.h>

#include "IO/Stream.h"
#include "Core/Messages.h"

extern "C" {
#include <libavutil/time.h>
}

struct Coro;
struct AVFormatContext;
struct AVDictionary;
struct PaUtilRingBuffer;

namespace Nidium {
namespace AV {

#define NIDIUM_AVIO_BUFFER_SIZE 32768
#define CORO_STACK_SIZE 4096 * 4
/* Audio processing buffer multiplier (must be power of 2) */
#define NIDIUM_AUDIO_BUFFER_MULTIPLIER 4
/* Max size for the resampling buffer */
#define NIDIUM_RESAMPLER_BUFFER_SAMPLES 16384

#define SOURCE_EVENT_PLAY 0x01
#define SOURCE_EVENT_PAUSE 0x02
#define SOURCE_EVENT_STOP 0x03
#define SOURCE_EVENT_EOF 0x04
#define SOURCE_EVENT_ERROR 0x05
#define SOURCE_EVENT_BUFFERING 0x06
#define SOURCE_EVENT_READY 0x07

#define NIDIUM_PTHREAD_VAR_DECL(name) \
    pthread_cond_t name;              \
    pthread_mutex_t name##Lock;       \
    bool name##Cond;

#define NIDIUM_PTHREAD_VAR_INIT(name)     \
    pthread_cond_init(name, NULL);        \
    pthread_mutex_init(name##Lock, NULL); \
    *(name##Cond) = false;

#define NIDIUM_PTHREAD_WAIT(mutexRef)                \
    pthread_mutex_lock(mutexRef##Lock);              \
    while (*(mutexRef##Cond) == false) {             \
        pthread_cond_wait(mutexRef, mutexRef##Lock); \
    }                                                \
    pthread_mutex_unlock(mutexRef##Lock);            \
    *(mutexRef##Cond) = false;

#define NIDIUM_PTHREAD_SIGNAL(mutexRef) \
    *(mutexRef##Cond) = true;           \
    pthread_cond_signal(mutexRef);

class AudioTrack;
class AVSource;

// {{{ AVReader
class AVReader
{
public:
    AVReader() : m_Pending(false), m_NeedWakup(false), m_Async(false){};

    bool m_Pending;
    bool m_NeedWakup;
    bool m_Async;

    virtual void finish() = 0;
    virtual ~AVReader(){};
};
// }}}

// {{{ AVBufferReader
class AVBufferReader : public AVReader
{
public:
    AVBufferReader(uint8_t *buffer, unsigned long bufferSize);
    static int read(void *opaque, uint8_t *buffer, int size);
    static int64_t seek(void *opaque, int64_t offset, int whence);

    void finish(){};
    ~AVBufferReader(){};

private:
    uint8_t *m_Buffer;
    unsigned long m_BufferSize;
    unsigned long m_Pos;
};
// }}}

typedef void (*AVStreamReadCallback)(void *m_CallbackPrivate);

// {{{ AVStreamReader
class AVStreamReader : public AVReader, public Core::Messages
{
public:
    AVStreamReader(const char *src,
                   AVStreamReadCallback readCallback,
                   void *callbackPrivate,
                   AVSource *source,
                   ape_global *net);

    AVSource *m_Source;
    int64_t m_TotalRead;

    static int read(void *opaque, uint8_t *buffer, int size);
    static int64_t seek(void *opaque, int64_t offset, int whence);

    void onAvailableData(size_t len);
    void finish();
    ~AVStreamReader();

private:
    enum
    {
        MSG_SEEK,
        MSG_READ,
        MSG_STOP
    };
    void onMessage(const Core::SharedMessages::Message &msg);
    bool isGenesisThread();

    NIDIUM_PTHREAD_VAR_DECL(m_ThreadCond);

    IO::Stream *m_Stream;
    AVStreamReadCallback m_ReadCallback;
    void *m_CallbackPrivate;

    bool m_Opened;

    size_t m_StreamRead;
    size_t m_StreamPacketSize;
    int m_StreamErr;
    size_t m_StreamSeekPos;
    off_t m_StreamSize;
    unsigned const char *m_StreamBuffer;
    int m_Error;
    bool m_HaveDataAvailable;
    pthread_t m_GenesisThread;
};
// }}}

// {{{ AudioParameters
struct AudioParameters
{
    int m_AskedBufferSize, m_BufferSize, m_Channels, m_SampleFmt, m_SampleRate,
        m_FramesPerBuffer;
    AudioParameters(int askedBufferSize,
                    int bufferSize,
                    int channels,
                    int sampleFmt,
                    int sampleRate)
        : m_AskedBufferSize(askedBufferSize), m_BufferSize(bufferSize),
          m_Channels(channels), m_SampleFmt(sampleFmt),
          m_SampleRate(sampleRate),
          m_FramesPerBuffer(bufferSize / sampleFmt)
    {
    }
};
// }}}

// {{{ Enums and texts
// {{{ Shared messages enum
enum
{
    NIDIUM_AUDIO_CALLBACK,
    NIDIUM_AUDIO_NODE_SET,
    NIDIUM_AUDIO_NODE_CALLBACK,
    NIDIUM_AUDIO_NODE_ONSET_CALLBACK, // XXX : Not used
    NIDIUM_AUDIO_SOURCE_CALLBACK,     // XXX : Not used
    NIDIUM_AUDIO_TRACK_CALLBACK,      // XXX : Not used
    NIDIUM_AUDIO_SHUTDOWN,            // XXX : Not used
};
// }}}

// {{{ Error code and description
enum
{
    ERR_FAILED_OPEN,
    ERR_READING,
    ERR_NO_INFORMATION,
    ERR_NO_AUDIO,
    ERR_NO_VIDEO,
    ERR_NO_CODEC,
    ERR_OOM,
    ERR_NO_RESAMPLING_CONVERTER,
    ERR_NO_VIDEO_CONVERTER,
    ERR_INIT_VIDEO_AUDIO,
    ERR_DECODING,
    ERR_SEEKING,
    ERR_INTERNAL,
    ERR_IO,
    ERR_MAX
};

const char *const AVErrorsStr[ERR_MAX]
    = { "Failed to open media file",
        "Failed to read media file",
        "Failed to find stream information",
        "No audio stream",
        "No video stream",
        "No codec available to decode stream",
        "Out of memory",
        "No resampling converter available",
        "No video converter available",
        "Failed to init audio stream from video",
        "Failed to decode stream",
        "Failed to seek",
        "Internal error",
        "Input/Output error" };
// }}}
// }}}

// Used for event (play, pause, stop, error, buffered...)
typedef void (*AVSourceEventCallback)(const struct AVSourceEvent *m_Ev);

// {{{ AVSourceEvent
struct AVSourceEvent
{
    int m_Ev;
    Core::Args m_Args;
    void *m_Custom;
    bool m_FromThread;
    AVSourceEvent(int ev, void *custom, bool fromThread)
        : m_Ev(ev), m_Custom(custom), m_FromThread(fromThread){};
};
// }}}

// {{{ AVSourceEventInterface
class AVSourceEventInterface
{
public:
    AVSourceEventInterface() {};

    void eventCallback(AVSourceEventCallback cbk, void *custom)
    {
        m_EventCbk       = cbk;
        m_EventCbkCustom = custom;
    };

    AVSourceEvent *createEvent(int ev, bool fromThread)
    {
        return new AVSourceEvent(ev, m_EventCbkCustom, fromThread);
    };

    void sendEvent(int type, int value, bool fromThread)
    {
        AVSourceEvent *ev = this->createEvent(type, fromThread);
        ev->m_Args[0].set(value);
        this->sendEvent(ev);
    };

    void sendEvent(AVSourceEvent *ev)
    {
        if (m_EventCbk != nullptr) {
            m_EventCbk(ev);
        }
    };

private:
    AVSourceEventCallback m_EventCbk = nullptr;
    void *m_EventCbkCustom = nullptr;
};
// }}}

// {{{ AVSource
class AVSource : public Core::Messages, public AVSourceEventInterface
{
public:
    AVSource();

    friend class AVStreamReader;

    bool m_Opened = false;
    bool m_Eof;
    static pthread_mutex_t m_FfmpegLock;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void close() = 0;
    virtual int open(const char *src) = 0;
    virtual int open(void *buffer, int size) = 0;
    virtual int openInit() = 0;

    virtual double getClock() = 0;
    virtual void seek(double time) = 0;
    double getDuration();
    int getBitrate();
    AVDictionary *getMetadata();
    AVFormatContext *getAVFormatContext();

    enum
    {
        MSG_CLOSE
    };

    virtual ~AVSource() {};
protected:
    AVFormatContext *m_Container;

    Coro *m_Coro;
    Coro *m_MainCoro;

    bool m_Seeking;
    bool m_DoSemek;
    double m_DoSeekTime;
    int m_SeekFlags;
    int m_Error;

    bool m_SourceDoOpen;

    int readError(int err);

    void onMessage(const Core::SharedMessages::Message &msg);
};
// }}}

} // namespace AV
} // namespace Nidium

#endif
