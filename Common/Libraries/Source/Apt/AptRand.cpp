#include "_Apt.h"
#include "MainInline.h"

// Mersenne Twister (thanks to Jaap Suter & SGADE).

#define SO_RAND_STATE_VECTOR_LENGTH (624)
#define SO_RAND_PERIOD (397)
#define SO_RAND_MAGIC (0x9908B0DFU)

#define SO_RAND_HI_BIT(u) ((u) & 0x80000000U)
#define SO_RAND_LO_BIT(u) ((u) & 0x00000001U)
#define SO_RAND_LO_BITS(u) ((u) & 0x7FFFFFFFU)
#define SO_RAND_MIX_BITS(u, v) (SO_RAND_HI_BIT(u) | SO_RAND_LO_BITS(v))

/** Next random value is computed from here. */
static unsigned int *s_RandNext;

/** Can do a *s_RandNext++ this many times before reloading. */
static int s_RandLeft = -1;

/** Random state vector, +1 extra to not violate ANSI C. */
static unsigned int s_RandState[SO_RAND_STATE_VECTOR_LENGTH + 1];

static unsigned int _randReloadMersenneTwister(void)
{
    unsigned int *p0 = s_RandState, *p2 = s_RandState + 2, *pM = s_RandState + SO_RAND_PERIOD, s0, s1;
    int j;

    if (s_RandLeft < -1)
        AptSeedRand(4357U);

    s_RandLeft = SO_RAND_STATE_VECTOR_LENGTH - 1;
    s_RandNext = s_RandState + 1;

    for (s0 = s_RandState[0], s1 = s_RandState[1], j = SO_RAND_STATE_VECTOR_LENGTH - SO_RAND_PERIOD + 1; --j; s0 = s1, s1 = *p2++)
    {
        *p0++ = *pM++ ^ (SO_RAND_MIX_BITS(s0, s1) >> 1) ^ (SO_RAND_LO_BIT(s1) ? SO_RAND_MAGIC : 0U);
    }

    for (pM = s_RandState, j = SO_RAND_PERIOD; --j; s0 = s1, s1 = *p2++)
    {
        *p0++ = *pM++ ^ (SO_RAND_MIX_BITS(s0, s1) >> 1) ^ (SO_RAND_LO_BIT(s1) ? SO_RAND_MAGIC : 0U);
    }

    s1 = s_RandState[0], *p0 = *pM ^ (SO_RAND_MIX_BITS(s0, s1) >> 1) ^ (SO_RAND_LO_BIT(s1) ? SO_RAND_MAGIC : 0U);
    s1 ^= (s1 >> 11);
    s1 ^= (s1 << 7) & 0x9D2C5680U;
    s1 ^= (s1 << 15) & 0xEFC60000U;

    return (s1 ^ (s1 >> 18));
}

uint32_t AptRand(void)
{
    unsigned int y;

    if (--s_RandLeft < 0)
        return (_randReloadMersenneTwister());

    y = *s_RandNext++;
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;

    return (y ^ (y >> 18));
}

void AptSeedRand(unsigned int nSeed)
{
    unsigned int x = (nSeed | 1U) & 0xFFFFFFFFU, *s = s_RandState;
    int j;

    for (s_RandLeft = 0, *s++ = x, j = SO_RAND_STATE_VECTOR_LENGTH;
         --j;
         *s++ = (x *= 69069U) & 0xFFFFFFFFU)
    {
        // nothing
    }
}
