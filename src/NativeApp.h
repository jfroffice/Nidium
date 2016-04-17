#ifndef nativeapp_h__
#define nativeapp_h__

#include <pthread.h>

#include <zip.h>
#include <jsoncpp.h>

#include <Binding/NidiumJS.h>

typedef bool (* NativeAppExtractCallback)(const char * buf,
    int len, size_t offset, size_t total, void *user);

class NativeApp
{
public:
    char *m_Path;

    NativeApp(const char *path);
    int open();
    void runWorker(struct _ape_global *net);
    uint64_t extractFile(const char *path, NativeAppExtractCallback cb, void *user);
    int extractApp(const char *path, void (*done)(void *, const char *), void *closure);
    ~NativeApp();

    const char *getTitle() const {
        return this->appInfos.title.asCString();
    }
    const char *getUDID() const {
        return this->appInfos.udid.asCString();
    }
    int getWidth() const {
        return this->appInfos.width;
    }
    int getHeight() const {
        return this->appInfos.height;
    }
    enum ACTION_TYPE {
        APP_ACTION_EXTRACT
    };

    struct {
        uint64_t u64;
        void *ptr;
        void *cb;
        void *user;
        enum ACTION_TYPE type;
        bool active;
        bool stop;
        uint32_t u32;
        uint8_t  u8;
    } m_Action;

    pthread_t m_ThreadHandle;
    pthread_mutex_t m_ThreadMutex;
    pthread_cond_t m_ThreadCond;

    Nidium::Core::SharedMessages *m_Messages;
    struct zip *m_fZip;

    void actionExtractRead(const char *buf, int len,
        size_t offset, size_t total);

    struct native_app_msg
    {
        size_t total;
        size_t offset;
        char *data;
        NativeAppExtractCallback cb;
        void *user;
        int len;
    };

    enum APP_MESSAGE {
        APP_MESSAGE_READ
    };
    int m_NumFiles;
private:

    Json::Reader m_Reader;
    bool m_WorkerIsRunning;

    int loadManifest();

    struct {
        Json::Value title;
        Json::Value udid;

        int width;
        int height;
    } appInfos;

    struct _ape_timer_t *m_Timer;
    struct _ape_global *m_Net;
};

#endif

