#include "config.h"
#include "common.h"
#include "bigint.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// reference implementation by michael@pantaloons.co.nz

static uint32_t small_bigint_data[17][1] = {
    {0x0}, {0x1}, {0x2}, {0x3}, {0x4}, {0x5}, {0x6}, {0x7},
    {0x8}, {0x9}, {0xa}, {0xb}, {0xc}, {0xd}, {0xe}, {0xf},
    {0x10}
};

bigint_t small_bigint[17] = {
    {1, 1, small_bigint_data[0]},
    {1, 1, small_bigint_data[1]},
    {1, 1, small_bigint_data[2]},
    {1, 1, small_bigint_data[3]},
    {1, 1, small_bigint_data[4]},
    {1, 1, small_bigint_data[5]},
    {1, 1, small_bigint_data[6]},
    {1, 1, small_bigint_data[7]},
    {1, 1, small_bigint_data[8]},
    {1, 1, small_bigint_data[9]},
    {1, 1, small_bigint_data[10]},
    {1, 1, small_bigint_data[11]},
    {1, 1, small_bigint_data[12]},
    {1, 1, small_bigint_data[13]},
    {1, 1, small_bigint_data[14]},
    {1, 1, small_bigint_data[15]},
    {1, 1, small_bigint_data[16]}
};

static void bigint_reserve(bigint_t *b, size_t size)
{
    if (b->capacity < size)
    {
        b->capacity = size;
        b->data = realloc(b->data, size*sizeof(uint32_t));
    }
}

bigint_t *bigint_alloc_reserve(size_t capacity)
{
    bigint_t* b = malloc(sizeof(bigint_t)+capacity*sizeof(uint32_t));
    b->size = 1;
    b->capacity = capacity;
    b->data = calloc(capacity, sizeof(uint32_t));
    return b;
}

void bigint_free(bigint_t *b)
{
    free(b->data);
    free(b);
}

// data_size must be a multiple of 4
void bigint_load(bigint_t *b, uint8_t *buf, size_t buf_size)
{
    b->size = buf_size/4;
    bigint_reserve(b, b->size);
    memcpy(b->data, buf, b->size*sizeof(uint32_t));
}

// the buffer pointed by data must be at least 'b->size' bytes long
void bigint_save(bigint_t *b, uint8_t *buf)
{ memcpy(buf, b->data, b->size*sizeof(uint32_t)); }

int bigint_iszero(bigint_t* b)
{ return !b->size || b->size==1 && !b->data[0]; }

void bigint_copy(bigint_t *src, bigint_t *dst)
{
    dst->size = src->size;
    bigint_reserve(dst, src->capacity);
    memcpy(dst->data, src->data, dst->size*sizeof(uint32_t));
}

// Load a bigint from a base 10 string. Only pure numeric strings will work.
// base: x,d
void bigint_fromstring(bigint_t *b, char* str, char base)
{
    size_t len = strlen(str);
    size_t base_sz = base=='d' ? 10 : 16;
    for (size_t i = 0; i<len; i++)
    {
        if (i)
            bigint_imul(b, &small_bigint[base_sz]);
        char c = str[i];
        if (c>='a')
            c = c-'a'+10;
        else
            c = c-'0';
        bigint_iadd(b, &small_bigint[c]);
    }
}

// Load a bigint from an unsigned integer.
void bigint_fromint(bigint_t *b, uint32_t num)
{
    b->size = 1;
    bigint_reserve(b, b->size);
    b->data[0] = num;
}

