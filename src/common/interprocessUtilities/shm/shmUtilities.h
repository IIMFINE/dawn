#ifndef __SHM_MANAGER_H__
#define __SHM_MANAGER_H__

#include <cstdlib>
#include <string_view>

namespace dawn
{
    namespace shm_utilities
    {
        void* createShm(const char* shm_name, size_t size);
        void* openShm(const char* shm_name, size_t size);
        void* createOrOpenShm(const char* shm_name, size_t size);
    }  // namespace shm_utilities
}  // namespace dawn
#endif
