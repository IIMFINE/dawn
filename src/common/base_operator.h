#ifndef _BASE_OPERATOR_H_
#define _BASE_OPERATOR_H_
#include "type.h"

namespace dawn
{

const int INTEGER_TYPE_32_BIT = 32;
inline int getTopBitPosition(uint32_t number)
{
    if (number != 0)
    {
        int factor = 0;
        for (int i = INTEGER_TYPE_32_BIT / 2; i > 0; i = i / 2)
        {
            if (number >> ((factor ^ i)))
            {
                factor ^= i;
            }
        }
        return factor;
    }
    return 0;
}

}  // namespace dawn
#endif