// Print a bigint to stdout as base 10 integer. This is done by repeated
// division by 10. We can make it more efficient by dividing by 10^9
// for example, then doing single precision arithmetic to retrieve the
// 9 remainders.
// format: X,x,d
void bigint_print(bigint_t *b, char format)
{
    size_t cap = 100;    
    if (!b->size || bigint_iszero(b))
    {
        printf("0");
        return;
    }
    char* buffer = malloc(cap);
    bigint_t *copy = bigint_alloc();
    bigint_t *rem = bigint_alloc();
    bigint_copy(b, copy);
    size_t len = 0;
    size_t base = format=='d' ? 10 : 16;
    while (!bigint_iszero(copy))
    {
        bigint_idivr(copy, &small_bigint[base], rem);
        buffer[len++] = rem->data[0];
        if (len>=cap)
        {
            cap *= 2;
            buffer = realloc(buffer, cap);
        }
    }
    char format_str[3] = {'%', format, 0};
    for (size_t i = len; i--;)
        printf(format_str, buffer[i]);
    free(buffer);
    bigint_free(copy);
    bigint_free(rem);
}

// Check if two bigints are equal.
int bigint_equal(bigint_t *b1, bigint_t *b2)
{
    if (bigint_iszero(b1) && bigint_iszero(b2))
        return 1;
    else if (bigint_iszero(b1))
        return 0;
    else if (bigint_iszero(b2))
        return 0;
    else if (b1->size!=b2->size)
        return 0;
    for (size_t i = b1->size; i--;)
    {
        if (b1->data[i] != b2->data[i])
            return 0;
    }
    return 1;
}

// Check if bigint b1 is greater than b2
int bigint_greater(bigint_t *b1, bigint_t *b2)
{
    if (bigint_iszero(b1) && bigint_iszero(b2))
        return 0;
    else if (bigint_iszero(b1))
        return 0;
    else if (bigint_iszero(b2))
        return 1;
    else if (b1->size!=b2->size)
        return b1->size > b2->size;
    for (size_t i = b1->size; i--;)
    {
        if (b1->data[i]!=b2->data[i])
            return b1->data[i] > b2->data[i];
    }
    return 0;
}

// Check if bigint b1 is less than b2
int bigint_less(bigint_t *b1, bigint_t *b2)
{
    if (bigint_iszero(b1) && bigint_iszero(b2))
        return 0;
    else if (bigint_iszero(b1))
        return 1;
    else if (bigint_iszero(b2))
        return 0;
    else if (b1->size!=b2->size)
        return b1->size < b2->size;
    for (size_t i = b1->size; i--;)
    {
        if (b1->data[i]!=b2->data[i])
            return b1->data[i] < b2->data[i];
    }
    return 0;
}

// Check if bigint b1 is greater than or equal to b2
int bigint_geq(bigint_t *b1, bigint_t *b2)
{ return !bigint_less(b1, b2); }

// Check if bigint b1 is less than or equal to b2
int bigint_leq(bigint_t *b1, bigint_t *b2)
{ return !bigint_greater(b1, b2); }

void bigint_iadd32(bigint_t *src, uint32_t b2)
{
    bigint_t *temp = bigint_alloc();
    bigint_add32(temp, src, b2);
    bigint_copy(temp, src);
    bigint_free(temp);
}

void bigint_add32(bigint_t *result, bigint_t *b1, uint32_t b2)
{
    size_t n = b1->size;
    bigint_reserve(result, n+1);
    uint32_t carry = 0;
    for (size_t i = 0; i<n; i++)
    {
        uint32_t sum = carry;
        if (i < b1->size)
            sum += b1->data[i];
        if (i<1)
            sum += b2;
        // Already taken mod 2^32 by unsigned wrap around
        result->data[i] = sum;
        // Result must have wrapped 2^32 so carry bit is 1
        if (i < b1->size)
            carry = sum < b1->data[i];
        else
            carry = sum<b2;
    }
    if (carry==1)
    {
        result->size = n+1;
        result->data[n] = 1;
    }
    else
        result->size = n;
}

// Perform an in place add into the source bigint.
// src += add
void bigint_iadd(bigint_t *src, bigint_t *add)
{
    bigint_t *temp = bigint_alloc();
    bigint_add(temp, src, add);
    bigint_copy(temp, src);
    bigint_free(temp);
}

