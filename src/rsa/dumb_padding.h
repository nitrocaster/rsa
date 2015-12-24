#pragma once
#include "config.h"
#include "common.h"

enum
{
    DP_OK = 0,
    DP_MORE = 1,
    DP_ERR = -1
};

int dp_pad(uint8_t *block, size_t block_size, size_t filled, size_t *param);
int dp_depad(const uint8_t *block, size_t block_size, size_t *pad_size);
