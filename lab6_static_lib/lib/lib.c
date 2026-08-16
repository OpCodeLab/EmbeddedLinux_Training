#include "lib.h"

/*
 ar rcs mylib.a lib.o 
*/

/*Public Api Implementation */
uint32_t compute_crc32(const void *buf, uint32_t size)
{
    const uint8_t *p = buf;
    uint32_t crc = 0;
    
    while (size--)
    {
        crc = crc ^ *p++;
        for (uint32_t i = 0; i < 8; i++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
          
    }

    return crc ^ ~0U;
}


uint32_t getMaxValue(uint32_t *table, uint32_t size)
{
    uint32_t max = 0;
    for (uint32_t i = 0; i < size; i++)
    {
        if (table[i] > max)
        {
            max = table[i];
        }
    }
    return max;
}