// Add two bigints by the add with carry method.
// result = b1 + b2
void bigint_add(bigint_t *result, bigint_t *b1, bigint_t *b2)
{
    size_t n = max(b1->size, b2->size);
    bigint_reserve(result, n+1);
    uint32_t carry = 0;
    for (size_t i = 0; i<n; i++)
    {
        uint32_t sum = carry;
        if (i < b1->size)
            sum += b1->data[i];
        if (i < b2->size)
            sum += b2->data[i];
        // Already taken mod 2^32 by unsigned wrap around
        result->data[i] = sum;
        // Result must have wrapped 2^32 so carry bit is 1
        if (i < b1->size)
            carry = sum < b1->data[i];
        else
            carry = sum < b2->data[i];
    }
    if (carry==1)
    {
        result->size = n+1;
        result->data[n] = 1;
    }
    else
        result->size = n;
}

// Perform an in place subtract from the source bigint.
// src -= sub
void bigint_isub(bigint_t *src, bigint_t *sub)
{
    bigint_t *temp = bigint_alloc();
    bigint_sub(temp, src, sub);
    bigint_copy(temp, src);
    bigint_free(temp);
}

// Subtract bigint b2 from b1.
// result = b1-b2
// The result is undefined if b2>b1.
// This uses the basic subtract with carry method.
void bigint_sub(bigint_t *dst, bigint_t *b1, bigint_t *b2)
{
    size_t length = 0;
    uint32_t carry = 0, diff, temp;
    bigint_reserve(dst, b1->size);
    for (size_t i = 0; i < b1->size; i++)
    {
        temp = carry;
        if (i < b2->size)
            temp += b2->data[i]; // Auto wrapped mod RADIX
        diff = b1->data[i]-temp;
        carry = temp > b1->data[i];
        dst->data[i] = diff;
        if (dst->data[i])
            length = i;
    }
    dst->size = length+1;
}

// Perform an in place multiplication into the source bigint.
// src *= mult
void bigint_imul(bigint_t *src, bigint_t *mult)
{
    bigint_t *temp = bigint_alloc();
    bigint_mul(temp, src, mult);
    bigint_copy(temp, src);
    bigint_free(temp);
}

// Multiply two bigints by the naive school method.
// dst = b1*b2
void bigint_mul(bigint_t *dst, bigint_t *b1, bigint_t *b2)
{
    size_t comp_size = b1->size + b2->size;
    bigint_reserve(dst, comp_size);
    memset(dst->data, 0, comp_size*sizeof(uint32_t));
    for (int i = 0; i < b1->size; i++)
    {
        for (int j = 0; j < b2->size; j++)
        {
            // This should not overflow...
            uint64_t prod = b1->data[i]*(uint64_t)b2->data[j]+
                (uint64_t)dst->data[i+j];
            uint32_t carry = (uint32_t)(prod/BIGINT_RADIX);
            // Add carry to the next uint32_t over, but this may cause
            // further overflow..
            int k = 1;
            while (carry)
            {
                uint32_t temp = dst->data[i+j+k]+carry;
                carry = temp < dst->data[i+j+k] ? 1 : 0;
                // Already wrapped in unsigned arithmetic
                dst->data[i+j+k] = temp;
                k++;
            }
            // Again, should not overflow...
            prod = (dst->data[i+j] + b1->data[i]*(uint64_t)b2->data[j])%
                BIGINT_RADIX;
            dst->data[i+j] = (uint32_t)prod; // add
        }
    }
    dst->size = comp_size;
    if (dst->size && !dst->data[dst->size-1])
        dst->size--;
}

// Perform an in place divide of source.
// src /= div
void bigint_idiv(bigint_t *src, bigint_t *div)
{
    bigint_t *q = bigint_alloc();
    bigint_t *r = bigint_alloc();
    bigint_div(q, r, src, div);
    bigint_copy(q, src);
    bigint_free(q);
    bigint_free(r);
}

