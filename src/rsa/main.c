#include "config.h"
#include "common.h"
#include "rsa.h"
#include "dumb_padding.h"
#include "rsa_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
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
    rsa_save_key(public_key, n, e);
    rsa_save_key(private_key, n, d);
    // XXX: zeroize keys before free?
    bigint_free(e);
    bigint_free(d);
    bigint_free(n);
    fclose(public_key);
    fclose(private_key);
    return 0;
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
    if (rsa_load_key(key, n, exp))
    {
        puts("invalid key file.");
        return 1;
    }
    size_t src_block_size, dst_block_size;
    {
        size_t mod_size = bigint_get_size(n);
        size_t msg_size =
            mod_size-(1+sizeof(uint32_t)-bsize(n->data[n->size-1]));
        src_block_size = mode=='e' ? msg_size : mod_size;
        dst_block_size = mode=='e' ? mod_size : msg_size;
    }
    // decrypting: transform plain_blocks-1, apply special case for last block
    // encrypting: transform plain_blocks, apply special case for the rest
    size_t buf_sz = max(src_block_size, dst_block_size);
    uint8_t *buf = malloc(buf_sz);
    size_t zbytes = mode=='e' ? dst_block_size-src_block_size : 0;
    assert(zbytes<=sizeof(uint32_t));
    size_t bytes_read = 0;
    while (1)
    {
        // XXX: valid for little endian only!
        bytes_read = fread(buf, 1, src_block_size, src);        
        memset(buf+buf_sz-zbytes, 0, zbytes);
        if (bytes_read!=src_block_size)
            break;
        rsa_transform(buf, buf_sz, buf, exp, n);
        fwrite(buf, 1, dst_block_size, dst);
    }
    // process last block
    if (mode=='d')
    {
        size_t src_plain_size = src_size/src_block_size*dst_block_size;
        size_t padding = 0;
        if (dp_depad(buf, dst_block_size, &padding)!=DP_OK ||
            padding>src_plain_size)
        {
            puts("padding is invalid and cannot be removed.");
            return 1;
        }
        // trim to the actual plaintext size
        if (fseek(dst, 0, SEEK_SET) || ftrim(dst, src_plain_size-padding))
        {
            puts("can't set destination file size.");
            return 1;
        }
    }
    else
    {
        size_t param = 0;
        int pad_result = dp_pad(buf, src_block_size, bytes_read, &param);
        if (pad_result!=DP_OK)
        {
            if (pad_result!=DP_MORE)
            {
                puts("cannot apply padding.");
                return 1;
            }
            rsa_transform(buf, buf_sz, buf, exp, n);
            fwrite(buf, 1, buf_sz, dst);
            if (dp_pad(buf, src_block_size, 0, &param)!=DP_OK)
            {
                puts("cannot apply padding.");
                return 1;
            }
        }
        rsa_transform(buf, buf_sz, buf, exp, n);
        fwrite(buf, 1, buf_sz, dst);
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
