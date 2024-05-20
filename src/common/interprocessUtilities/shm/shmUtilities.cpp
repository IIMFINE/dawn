#include "interprocessUtilities/shm/shmUtilities.h"
#include "setLogger.h"
#include "type.h"

#include <fcntl.h>
#include <sys/mman.h>

namespace dawn
{
    namespace shm_utilities
    {
        void* createShm(const char* shm_name, size_t size)
        {
            if (shm_name == nullptr || size == 0)
            {
                LOG_ERROR("shm_name is nullptr or size is 0. size = {}", size);
                return nullptr;
            }

            int shm_fd = shm_open(shm_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
            if (shm_fd == -1)
            {
                LINUX_ERROR_PRINT();
                return nullptr;
            }

            if (ftruncate(shm_fd, size) == -1)
            {
                LINUX_ERROR_PRINT();
                close(shm_fd);
                return nullptr;
            }

            void* shm_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shm_ptr == MAP_FAILED)
            {
                LINUX_ERROR_PRINT();
                close(shm_fd);
                return nullptr;
            }

            close(shm_fd);

            return shm_ptr;
        }

        void* openShm(const char* shm_name, size_t size)
        {
            if (shm_name == nullptr || size == 0)
            {
                LOG_ERROR("shm_name is nullptr or size is 0. size = {}", size);
                return nullptr;
            }

            int shm_fd = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR);
            if (shm_fd == -1)
            {
                LINUX_ERROR_PRINT();
                return nullptr;
            }

            void* shm_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shm_ptr == MAP_FAILED)
            {
                LINUX_ERROR_PRINT();
                close(shm_fd);
                return nullptr;
            }

            close(shm_fd);

            return shm_ptr;
        }

        void* createOrOpenShm(const char* shm_name, size_t size)
        {
            if (shm_name == nullptr || size == 0)
            {
                LOG_ERROR("shm_name is nullptr or size is 0. size = {}", size);
                return nullptr;
            }

            void* shm_ptr = createShm(shm_name, size);

            if (shm_ptr != nullptr)
            {
                return shm_ptr;
            }

            return openShm(shm_name, size);
        }
    }  // namespace shm_utilities
}  // namespace dawn
