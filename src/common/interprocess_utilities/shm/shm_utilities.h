#ifndef __SHM_MANAGER_H__
#define __SHM_MANAGER_H__

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace dawn
{
namespace shm_utilities
{
void* createShm(const char* shm_name, size_t size);
void* openShm(const char* shm_name, size_t size);
void* createOrOpenShm(const char* shm_name, size_t size);
bool unmapShm(void* shm_ptr, size_t size);

void* createShm(const std::string_view shm_name, size_t size);
void* openShm(const std::string_view shm_name, size_t size);
void* createOrOpenShm(const std::string_view shm_name, size_t size);

template <typename T>
std::shared_ptr<T> findOrConstruct(const std::string_view shm_name)
{
    auto hash_shm_name = [shm_name]()
    {
        auto type_info_hash_code = typeid(std::remove_reference<T>).hash_code();
        return std::to_string(type_info_hash_code) + std::string(shm_name);
    };
}

}  // namespace shm_utilities
}  // namespace dawn
#endif