// Perform an in place divide of source, also producing a remainder.
// src = src/div, rem = src%div
void bigint_idivr(bigint_t *src, bigint_t *div, bigint_t *rem)
{
    bigint_t *q = bigint_alloc();
    bigint_t *r = bigint_alloc();
    bigint_div(q, r, src, div);
    bigint_copy(q, src);
    bigint_copy(r, rem);
    bigint_free(q);
    bigint_free(r);
}

// Calculate the remainder when src is divided by div.
void bigint_rem(bigint_t *src, bigint_t *div, bigint_t *rem)
{
    bigint_t *q = bigint_alloc();
    bigint_div(q, rem, src, div);
    bigint_free(q);
}

// Modulate the source by the modulus.
// src %= mod
void bigint_imod(bigint_t *src, bigint_t *mod)
{
    bigint_t *q = bigint_alloc();
    bigint_t *r = bigint_alloc();
    bigint_div(q, r, src, mod);
    bigint_copy(r, src);
    bigint_free(q);
    bigint_free(r);
}

// Divide two bigints by naive long division, producing both
// quotient and remainder.
// q = floor(b1/b2), rem = b1-q*b2
// If b1<b2, the quotient is trivially 0 and remainder is b2.
void bigint_div(bigint_t *q, bigint_t *rem, bigint_t *b1, bigint_t *b2)
{
    bigint_t *b2copy = bigint_alloc();
    bigint_t *b1copy = bigint_alloc();
    bigint_t *temp = bigint_alloc();
    bigint_t *temp2 = bigint_alloc();
    bigint_t *temp3 = bigint_alloc();
    bigint_t *quottemp = bigint_alloc();
    uint32_t carry = 0;
    size_t length = 0;
    uint64_t factor = 1;
    uint64_t gquot, gtemp, grem;
    if (bigint_less(b1, b2))
    {
        // Trivial case: b1/b2 = 0 if b1<b2.
        q->size = 0;
        bigint_copy(b1, rem);
    }
    else if (bigint_iszero(b1))
    {
        // 0/x = 0.. assuming b2 is nonzero
        q->size = 0;
        bigint_fromint(rem, 0);
    }
    else if (b2->size==1)
    {
        // Division by a single limb means we can do simple division
        bigint_reserve(q, b1->size);
        for (size_t i = b1->size; i--;)
        {
            gtemp = carry*BIGINT_RADIX + b1->data[i];
            gquot = gtemp/b2->data[0];
            q->data[i] = (uint32_t)gquot;
            if (q->data[i] && !length)
                length = i+1;
            carry = gtemp%b2->data[0];
        }
        bigint_fromint(rem, carry);
        q->size = length;
    }
    else // Long division is neccessary
    {
        size_t n = b1->size+1;
        size_t m = b2->size;
        bigint_reserve(q, n-m);
        bigint_copy(b1, b1copy);
        bigint_copy(b2, b2copy);
        // Normalize.. multiply by the divisor by 2 until MSB >= HALFRADIX.
        // This ensures fast convergence when guessing the quotient below.
        // We also multiply the dividend by the same amount to ensure the
        // result does not change.
        while (b2copy->data[b2copy->size-1]<BIGINT_HALFRADIX)
        {
            factor *= 2;
            bigint_imul(b2copy, &small_bigint[2]);
        }
        if (factor>1)
        {
            bigint_fromint(temp, (uint32_t)factor);
            bigint_imul(b1copy, temp);
        }
        // Ensure the dividend is longer than the original (pre-normalized)
        // divisor. If it is not we introduce a dummy zero uint32_t to
        // artificially inflate it.
        if (b1copy->size!=n)
        {
            b1copy->size++;
            bigint_reserve(b1copy, b1copy->size);
            b1copy->data[n-1] = 0;
        }
        // Process quotient by long division
        for (size_t i = n-m; i--;)
        {
            gtemp = BIGINT_RADIX * b1copy->data[i+m] + b1copy->data[i+m-1];
            gquot = gtemp / b2copy->data[m-1];
            if (gquot>=BIGINT_RADIX)
                gquot = UINT_MAX;
            grem = gtemp % b2copy->data[m-1];
            while (grem<BIGINT_RADIX && gquot*b2copy->data[m-2] >
                BIGINT_RADIX*grem + b1copy->data[i+m-2])
            {
                // Should not overflow... ?
                gquot--;
                grem += b2copy->data[m-1];
            }
            quottemp->data[0] = gquot%BIGINT_RADIX;
            quottemp->data[1] = (uint32_t)(gquot/BIGINT_RADIX);
            quottemp->size = quottemp->data[1] ? 2 : 1;
            bigint_mul(temp2, b2copy, quottemp);
            bigint_reserve(temp3, m+1);
            temp3->size = 0;
            for (size_t j = 0; j<=m; j++)
            {
                temp3->data[j] = b1copy->data[i+j];
                if (temp3->data[j])
                    temp3->size = j+1;
            }
            if (bigint_less(temp3, temp2))
            {
                bigint_iadd(temp3, b2copy);
                gquot--;
            }
            bigint_isub(temp3, temp2);
            for (size_t j = 0; j < temp3->size; j++)
                b1copy->data[i+j] = temp3->data[j];
            for (size_t j = temp3->size; j<=m; j++)
                b1copy->data[i+j] = 0;
            q->data[i] = (uint32_t)gquot;
            if (q->data[i])
                q->size = i;
        }
        q->size = b1->size - b2->size;
        if (q->data[q->size])
            q->size++;
        // Divide by factor now to find final remainder
        carry = 0;
        for (size_t i = b1copy->size; i--;)
        {
            gtemp = carry*BIGINT_RADIX+b1copy->data[i];
            b1copy->data[i] = (uint32_t)(gtemp/factor);
            if (b1copy->data[i] && !length)
                length = i+1;
            carry = (uint32_t)(gtemp%factor);
        }
        b1copy->size = length;
        bigint_copy(b1copy, rem);
    }
    bigint_free(temp);
    bigint_free(temp2);
    bigint_free(temp3);
    bigint_free(b1copy);
    bigint_free(b2copy);
    bigint_free(quottemp);
}

