#include "config.h"
#include "rsa.h"
#include "bigint.h"
#include "solovay_strassen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define ACCURACY 20

static const char hex_digits[16] = {
    '0','1','2','3','4','5','6','7',
    '8','9','a','b','c','d','e','f'
};

static void rand_prime(size_t digit_count, bigint_t *result)
{
    char *str = malloc(digit_count+1);
    str[0] = hex_digits[rand()%9+1]; // 1..f
    str[digit_count-1] = hex_digits[(rand()%8)*2+1]; // odd digit
    for (int i = 1; i<digit_count-1; i++)
        str[i] = hex_digits[rand()%16];
    str[digit_count] = 0;
    bigint_fromstring(result, str, 'h');
    while (1)
    {
        if (is_prime_ss(result, ACCURACY))
        {
            free(str);
            return;
        }
        bigint_iadd(result, &small_bigint[2]);
    }
}

// the result is less than n and coprime to phi
static void rand_exponent(bigint_t *phi, int n, bigint_t *result)
{
    bigint_t *gcd = bigint_alloc();
    int e = rand()%n;
    while (1)
    {
        bigint_fromint(result, e);
        bigint_gcd(result, phi, gcd);
        if (bigint_equal(gcd, &small_bigint[1]))
        {
            bigint_free(gcd);
            return;
        }
        e = (e+1)%n;
        if (e<=2)
            e = 3;
    }
}

void rsa_generate_keypair(bigint_t *e, bigint_t *d, bigint_t *n,
    size_t keysize)
{
    assert(keysize%32==0);
    size_t factor_digits = keysize/8;
    bigint_t *p = bigint_alloc();
    bigint_t *q = bigint_alloc();
    bigint_t *phi = bigint_alloc();
    srand((uint32_t)time(NULL));
    // 1] pick two primes: p, q
    rand_prime(factor_digits, p);
    rand_prime(factor_digits, q);
    // 2] calculate modulus = p*q
    bigint_mul(n, p, q);
    // 3] calculate phi = (p-1)*(q-1)
    bigint_t *ps = bigint_alloc();
    bigint_t *qs = bigint_alloc();
    bigint_sub(ps, p, &small_bigint[1]);
    bigint_sub(qs, q, &small_bigint[1]);
    bigint_mul(phi, ps, qs);
    // 4] pick e (public exponent)
    rand_exponent(phi, RAND_MAX, e);
    // 5] calculate d (private exponent)
    bigint_inv(e, phi, d);
    bigint_free(p);
    bigint_free(q);
    bigint_free(phi);
}

void rsa_transform(uint8_t *src, size_t src_size, uint8_t *dst,
    bigint_t *exp, bigint_t *n)
{
    bigint_t *m = bigint_alloc();
    bigint_load(m, src, src_size);
    bigint_t *result = bigint_alloc();
    bigint_modpow(m, exp, n, result);
    assert(bigint_get_size(result)<=src_size);
    bigint_save(result, dst);
    bigint_free(result);
    bigint_free(m);
}
