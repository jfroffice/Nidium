/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "NativeFile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fts.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ape_buffer.h>

#define NATIVE_FILE_NOTIFY(param, event, arg) \
    do {   \
        NativeSharedMessages::Message *__msg = new NativeSharedMessages::Message(event); \
        __msg->args[0].set(param); \
        __msg->args[7].set(arg); \
        this->postMessage(__msg); \
    } while(0);

enum {
    NATIVEFILE_TASK_OPEN,
    NATIVEFILE_TASK_CLOSE,
    NATIVEFILE_TASK_READ,
    NATIVEFILE_TASK_WRITE,
    NATIVEFILE_TASK_SEEK,
    NATIVEFILE_TASK_LISTFILES
};

NativeFile::NativeFile(const char *name) :
    m_Dir(NULL), m_Fd(NULL), m_Delegate(NULL),
    m_Filesize(0), m_AutoClose(true),
    m_Eof(false), m_OpenSync(false), m_isDir(false)
{
    m_Mmap.addr = NULL;
    m_Mmap.size = 0;
    m_Path = strdup(name);
}

/*
    /!\ Exec in a worker thread
*/
void NativeFile_dispatchTask(NativeTask *task)
{
    NativeFile *file = (NativeFile *)task->getObject();
    uint64_t type = task->args[0].toInt64();
    void *arg = task->args[7].toPtr();

    switch (type) {
        case NATIVEFILE_TASK_OPEN:
        {
            char *modes = (char *)task->args[1].toPtr();
            file->openTask(modes, arg);
            free(modes);
            break;
        }
        case NATIVEFILE_TASK_CLOSE:
        {
            file->closeTask();
            break;
        }
        case NATIVEFILE_TASK_READ:
        {
            uint64_t size = task->args[1].toInt64();
            file->readTask(size, arg);
            break;
        }
        case NATIVEFILE_TASK_WRITE:
        {
            uint64_t buflen = task->args[1].toInt64();
            char *buf = (char *)task->args[2].toPtr();

            file->writeTask(buf, buflen, arg);

            free(buf);
            break;
        }
        case NATIVEFILE_TASK_SEEK:
        {
            uint64_t pos = task->args[1].toInt64();
            file->seekTask(pos, arg);
            break;
        }
        case NATIVEFILE_TASK_LISTFILES:
        {
            file->listFilesTask(arg);
            break;
        }
        default:
            break;
    }
}

/*
    /!\ Exec in a worker thread
*/
void NativeFile::openTask(const char *mode, void *arg)
{
    if (this->isOpen()) {
        // seek(0)?
        NATIVE_FILE_NOTIFY(m_Fd, NATIVEFILE_OPEN_SUCCESS, arg);
        return;
    }

    struct stat s;
    int ret;

    ret = lstat(m_Path, &s);

    if (ret == 0 && S_ISDIR(s.st_mode)) {
        m_Dir = opendir(m_Path);
        if (!m_Dir) {
            printf("Failed to open dir %s : %s\n", m_Path, strerror(errno));
            NATIVE_FILE_NOTIFY(errno, NATIVEFILE_OPEN_ERROR, arg);
            return;
        }
        m_isDir = true;
        m_Filesize = 0;
    } else {
        m_Fd = fopen(m_Path, mode);
        if (m_Fd == NULL) {
            NATIVE_FILE_NOTIFY(errno, NATIVEFILE_OPEN_ERROR, arg);
            return;
        }

        m_Filesize = ret == 0 ? s.st_size : 0;
        m_isDir = false;
    }

    NATIVE_FILE_NOTIFY(m_Fd, NATIVEFILE_OPEN_SUCCESS, arg);
}

/*
    /!\ Exec in a worker thread
*/
void NativeFile::closeTask(void *arg)
{
    closeFd();

    if (!m_OpenSync) {
        NATIVE_FILE_NOTIFY((void *)NULL, NATIVEFILE_CLOSE_SUCCESS, arg);
    }
}

/*
    /!\ Exec in a worker thread
*/
void NativeFile::readTask(size_t size, void *arg)
{
    if (!this->isOpen() || this->isDir()) {
        NATIVE_FILE_NOTIFY((void *)NULL, NATIVEFILE_READ_ERROR, arg);
        return;
    }

    uint64_t clamped_len;
    clamped_len = native_min(m_Filesize, size);

    buffer *buf = buffer_new(clamped_len + 1);

    /*
        Read an empty file
    */
    if (clamped_len == 0) {
        NATIVE_FILE_NOTIFY((void *)buf, NATIVEFILE_READ_SUCCESS, arg);
        buf->data[0] = '\0';
        return;
    }

    if ((buf->used = fread(buf->data, 1, clamped_len, m_Fd)) == 0) {
        this->checkRead(true, arg);
        buffer_destroy(buf);
        return;
    }

    this->checkEOF();

    buf->data[buf->used] = '\0';

    NATIVE_FILE_NOTIFY((void *)buf, NATIVEFILE_READ_SUCCESS, arg);
}