// Perform modular exponentiation by repeated squaring.
// result = (base^exp)%mod
void bigint_modpow(bigint_t *base, bigint_t *exp, bigint_t *mod,
    bigint_t *result)
{
    bigint_t *a = bigint_alloc();
    bigint_t *b = bigint_alloc();
    bigint_t *c = bigint_alloc();
    bigint_t *discard = bigint_alloc();
    bigint_t *remainder = bigint_alloc();
    bigint_copy(base, a);
    bigint_copy(exp, b);
    bigint_copy(mod, c);
    bigint_fromint(result, 1);
    while (bigint_greater(b, &small_bigint[0]))
    {
        if (b->data[0] & 1)
        {
            bigint_imul(result, a);
            bigint_imod(result, c);
        }
        bigint_idiv(b, &small_bigint[2]);
        bigint_copy(a, discard);
        bigint_imul(a, discard);
        bigint_imod(a, c);
    }
    bigint_free(a);
    bigint_free(b);
    bigint_free(c);
    bigint_free(discard);
    bigint_free(remainder);
}

// Compute the gcd of two bigints.
// result = gcd(b1, b2)
void bigint_gcd(bigint_t *b1, bigint_t *b2, bigint_t *result)
{
    bigint_t *a = bigint_alloc();
    bigint_t *b = bigint_alloc();
    bigint_t *remainder = bigint_alloc();
    bigint_t *temp = bigint_alloc();
    bigint_t *discard = bigint_alloc();
    bigint_copy(b1, a);
    bigint_copy(b2, b);
    while (!bigint_equal(b, &small_bigint[0]))
    {
        bigint_copy(b, temp);
        bigint_imod(a, b);
        bigint_copy(a, b);
        bigint_copy(temp, a);
    }
    bigint_copy(a, result);
    bigint_free(a);
    bigint_free(b);
    bigint_free(remainder);
    bigint_free(temp);
    bigint_free(discard);
}

