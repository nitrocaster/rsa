#include "config.h"
#include "rsa_util.h"
#include <stdlib.h>

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

static size_t bsize(uint32_t n)
{
    if (n & 0xff000000)
        return 4;
    if (n & 0x00ff0000)
        return 3;
    if (n & 0x0000ff00)
        return 2;
    if (n & 0x000000ff)
        return 1;
    return 0;
}

void rsa_get_block_sizes(char mode, bigint_t *n,
    size_t *src_block_size, size_t *dst_block_size)
{
    size_t mod_size = bigint_get_size(n);
    size_t msg_size =
        mod_size-(1+sizeof(uint32_t)-bsize(n->data[n->size-1]));
    *src_block_size = mode=='e' ? msg_size : mod_size;
    *dst_block_size = mode=='e' ? mod_size : msg_size;
}