/*
    /!\ Exec in a worker thread
*/
void NativeFile::writeTask(char *buf, size_t buflen, void *arg)
{
    if (!this->isOpen() || this->isDir()) {
        NATIVE_FILE_NOTIFY((void *)NULL, NATIVEFILE_WRITE_ERROR, arg);
        return;
    }

    size_t writelen = fwrite(buf, 1, buflen, m_Fd);
    /*
        TODO:
        save curosor pisition,
        position cursor at the end,
        restore position
    */
    m_Filesize = ftell(m_Fd);

    NATIVE_FILE_NOTIFY(writelen, NATIVEFILE_WRITE_SUCCESS, arg);
}

/*
    /!\ Exec in a worker thread
*/
void NativeFile::seekTask(size_t pos, void *arg)
{
    if (!this->isOpen() || this->isDir()) {
        int err = 0;
        NATIVE_FILE_NOTIFY(err, NATIVEFILE_SEEK_ERROR, arg);
        return;
    }

    if (fseek(m_Fd, pos, SEEK_SET) == -1) {
        NATIVE_FILE_NOTIFY(errno, NATIVEFILE_SEEK_ERROR, arg);
        return;
    }

    this->checkEOF();

    NATIVE_FILE_NOTIFY((void *)NULL, NATIVEFILE_SEEK_SUCCESS, arg);
}

/*
    /!\ Exec in a worker thread
*/
void NativeFile::listFilesTask(void *arg)
{
    if (!this->isOpen() || !this->isDir()) {
        return;
    }

    dirent *cur;
    DirEntries *entries = (DirEntries *)malloc(sizeof(DirEntries));
    entries->allocated = 64;
    entries->lst = (dirent *)malloc(sizeof(dirent) * entries->allocated);

    entries->size = 0;

    while ((cur = readdir(m_Dir)) != NULL) {
        if (strcmp(cur->d_name, ".") == 0 || strcmp(cur->d_name, "..") == 0) {
            continue;
        }

        memcpy(&entries->lst[entries->size], cur, sizeof(dirent));
        entries->size++;

        if (entries->size == entries->allocated) {
            entries->allocated *= 2;
            entries->lst = (dirent *)realloc(entries->lst,
                sizeof(dirent) * entries->allocated);
        }
    }

    NATIVE_FILE_NOTIFY(entries, NATIVEFILE_LISTFILES_ENTRIES, arg);

    rewinddir(m_Dir);
}

void NativeFile::open(const char *mode, void *arg)
{
    NativeTask *task = new NativeTask();
    task->args[0].set(NATIVEFILE_TASK_OPEN);
    task->args[1].set(strdup(mode));
    task->args[7].set(arg);

    task->setFunction(NativeFile_dispatchTask);

    this->addTask(task);
}

void NativeFile::close(void *arg)
{
    NativeTask *task = new NativeTask();
    task->args[0].set(NATIVEFILE_TASK_CLOSE);
    task->args[7].set(arg);

    task->setFunction(NativeFile_dispatchTask);

    this->addTask(task);
}

void NativeFile::read(size_t size, void *arg)
{
    NativeTask *task = new NativeTask();
    task->args[0].set(NATIVEFILE_TASK_READ);
    task->args[1].set(size);
    task->args[7].set(arg);

    task->setFunction(NativeFile_dispatchTask);

    this->addTask(task);
}

void NativeFile::write(char *buf, size_t size, void *arg)
{
    unsigned char *newbuf = (unsigned char *)malloc(size);
    memcpy(newbuf, buf, size);

    NativeTask *task = new NativeTask();
    task->args[0].set(NATIVEFILE_TASK_WRITE);
    task->args[1].set(size);
    task->args[2].set(newbuf);
    task->args[7].set(arg);

    task->setFunction(NativeFile_dispatchTask);

    this->addTask(task);
}

void NativeFile::seek(size_t pos, void *arg)
{
    NativeTask *task = new NativeTask();
    task->args[0].set(NATIVEFILE_TASK_SEEK);
    task->args[1].set(pos);
    task->args[7].set(arg);

    task->setFunction(NativeFile_dispatchTask);

    this->addTask(task);
}

void NativeFile::listFiles(void *arg)
{
    NativeTask *task = new NativeTask();
    task->args[0].set(NATIVEFILE_TASK_LISTFILES);
    task->args[7].set(arg);

    task->setFunction(NativeFile_dispatchTask);

    this->addTask(task);
}

bool NativeFile::checkEOF()
{
    if (m_Fd &&
        ((m_Eof = (bool)feof(m_Fd)) == true ||
        (m_Eof = (ftell(m_Fd) == this->m_Filesize)))) {

        if (m_AutoClose) {
            this->closeTask();
        }
    }

    return m_Eof;
}

void NativeFile::checkRead(bool async, void *arg)
{
    int err = -1;

    if (ferror(m_Fd)) {
        err = errno;
    } else if (this->checkEOF()) {
        err = 0;
    }

    if (async && err != -1) {
        NATIVE_FILE_NOTIFY(err, NATIVEFILE_READ_ERROR, arg);
    }
}

static int NativeFile_compare(const FTSENT** one, const FTSENT** two)
{
    return (strcmp((*one)->fts_name, (*two)->fts_name));
}

