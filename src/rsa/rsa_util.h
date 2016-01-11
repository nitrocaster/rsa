#pragma once
#include "config.h"
#include "common.h"
#include "bigint.h"
#include <stdio.h>

int rsa_load_key(FILE *f, bigint_t *n, bigint_t *exp);
void rsa_save_key(FILE *f, bigint_t *n, bigint_t *exp);
