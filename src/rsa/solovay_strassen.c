#include "config.h"
#include "solovay_strassen.h"
#include "bigint.h"
#include <stdlib.h>

// a^(n-1)/2 != Ja(a, n)%n
static int is_euler_witness(int a, bigint_t *n, bigint_t *prealloc[4])
{
    bigint_t *ab = prealloc[0];
    bigint_t *res = prealloc[1];
    bigint_t *pow = prealloc[2];
    bigint_t *modpow = prealloc[3];
    bigint_fromint(ab, a);
    int x = bigint_jacobi(ab, n);
    if (x==-1)
        bigint_sub(res, n, &small_bigint[1]);
    else
        bigint_fromint(res, x);
    bigint_copy(n, pow);
    bigint_isub(pow, &small_bigint[1]);
    bigint_idiv(pow, &small_bigint[2]);
    bigint_modpow(ab, pow, n, modpow);
    int result = !bigint_equal(res, &small_bigint[0]) &&
        bigint_equal(modpow, res);
    return result;
}

int is_prime_ss(bigint_t *n, size_t k)
{
    // early out for n=2
    if (bigint_equal(n, &small_bigint[2]))
        return 1;
    // early out for n=2*m and n=1
    if (n->data[0]%2==0 || bigint_equal(n, &small_bigint[1]))
        return 0;
    bigint_t *prealloc[4];
    for (size_t i = 0; i<4; i++)
        prealloc[i] = bigint_alloc();
    int result = 1;
    while (k--)
    {
        // XXX: pick random bigint instead of plain int
        // 1] choose 1<a<n
        int rmax = n->size<=1 ? n->data[0] : RAND_MAX;
        int wit = rand()%(rmax-2)+2; // rand in range [2..n-1]
        // 2] check if 'wit' is a Euler witness for 'n'
        if (!is_euler_witness(wit, n, prealloc))
        {
            result = 0;
            break;
        }
    }
    for (size_t i = 0; i<4; i++)
        bigint_free(prealloc[i]);
    return result;
}
