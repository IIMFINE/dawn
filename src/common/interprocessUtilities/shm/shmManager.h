#ifndef __SHM_MANAGER_H__
#define __SHM_MANAGER_H__

#include <cstdlib>

struct ShmManager
{
    ShmManager();
    ~ShmManager();
    bool createShm(const char *shm_name, size_t size);
    bool openShm(const char *shm_name, size_t size);
    bool closeShm();
    bool destroyShm(const char *shm_name);
    void *getShmPtr();
    bool isInitialized() const;
    bool isCreated() const;
    bool isDestroyed() const;
    bool isOpened() const;

    void *shm_ptr_;
    bool is_initialized_;
    bool is_created_;
    bool is_destroyed_;
    bool is_opened_;
};
#endif
