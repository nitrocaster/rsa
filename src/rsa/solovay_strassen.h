#pragma once
#include "config.h"
#include "common.h"
#include "bigint.h"

// Solovay-Strassen probabilistic primality test for 'k' rounds
int is_prime_ss(bigint_t *n, size_t k);
