/* SO 5308-6674 */
/* Inspired by the deleted SO question */
/* Determine the next prime after a given number */

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>

#define NEXT_PRIME_AFTER    /* Avoid unnecessary checks in is_prime() */

#ifdef TEST
static unsigned primes[] = { 2, 3, 5, 7, 11 };
#else
static unsigned primes[] =
{
       2,    3,    5,    7,   11,   13,   17,   19,   23,   29,
      31,   37,   41,   43,   47,   53,   59,   61,   67,   71,
      73,   79,   83,   89,   97,  101,  103,  107,  109,  113,
     127,  131,  137,  139,  149,  151,  157,  163,  167,  173,
     179,  181,  191,  193,  197,  199,  211,  223,  227,  229,
     233,  239,  241,  251,  257,  263,  269,  271,  277,  281,
     283,  293,  307,  311,  313,  317,  331,  337,  347,  349,
     353,  359,  367,  373,  379,  383,  389,  397,  401,  409,
     419,  421,  431,  433,  439,  443,  449,  457,  461,  463,
     467,  479,  487,  491,  499,  503,  509,  521,  523,  541,
     547,  557,  563,  569,  571,  577,  587,  593,  599,  601,
     607,  613,  617,  619,  631,  641,  643,  647,  653,  659,
     661,  673,  677,  683,  691,  701,  709,  719,  727,  733,
     739,  743,  751,  757,  761,  769,  773,  787,  797,  809,
     811,  821,  823,  827,  829,  839,  853,  857,  859,  863,
     877,  881,  883,  887,  907,  911,  919,  929,  937,  941,
     947,  953,  967,  971,  977,  983,  991,  997, 1009, 1013,
};
#endif /* TEST */

enum { N_PRIMES = sizeof(primes) / sizeof(primes[0]) };

/*
** In the context of next_prime_after(), this function is never called
** upon to validate small numbers - numbers less than primes[N_PRIMES-1]
** are not passed here.  In more general contexts, the extra conditions
** in the conditionally compiled code are necessary for accuracy.
*/
static bool is_prime(unsigned p)
{
    for (int i = 0; i < N_PRIMES; i++)
    {
#ifndef NEXT_PRIME_AFTER
        if (p < primes[i])
            return false;
        if (p == primes[i])
            return true;
#endif /* NEXT_PRIME_AFTER */
        if (p % primes[i] == 0)
            return false;
    }
    for (unsigned t = primes[N_PRIMES - 1]; t * t <= p; t += 6)
    {
        if (p % t == 0)
            return false;
        if (p % (t + 2) == 0)
            return false;
    }
    return true;
}

static unsigned next_prime_after(unsigned start)
{
    for (int i = 0; i < N_PRIMES; i++)
    {
        /* bsearch for first value more than start? */
        if (start < primes[i])
            return primes[i];
    }
    for (unsigned x = (start + 1) / 6; x < UINT_MAX / 6; x++)
    {
        unsigned t = 6 * x - 1;
        if (t > start && is_prime(t))
            return(t);
        t += 2;
        if (t > start && is_prime(t))
            return(t);
    }
    return 0;
}

static unsigned next_prime_before(unsigned start)
{
    if (start <= primes[0])
        return 0;           /* There isn't a prime before 2 */
    if (start < primes[N_PRIMES - 1])
    {
        /* bsearch for first value less than start? */
        for (int i = N_PRIMES - 2; i >= 0; i--)
        {
            //printf("%d [%d:%d]\n", start, primes[i], primes[i+1]);
            if (start > primes[i] && start <= primes[i + 1])
                return primes[i];
        }
        assert(0);
    }
    for (unsigned x = (start + 1) / 6; x > 1; x--)
    {
        unsigned t = 6 * x + 1;
        if (t < start && is_prime(t))
            return(t);
        t -= 2;
        if (t < start && is_prime(t))
            return(t);
    }
    return 0;
}

int main(void)
{
    assert((primes[N_PRIMES-1]+1) % 6 == 0);
    for (unsigned u = 0; u < 100; u++)
        printf("%3u < %3u < %3u\n", next_prime_before(u), u, next_prime_after(u));
    for (unsigned t = 100, u = next_prime_after(t); u < 12345678; t = u)
        printf("%3u < %3u < %3u\n", next_prime_before(t), t, (u = next_prime_after(t)));
}
