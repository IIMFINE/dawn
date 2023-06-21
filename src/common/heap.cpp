#include "heap.h"
#include "type.h"

namespace  dawn
{
  bool minixHeap::empty()
  {
    return heapSize_ == 0;
  }

  int minixHeap::size()
  {
    return heapSize_;
  }
} // namespace  dawn

