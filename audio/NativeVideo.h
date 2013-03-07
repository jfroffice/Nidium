#ifndef nativevideo_h__
#define nativevideo_h__

#include <pthread.h>
#include <stdint.h>
#include <native_netlib.h>
#include "NativeAudio.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#define NATIVE_VIDEO_BUFFER_SAMPLES 8
#define NATIVE_VIDEO_SYNC_THRESHOLD 0.01 
#define NATIVE_VIDEO_NOSYNC_THRESHOLD 10.0
#define NATIVE_VIDEO_PAQUET_QUEUE_SIZE 16

typedef void (*VideoCallback)(uint8_t *data, void *custom);
struct PaUtilRingBuffer;

struct SwsContext;
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;

class NativeVideo : public NativeAVSource
{
    public :
        NativeVideo(NativeAudio *audio, ape_global *n);

        struct TimerItem {
            int id;
            int delay;
            TimerItem() : id(-1), delay(-1) {};
        };

        struct Packet {
            AVPacket curr;
            Packet *prev;
            Packet *next;
            
            Packet() 
                : prev(NULL), next(NULL) {}
        };

        struct PacketQueue {
            Packet *head;
            Packet *tail;
            int count;
            PacketQueue() : head(NULL), tail(NULL), count(0) {}
        };

        struct Frame {
            uint8_t *data;
            double pts;
            Frame() : data(NULL), pts(0) {};
        };

        TimerItem *timers[NATIVE_VIDEO_BUFFER_SAMPLES];
        PacketQueue *audioQueue;
        PacketQueue *videoQueue;
        Packet *freePacket;
        Frame *pendingFrame;
        int timerIdx;
        int lastTimer;
        int timersDelay;

        ape_global *net;
        NativeAudio *audio;
        NativeAudioTrack *track;

        VideoCallback frameCbk;
        void *frameCbkArg;

        bool shutdown;
        bool eof;

        uint8_t *tmpFrame;
        uint8_t *frameBuffer;
        int frameSize;
        double lastPts;
        static uint64_t pktPts;
        double videoClock;
        double audioClock;
        double frameTimer;
        double lastDelay;

        bool playing;
        bool stoped;
        bool doSeek;
        double doSeekTime;

        int width;
        int height;

        SwsContext *swsCtx;
        AVCodecContext *codecCtx;
        int videoStream;

        PaUtilRingBuffer *rBuff;
        uint8_t *buff;
        unsigned char *avioBuffer;
        AVFrame *decodedFrame; 
        AVFrame *convertedFrame;

        pthread_t threadDecode;
        pthread_mutex_t buffLock, resetWaitLock, seekLock;
        pthread_cond_t buffNotEmpty, resetWait, seekCond;

        void play();
        void pause();
        void stop();
        int open(void *buffer, int size);
        double getClock();
        void seek(double time);

        void frameCallback(VideoCallback cbk, void *arg);

        NativeAudioTrack *getAudio();
        static void* decode(void *args);
        static int display(void *custom);

        ~NativeVideo();
    private :
        NativeAVBufferReader *br;

        void close(bool reset);
        void seekMethod(double time);
        
        bool processAudio();
        bool processVideo();

        double syncVideo(double pts);
        void scheduleDisplay(int delay);
        void scheduleDisplay(int delay, bool force);
        int addTimer(int delay);
        bool addPacket(PacketQueue *queue, AVPacket *pkt);
        Packet *getPacket(PacketQueue *queue);
        void clearTimers(bool reset);
        void clearAudioQueue();
        void clearVideoQueue();
        void flushBuffers();

        static int getBuffer(struct AVCodecContext *c, AVFrame *pic);
        static void releaseBuffer(struct AVCodecContext *c, AVFrame *pic);

};

#endif
