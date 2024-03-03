// In order to prevent linker error, we should place all the base struct type
// in this head file.By the way,
// we make sure this head file is the lowest level head file.

#ifndef _TYPE_H_
#define _TYPE_H_
#include <functional>
#include <type_traits>

namespace dawn
{


#define FALSE 0
#define TRUE 1

#define PROCESS_FAIL 0
#define PROCESS_SUCCESS 1
#define INVALID_VALUE -1

#define LINUX_ERROR_PRINT() LOG_ERROR("error code {}", strerror(errno))

  using uint8_t = unsigned char;
  using uint16_t = unsigned short;
  using uint32_t = unsigned int;
  using uint64_t = unsigned long;

  constexpr const char *INVALID_IP = "0";
  constexpr const char *LOCAL_HOST = "127.0.0.1";

  template <typename T>
  class singleton
  {
    public:
    singleton() = default;
    ~singleton() = default;
    singleton(singleton &) = delete;
    singleton(const singleton &) = delete;
    singleton &operator=(const singleton &) = delete;
    static T *getInstance()
    {
      static T instance;
      return &instance;
    }
  };

  /// @brief create a arithmetic sequence contained increase number
  /// @tparam T array type
  /// @tparam TOTAL_NUM size of sequence
  /// @tparam BASE the smallest number in sequence
  /// @tparam RATIO arithmetic sequence ratio
  template<typename T, unsigned int TOTAL_NUM, int BASE, int RATIO>
  struct arithmeticSequenceInc
  {
    constexpr arithmeticSequenceInc() :
      array()
    {
      static_assert(TOTAL_NUM > 0, "total number at least is 1");
      array[0] = BASE;
      for (unsigned int i = 1; i < TOTAL_NUM; i++)
      {
        array[i] = array[i - 1] + RATIO;
      }
    }
    T array[TOTAL_NUM];
  };

  /// @brief create a arithmetic sequence contained decrease number
  /// @tparam T array type
  /// @tparam TOTAL_NUM size of sequence
  /// @tparam BASE the smallest number in sequence
  /// @tparam RATIO arithmetic sequence ratio
  template<typename T, unsigned int TOTAL_NUM, int BASE, int RATIO>
  struct arithmeticSequenceDec
  {
    constexpr arithmeticSequenceDec() :
      array()
    {
      static_assert(TOTAL_NUM > 0, "total number at least is 1");
      array[TOTAL_NUM - 1] = BASE;
      for (unsigned int i = 1; i < TOTAL_NUM; i++)
      {
        array[TOTAL_NUM - 1 - i] = array[TOTAL_NUM - i] + RATIO;
      }
    }
    T array[TOTAL_NUM];
  };
  template <typename, typename = void>
  struct is_callable_helper: std::false_type
  {
  };

  template <typename T>
  struct is_callable_helper<T, std::void_t<decltype(std::declval<T>()())>>: std::true_type
  {
  };

  template <typename T>
  struct is_callable: is_callable_helper<std::remove_reference_t<T>>
  {
  };

};

#endif