void NativeFile::rmrf()
{
    if (!isDir()) {
        return;
    }

    FTS *tree;
    FTSENT *f;

    char *path[] = {m_Path, NULL};

    tree = fts_open(path,
        FTS_COMFOLLOW | FTS_NOCHDIR, NativeFile_compare);

    if (!tree) {
        printf("Failed to fts_open()\n");
        return;
    }
    while ((f = fts_read(tree))) {
        switch(f->fts_info) {
            case FTS_F:
            case FTS_NS:
            case FTS_SL:
            case FTS_SLNONE:
                unlink(f->fts_path);
                break;
            case FTS_DP:
                rmdir(f->fts_path);
                break;
            default:
                break;
        }
    }

    fts_close(tree);

    closeFd();
}

NativeFile::~NativeFile()
{
    if (m_Mmap.addr) {
        munmap(m_Mmap.addr, m_Mmap.size);
    }
    if (this->isOpen()) {
        this->closeTask();
    }

    free(m_Path);
}

void NativeFile::onMessage(const NativeSharedMessages::Message &msg)
{
    if (m_Delegate) {
        m_Delegate->onMessage(msg);
    }

    switch(msg.event()) {
        case NATIVEFILE_READ_SUCCESS:
        {
            buffer *buf = (buffer *)msg.args[0].toPtr();
            buffer_delete(buf);
            break;
        }
        case NATIVEFILE_LISTFILES_ENTRIES:
        {
            DirEntries *entries = (DirEntries *)msg.args[0].toPtr();
            free(entries->lst);
            free(entries);
            break;
        }
    }
}

void NativeFile::onMessageLost(const NativeSharedMessages::Message &msg)
{
    switch(msg.event()) {
        case NATIVEFILE_READ_SUCCESS:
        {
            buffer *buf = (buffer *)msg.args[0].toPtr();
            buffer_delete(buf);
            break;
        }
        case NATIVEFILE_LISTFILES_ENTRIES:
        {
            DirEntries *entries = (DirEntries *)msg.args[0].toPtr();
            free(entries->lst);
            free(entries);
            break;
        }
    }
}

////////////////////
////////////////////

int NativeFile::openSync(const char *modes, int *err)
{
    *err = 0;

    if (this->isOpen()) {
        return 1;
    }

    struct stat s;
    int ret;

    ret = lstat(m_Path, &s);

    if (ret == 0 && S_ISDIR(s.st_mode)) {
        m_Dir = opendir(m_Path);
        if (!m_Dir) {
            printf("Failed to open : %s errno=%d\n", m_Path, errno);

            *err = errno;
            return 0;
        }
        m_isDir = true;
        m_Filesize = 0;
    } else {
        if ((m_Fd = fopen(m_Path, modes)) == NULL) {
            printf("Failed to open : %s errno=%d\n", m_Path, errno);
            *err = errno;
            return 0;
        }

         m_Filesize = s.st_size;
    }
    m_OpenSync = true;

    return 1;
}

ssize_t NativeFile::writeSync(char *data, uint64_t len, int *err)
{
    *err = 0;

    if (!this->isOpen() || this->isDir()) {
        return -1;
    }
    int ret;

    ret = fwrite(data, sizeof(char), len, m_Fd);

    if (ret < len) {
        *err = errno;
    } else {
        m_Filesize = ftell(m_Fd);
    }

    return ret;
}

ssize_t NativeFile::mmapSync(char **buffer, int *err)
{
    *err = 0;

    if (!this->isOpen() || this->isDir()) {
        return -1;
    }
    size_t size = this->getFileSize();

    m_Mmap.addr = mmap(NULL, size,
        PROT_READ,
        MAP_SHARED,
        fileno(m_Fd), 0);

    if (m_Mmap.addr == MAP_FAILED) {
        *err = errno;
        return -1;
    }

    m_Mmap.size = size;

    *buffer = (char *)m_Mmap.addr;

    return this->getFileSize();
}

ssize_t NativeFile::readSync(uint64_t len, char **buffer, int *err)
{
    *err = 0;

    *buffer = NULL;

    if (!this->isOpen() || this->isDir()) {
        return -1;
    }

    uint64_t clamped_len;
    clamped_len = native_min(m_Filesize, len);

    /*
        Read an empty file
    */
    if (clamped_len == 0) {
        return 0;
    }

    char *data = (char *)malloc(clamped_len + 1);
    size_t readsize = 0;

    if ((readsize = fread(data, sizeof(char), clamped_len, m_Fd)) == 0) {

        this->checkRead(false);

        free(data);
        *err = errno;
        return -1;
    }

    data[readsize] = '\0';

    *buffer = data;

    this->checkEOF();

    return readsize;
}

void NativeFile::closeSync()
{
    if (!this->isOpen()) {
        return;
    }

    closeFd();
}

int NativeFile::seekSync(size_t pos, int *err)
{
    *err = 0;
    if (!this->isOpen() || this->isDir()) {
        return -1;
    }

    if (fseek(m_Fd, pos, SEEK_SET) == -1) {
        *err = errno;

        return -1;
    }

    return 0;
}

