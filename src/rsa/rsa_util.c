#include "config.h"
#include "rsa_util.h"

int rsa_load_key(FILE *f, bigint_t *n, bigint_t *exp)
{
    // XXX: handle errors
    uint32_t n_size = 0, exp_size = 0;
    // read n_size, n, exp_size, exp
    fread(&n_size, sizeof(uint32_t), 1, f);
    uint8_t *buf = malloc(n_size);
    fread(buf, n_size, 1, f);
    bigint_load(n, buf, n_size);
    fread(&exp_size, sizeof(uint32_t), 1, f);
    buf = realloc(buf, exp_size);
    fread(buf, exp_size, 1, f);
    bigint_load(exp, buf, exp_size);
    free(buf);
    return 0;
}

void rsa_save_key(FILE *f, bigint_t *n, bigint_t *exp)
{
    uint32_t n_size = (uint32_t)bigint_get_size(n);
    uint32_t exp_size = (uint32_t)bigint_get_size(exp);
    uint8_t *buf = malloc(n_size+exp_size);
    // write n_size, n, exp_size, exp
    bigint_save(n, buf);
    fwrite(&n_size, sizeof(uint32_t), 1, f);
    fwrite(buf, n_size, 1, f);
    bigint_save(exp, buf);
    fwrite(&exp_size, sizeof(uint32_t), 1, f);
    fwrite(buf, exp_size, 1, f);
    free(buf);
}