// Compute the inverse of a mod m.
// result = (a^-1)%m
void bigint_inv(bigint_t *a, bigint_t *m, bigint_t *result)
{
    bigint_t *remprev = bigint_alloc();
    bigint_t *rem = bigint_alloc();
    bigint_t *auxprev = bigint_alloc();
    bigint_t *aux = bigint_alloc();
    bigint_t *rcur = bigint_alloc();
    bigint_t *qcur = bigint_alloc();
    bigint_t *acur = bigint_alloc();
    bigint_copy(m, remprev);
    bigint_copy(a, rem);
    bigint_fromint(auxprev, 0);
    bigint_fromint(aux, 1);
    while (bigint_greater(rem, &small_bigint[1]))
    {
        bigint_div(qcur, rcur, remprev, rem);
        // Observe we are finding the inverse in a finite field so we can
        // use a modified algorithm that avoids negative numbers here
        bigint_sub(acur, m, qcur);
        bigint_imul(acur, aux);
        bigint_iadd(acur, auxprev);
        bigint_imod(acur, m);
        bigint_copy(rem, remprev);
        bigint_copy(aux, auxprev);
        bigint_copy(rcur, rem);
        bigint_copy(acur, aux);
    }
    bigint_copy(acur, result);
    bigint_free(remprev);
    bigint_free(rem);
    bigint_free(auxprev);
    bigint_free(aux);
    bigint_free(rcur);
    bigint_free(qcur);
    bigint_free(acur);
}

// Compute the jacobi symbol, J(ac, nc).
int bigint_jacobi(bigint_t *ac, bigint_t *nc)
{
    bigint_t *remainder = bigint_alloc();
    bigint_t *twos = bigint_alloc();
    bigint_t *temp = bigint_alloc();
    bigint_t *a = bigint_alloc();
    bigint_t *n = bigint_alloc();
    int mult = 1, result = 0;
    bigint_copy(ac, a);
    bigint_copy(nc, n);
    while (bigint_greater(a, &small_bigint[1]) && !bigint_equal(a, n))
    {
        bigint_imod(a, n);
        if (bigint_leq(a, &small_bigint[1]) || bigint_equal(a, n))
            break;
        bigint_fromint(twos, 0);
        /* Factor out multiples of two */
        while (a->data[0]%2==0)
        {
            bigint_iadd(twos, &small_bigint[1]);
            bigint_idiv(a, &small_bigint[2]);
        }
        // Coefficient for flipping
        if (bigint_greater(twos, &small_bigint[0]) && twos->data[0]%2==1)
        {
            bigint_rem(n, &small_bigint[8], remainder);
            if (!bigint_equal(remainder, &small_bigint[1]) &&
                !bigint_equal(remainder, &small_bigint[7]))
            {
                mult *= -1;
            }
        }
        if (bigint_leq(a, &small_bigint[1]) || bigint_equal(a, n))
            break;
        bigint_rem(n, &small_bigint[4], remainder);
        bigint_rem(a, &small_bigint[4], temp);
        if (!bigint_equal(remainder, &small_bigint[1]) &&
            !bigint_equal(temp, &small_bigint[1]))
        {
            mult *= -1;
        }
        bigint_copy(a, temp);
        bigint_copy(n, a);
        bigint_copy(temp, n);
    }
    if (bigint_equal(a, &small_bigint[1]))
        result = mult;
    else
        result = 0;
    bigint_free(remainder);
    bigint_free(twos);
    bigint_free(temp);
    bigint_free(a);
    bigint_free(n);
    return result;
}
