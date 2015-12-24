#pragma once
#include "config.h"
#include "common.h"
#include "bigint.h"

// generates 3 parameters:
// e - public exponent, d - secret exponent, n - modulus
// keysize must be a multiple of 32
void rsa_generate_keypair(bigint_t *e, bigint_t *d, bigint_t *n,
    size_t keysize);

void rsa_transform(uint8_t *src, size_t src_size, uint8_t dst,
    bigint_t *exp, bigint_t *n);
