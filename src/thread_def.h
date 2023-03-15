#ifndef _THREAD_DEF_H_
#define _THREAD_DEF_H_

#include <perfcounter.h>

#define R_SET_SIZE 15 /* Initial size of read sets */
#define W_SET_SIZE 15 /* Initial size of write sets */

typedef int BitMap;

typedef struct r_entry
{ /* Read set entry */
    volatile TYPE_ACC intptr_t *Addr;
    intptr_t Valu;
} r_entry_t;

typedef struct r_set
{
    r_entry_t entries[R_SET_SIZE]; /* Array of entries */
    unsigned int nb_entries;       /* Number of entries */
    unsigned int size;             /* Size of array */
} r_set_t;

typedef struct w_entry
{ /* Write set entry */
    volatile TYPE_ACC intptr_t *Addr;
    intptr_t Valu;
    long Ordinal;
} w_entry_t;

typedef struct w_set
{
    w_entry_t entries[W_SET_SIZE]; /* Array of entries */
    unsigned int nb_entries;       /* Number of entries */
    unsigned int size;             /* Size of array */
    BitMap BloomFilter;            /* Address exclusion fast-path test */
} w_set_t;

struct _Thread
{
    w_set_t wrSet;
    r_set_t rdSet;
    unsigned long long rng;
    unsigned long long xorrng[1];
    volatile long Retries;
    long snapshot;
    long status;
    int UniqID;
    uint32_t Starts;
    uint32_t Aborts; /* Tally of # of aborts */
};

#endif
