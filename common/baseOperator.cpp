#include <iostream>
#include "type.h"
#include "baseOperator.h"

/**************************************
*only apply for 32 bit integer
**************************************/
int getTopBitPostion(uint32_t number)
{
    if(number != 0)
    {
        int factor = 0;
        for(int i = INTEGER_TYPE_32_BIT / 2; i > 0; i = i / 2)
        {
            if(number >> ((factor ^ i)))
            {
                factor ^= i;
            }
        }
        return factor;
    }
    return 0;
}