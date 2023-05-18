#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__
#include "common/type.h"

namespace dawn
{
  struct abstractTransport
  {
    enum class BLOCKING_TYPE
    {
      BLOCK,
      NON_BLOCK
    };
    virtual bool write(const void *write_data, const uint32_t data_len) = 0;
    virtual bool read(void *read_data, uint32_t &data_len, BLOCKING_TYPE block_type) = 0;
    virtual bool wait() = 0;
  };
}

#endif

