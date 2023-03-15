#ifndef _MACROS_H_
#define _MACROS_H_

#define START(t)                                                                         \
    do                                                                                   \
    {                                                                                    \
        TxStart(t);

#define LOAD(t, addr)                                                                    \
    TxLoad(t, addr);                                                                     \
    if ((t)->status == 4)                                                                \
    {                                                                                    \
        continue;                                                                        \
    }

#define STORE(t, addr, val)                                                              \
    TxStore(t, (intptr_t *)addr, (intptr_t)val);                                         \
    if ((t)->status == 4)                                                                \
    {                                                                                    \
        continue;                                                                        \
    }

#define LOAD_LOOP(t, addr)                                                               \
    TxLoad(t, addr);                                                                     \
    if ((t)->status == 4)                                                                \
    {                                                                                    \
        break;                                                                           \
    }

#define STORE_LOOP(t, addr, val)                                                         \
    TxStore(t, (intptr_t *)addr, (intptr_t)val);                                         \
    if ((t)->status == 4)                                                                \
    {                                                                                    \
        break;                                                                           \
    }

#define COMMIT(t)                                                                        \
    TxCommit(t);                                                                         \
    if ((t)->status != 4)                                                                \
    {                                                                                    \
        break;                                                                           \
    }                                                                                    \
    }                                                                                    \
    while (1)

#define RAND_R_FNC(seed)                                                                 \
    ({                                                                                   \
        uint64_t next = (seed);                                                          \
        uint64_t result;                                                                 \
        next *= 1103515245;                                                              \
        next += 12345;                                                                   \
        result = (uint64_t)(next >> 16) & (2048 - 1);                                    \
        next *= 1103515245;                                                              \
        next += 12345;                                                                   \
        result <<= 10;                                                                   \
        result ^= (uint64_t)(next >> 16) & (1024 - 1);                                   \
        next *= 1103515245;                                                              \
        next += 12345;                                                                   \
        result <<= 10;                                                                   \
        result ^= (uint64_t)(next >> 16) & (1024 - 1);                                   \
        (seed) = next;                                                                   \
        result; /* returns result */                                                     \
    })

#endif /* _MACROS_H_ */