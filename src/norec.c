#include <alloc.h>
#include <assert.h>
#include <perfcounter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "norec.h"
#include "utils.h"

#define FILTERHASH(a) ((UNS(a) >> 2) ^ (UNS(a) >> 5))
#define FILTERBITS(a) (1 << (FILTERHASH(a) & 0x1F))

enum
{
    TX_ACTIVE = 1,
    TX_COMMITTED = 2,
    TX_ABORTED = 4
};

#include "thread_def.h"

volatile long *LOCK;

// --------------------------------------------------------------

static inline unsigned long long
MarsagliaXORV(unsigned long long x)
{
    if (x == 0)
    {
        x = 1;
    }

    x ^= x << 6;
    x ^= x >> 21;
    x ^= x << 7;

    return x;
}

static inline unsigned long long
MarsagliaXOR(TYPE unsigned long long *seed)
{
    unsigned long long x = MarsagliaXORV(*seed);
    *seed = x;

    return x;
}

static inline unsigned long long
TSRandom(TYPE Thread *Self)
{
    return MarsagliaXOR(&Self->rng);
}

static inline void
backoff(TYPE Thread *Self, long attempt)
{
    unsigned long long stall = TSRandom(Self) & 0xF;
    stall += attempt >> 2;
    stall *= 10;

    // stall = stall << attempt;
    /* CCM: timer function may misbehave */
    volatile unsigned long long i = 0;
    while (i++ < stall)
    {
        PAUSE();
    }
}

void
TxAbort(TYPE Thread *Self)
{
    Self->Aborts++;

#ifdef BACKOFF
    Self->Retries++;
    if (Self->Retries > 3)
    { /* TUNABLE */
        backoff(Self, Self->Retries);
    }
#endif

    Self->status = TX_ABORTED;

    // SIGLONGJMP(*Self->envPtr, 1);
    // ASSERT(0);
}

void
TxInit(TYPE Thread *t, int id)
{
    memset(t, 0, sizeof(*t)); /* Default value for most members */

    t->UniqID = id;
    t->rng = id + 1;
    t->xorrng[0] = t->rng;
    t->Starts = 0;
    t->Aborts = 0;

    t->rdSet.size = R_SET_SIZE;
    t->wrSet.size = W_SET_SIZE;
}

// --------------------------------------------------------------

static inline void
txReset(TYPE Thread *Self)
{
    Self->rdSet.nb_entries = 0;
    Self->wrSet.nb_entries = 0;

    Self->wrSet.BloomFilter = 0;

    Self->status = TX_ACTIVE;
}

void
TxStart(TYPE Thread *Self)
{
    txReset(Self);

    MEMBARLDLD();

    Self->Starts++;

    do
    {
        Self->snapshot = *LOCK;
    } while ((Self->snapshot & 1) != 0);
}

// --------------------------------------------------------------

// returns -1 if not coherent
static inline long
ReadSetCoherent(TYPE Thread *Self)
{
    long time;
    TYPE r_entry_t *r;

    while (1)
    {
        MEMBARSTLD();
        time = *LOCK;
        if ((time & 1) != 0)
        {
            continue;
        }

        r = Self->rdSet.entries;
        for (int i = Self->rdSet.nb_entries; i > 0; i--, r++)
        {
            if (r->Valu != LDNF(r->Addr))
            {
                return -1;
            }
        }

        if (*LOCK == time)
        {
            break;
        }
    }

    return time;
}

intptr_t
TxLoad(TYPE Thread *Self, volatile TYPE_ACC intptr_t *Addr)
{
    intptr_t Valu;
    TYPE w_entry_t *w;
    TYPE r_entry_t *r;

    intptr_t msk = FILTERBITS(Addr);
    if ((Self->wrSet.BloomFilter & msk) == msk)
    {
        w = Self->wrSet.entries + (Self->wrSet.nb_entries - 1);
        for (int i = Self->wrSet.nb_entries; i > 0; i--, w--)
        {
            if (w->Addr == Addr)
            {
                return w->Valu;
            }
        }
    }

    MEMBARLDLD();
    Valu = LDNF(Addr);
    while (*LOCK != Self->snapshot)
    {
        long newSnap = ReadSetCoherent(Self);

        if (newSnap == -1)
        {
            TxAbort(Self);
            return 0;
        }

        Self->snapshot = newSnap;
        MEMBARLDLD();
        Valu = LDNF(Addr);
    }

    if (Self->rdSet.nb_entries == Self->rdSet.size)
    {
        printf("[WARNING] Reached RS extend\n");
        assert(0);
    }

    r = &Self->rdSet.entries[Self->rdSet.nb_entries++];
    r->Addr = Addr;
    r->Valu = Valu;

    return Valu;
}

// --------------------------------------------------------------

void
TxStore(TYPE Thread *Self, volatile TYPE_ACC intptr_t *addr, intptr_t valu)
{
    TYPE w_entry_t *w;

    Self->wrSet.BloomFilter |= FILTERBITS(addr);

    if (Self->wrSet.nb_entries == Self->wrSet.size)
    {
        printf("[WARNING] Reached WS extend\n");
        assert(0);
    }

    w = &Self->wrSet.entries[Self->wrSet.nb_entries++];
    w->Addr = addr;
    w->Valu = valu;
}

// --------------------------------------------------------------

static inline void
txCommitReset(TYPE Thread *Self)
{
    txReset(Self);
    Self->Retries = 0;

    Self->status = TX_COMMITTED;
}

static inline void
WriteBackForward(TYPE Thread *Self)
{
    TYPE w_entry_t *w;

    w = Self->wrSet.entries;
    for (unsigned int i = Self->wrSet.nb_entries; i > 0; i--, w++)
    {
        *(w->Addr) = w->Valu;
    }
}

static inline long
TryFastUpdate(TYPE Thread *Self)
{
acquire:
    acquire(LOCK);

    if (*LOCK != Self->snapshot)
    {
        release(LOCK);

        long newSnap = ReadSetCoherent(Self);
        if (newSnap == -1)
        {
            return 0; // TxAbort(Self);
        }

        Self->snapshot = newSnap;

        goto acquire;
    }

    *LOCK = Self->snapshot + 1;

    release(LOCK);

    {
        WriteBackForward(Self); /* write-back the deferred stores */
    }

    MEMBARSTST(); /* Ensure the above stores are visible  */
    *LOCK = Self->snapshot + 2;
    MEMBARSTLD();

    return 1; /* success */
}

int
TxCommit(TYPE Thread *Self)
{
    /* Fast-path: Optional optimization for pure-readers */
    if (Self->wrSet.nb_entries == 0)
    {
        txCommitReset(Self);

        return 1;
    }

    if (TryFastUpdate(Self))
    {
        txCommitReset(Self);

        return 1;
    }

    TxAbort(Self);

    return 0;
}