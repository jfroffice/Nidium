#ifndef nativerepl_h__
#define nativerepl_h__

#include <ape_netlib.h>
#include <Core/NativeMessages.h>

#include <ape_buffer.h>
#include <semaphore.h>

class NativeJS;

class NativeREPL : public NativeMessages
{
public:
    NativeREPL(NativeJS *js);
    ~NativeREPL();
    void onMessage(const NativeSharedMessages::Message &msg);
    void onMessageLost(const NativeSharedMessages::Message &msg);

    sem_t *getReadLineLock() {
        return &m_ReadLineLock;
    }

    bool isContinuing() const {
        return m_Continue;
    }

    int getExitCount() const {
        return m_ExitCount;
    }

    int setExitCount(int val) {
        m_ExitCount = val;

        return m_ExitCount;
    }
private:
    pthread_t m_ThreadHandle;
    sem_t m_ReadLineLock;

    NativeJS *m_JS;

    buffer *m_Buffer;

    bool m_Continue;
    int m_ExitCount;
};

#endif

