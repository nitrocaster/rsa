#include "config.h"
#include "common.h"
#include "rsa.h"
#include "dumb_padding.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

static void print_usage()
{
    const char *usage_str = "usage: rsa {keygen|encrypt|decrypt} <args>\n"
        "args:\n"
        "  keygen:          <key size> <public key file> <private key file>\n"
        "  encrypt/decrypt: <key file> <source file> <destination file>";
    puts(usage_str);
}

static int fsize(const char *path, size_t *size)
{
    struct stat buffer;    
    if (stat(path, &buffer))
        return 1;
    *size = buffer.st_size;
    return 0;
}

static int ftrim(FILE *f, size_t new_size)
{
#ifndef _WIN32
    return ftruncate(fileno(f), new_size);
#else
    return _chsize(_fileno(f), (long)new_size)==-1;
#endif
}

static int load_key(FILE *f, bigint_t *n, bigint_t *exp)
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

static void save_key(FILE *f, bigint_t *n, bigint_t *exp)
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

static int run_keygen(int argc, char *argv[])
{
    // 0    1      2      3       4
    // rsa keygen key_sz pub_key priv_key
    uint32_t key_size = 0;
    if (sscanf(argv[2], "%u", &key_size)!=1)
    {
        puts("invalid key size (number expected).");
        return 1;
    }
    if (key_size%32)
    {
        puts("invalid key size (must me a multiple of 32).");
        return 1;
    }
    FILE *public_key = fopen(argv[3], "wb+");
    if (!public_key)
    {
        puts("can't open public key file.");
        return 1;
    }
    FILE *private_key = fopen(argv[4], "wb+");
    if (!public_key)
    {
        fclose(public_key);
        puts("can't open private key file.");
        return 1;
    }
    bigint_t *e = bigint_alloc();
    bigint_t *d = bigint_alloc();
    bigint_t *n = bigint_alloc();
    rsa_generate_keypair(e, d, n, key_size);
    save_key(public_key, n, e);
    save_key(private_key, n, d);
    // XXX: zeroize keys before free?
    bigint_free(e);
    bigint_free(d);
    bigint_free(n);
    fclose(public_key);
    fclose(private_key);
    return 0;
}

static int run_transform(int argc, char *argv[])
{
    // 0    1         2   3   4
    // rsa transform key src dst
    char mode = '-';
    if (!strcmp(argv[1], "encrypt"))
        mode = 'e';
    else if (!strcmp(argv[1], "decrypt"))
        mode = 'd';
    else
    {
        print_usage();
        return 1;
    }
    FILE *key = fopen(argv[2], "rb");
    if (!key)
    {
        puts("can't open key file.");
        return 1;
    }
    size_t src_size;
    if (fsize(argv[3], &src_size))
    {
        printf("can't stat %s.\n", argv[3]);
        return 1;
    }
    FILE *src = fopen(argv[3], "rb");
    if (!src)
    {
        puts("can't open source file.");
        return 1;
    }
    FILE *dst = fopen(argv[4], "wb+");
    if (!src)
    {
        puts("can't open destination file.");
        return 1;
    }
    bigint_t *n = bigint_alloc();
    bigint_t *exp = bigint_alloc();
    // XXX: free allocated resources on failure
    if (load_key(key, n, exp))
    {
        puts("invalid key file.");
        return 1;
    }
    size_t block_size = bigint_get_size(n);
    size_t blocks_done = 0;
    size_t plain_blocks = src_size/block_size;
    if (mode=='d')
        plain_blocks = plain_blocks>=1 ? plain_blocks-1 : 0;
    // decrypting: transform plain_blocks-1, apply special case for last block
    // encrypting: transform plain_blocks, apply special case for the rest
    uint8_t *buf = malloc(block_size*64);
    uint8_t *buf_ptr = buf;
    size_t remain = 0;
    while (1)
    {
        size_t bytes_read = fread(buf+remain, 1, block_size-remain, src);
        if (!bytes_read) // nothing remained, proceed with the last block
        {
            if (buf_ptr==buf+block_size)
                buf_ptr = buf;
            break;
        }
        bytes_read += remain;
        buf_ptr = buf;
        size_t blocks_read = bytes_read/block_size;
        remain = bytes_read%block_size;
        size_t stage_block_lim = min(blocks_done+blocks_read, plain_blocks);
        size_t blocks_staged = stage_block_lim-blocks_done;
        for (; blocks_done<stage_block_lim; blocks_done++)
        {
            rsa_transform(buf_ptr, block_size, buf_ptr, exp, n);
            buf_ptr += block_size;
        }
        fwrite(buf, 1, blocks_staged*block_size, dst);
        if (remain)
        {
            memmove(buf, buf_ptr, remain);
            buf_ptr = buf;
        }
    }
    // process last block
    if (mode=='d')
    {
        rsa_transform(buf_ptr, block_size, buf_ptr, exp, n);
        size_t padding = 0;
        if (dp_depad(buf_ptr, block_size, &padding)!=DP_OK)
        {
            puts("padding is invalid and cannot be removed.");
            return 1;
        }
        // trim to the actual plaintext size
        if (fseek(dst, 0, SEEK_SET) || ftrim(dst, src_size-padding))
        {
            puts("can't set destination file size.");
            return 1;
        }
    }
    else
    {
        size_t param = 0;
        if (dp_pad(buf_ptr, block_size, remain, &param)!=DP_MORE)
        {
            puts("cannot apply padding.");
            return 1;
        }
        rsa_transform(buf_ptr, block_size, buf_ptr, exp, n);
        fwrite(buf_ptr, 1, block_size, dst);
        if (dp_pad(buf_ptr, block_size, 0, &param)!=DP_OK)
        {
            puts("cannot apply padding.");
            return 1;
        }
        rsa_transform(buf_ptr, block_size, buf_ptr, exp, n);
        fwrite(buf_ptr, 1, block_size, dst);
    }
    free(buf);
    bigint_free(n);
    bigint_free(exp);
    fclose(key);
    fclose(src);
    fclose(dst);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc==5 && !strcmp(argv[1], "keygen"))
        return run_keygen(argc, argv);
    if (argc==5)
        return run_transform(argc, argv);
    print_usage();
    return 1;
}
