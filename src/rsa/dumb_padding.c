#include "config.h"
#include "dumb_padding.h"
#include <limits.h>
#include <stdlib.h>

int dp_pad(uint8_t *block, size_t block_size, size_t filled, size_t *param)
{
    if (block_size<sizeof(uint32_t) || block_size>UINT_MAX)
        return DP_ERR;
    if (!filled) // fill empty block
    {
        // ... with random data
        for (size_t i = filled; i<block_size-sizeof(uint32_t); i++)
            block[i] = (uint8_t)(rand()%255);
        // XXX: respect endianness
        // ... followed by length
        uint32_t *len = block+block_size-sizeof(uint32_t);
        *len = (uint32_t)(*param+block_size);
        return DP_OK;
    }
    else
    {
        for (size_t i = filled; i<block_size; i++)
            block[i] = (uint8_t)(rand()%255);
        *param = block_size-filled;
        return DP_MORE;
    }
}

int dp_depad(const uint8_t *block, size_t block_size, size_t *pad_size)
{
    const uint32_t *len = block+block_size-sizeof(uint32_t);
    size_t sz = *len;
    if (sz>=block_size)
    {
        *pad_size = sz;
        return DP_OK;
    }
    return DP_ERR;
}
