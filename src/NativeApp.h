#ifndef nativeapp_h__
#define nativeapp_h__

#include <json/json.h>
#include <pthread.h>
#include "zip.h"

class NativeSharedMessages;
class NativeJS;

typedef bool (* NativeAppExtractCallback)(const char * buf,
    int len, size_t offset, size_t total);

class NativeApp
{
public:
    char *path;
    
    NativeApp(const char *path);
    int open();
    void runWorker(struct _ape_global *net);
    uint64_t extractFile(const char *path, NativeAppExtractCallback cb);
    ~NativeApp();

    const char *getTitle() const {
        return this->appInfos.title.asCString();
    }
    const char *getUDID() const {
        return this->appInfos.udid.asCString();
    }

    enum ACTION_TYPE {
        APP_ACTION_EXTRACT
    };

    struct {
        uint64_t u64;
        void *ptr;
        void *user;
        enum ACTION_TYPE type;
        bool active;
        bool stop;
        uint32_t u32;
        uint8_t  u8;
    } action;

    pthread_t threadHandle;
    pthread_mutex_t threadMutex;
    pthread_cond_t threadCond;

    NativeSharedMessages *messages;
    struct zip *fZip;

    void actionExtractRead(const char *buf, int len,
        size_t offset, size_t total);

    struct native_app_msg
    {
        size_t total;
        size_t offset;
        char *data;
        NativeAppExtractCallback cb;
        int len;
    };

    enum APP_MESSAGE {
        APP_MESSAGE_READ
    };
private:

    Json::Reader reader;
    int numFiles;
    bool workerIsRunning;

    int loadManifest();
    

    struct {
        Json::Value title;
        Json::Value udid;
    } appInfos;

    struct _ape_timer *timer;
    struct _ape_global *net;
};

#endif
