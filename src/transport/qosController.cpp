#include "qosController.h"

namespace dawn
{

  reliableQosCfg::reliableQosCfg() :
    qosCfg()
  {
    qosType_ = qosCfg::QOS_TYPE::RELIABLE;
  }
}
