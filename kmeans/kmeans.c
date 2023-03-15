#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <barrier.h>
#include <defs.h>
#include <limits.h>
#include <mram.h>
#include <norec.h>
#include <perfcounter.h>

#include <thread_def.h>

#include "kmeans_macros.h"

BARRIER_INIT(kmeans_barr, NR_TASKLETS);

#ifdef TX_IN_MRAM
#define TYPE __mram_ptr
#else
#define TYPE
#endif

#include "util.h"

// Input variables
__mram float attributes[NUM_OBJECTS_PER_DPU * NUM_ATTRIBUTES];
__host __dma_aligned float current_cluster_centers[N_CLUSTERS * NUM_ATTRIBUTES];

// Output variables
__host float local_cluster_centers[N_CLUSTERS * NUM_ATTRIBUTES];
__host uint32_t local_centers_len[N_CLUSTERS];
__host uint64_t agregated_delta;

// Variables for local use
float delta_per_thread[NR_TASKLETS];
int membership[NUM_OBJECTS_PER_DPU];

#ifdef TX_IN_MRAM
Thread __mram_noinit t_mram[NR_TASKLETS];
#endif

float
euclidian_distance(float *pt1, float *pt2);
int
find_nearest_center(float *pt, float *centers);

int
main()
{
#ifndef TX_IN_MRAM
    Thread tx;
#endif
    uint64_t s;
    int tid;
    int index;
    int tmp_center_len;
    float tmp_center_attr;

    __dma_aligned float tmp_point[NUM_ATTRIBUTES];

    tid = me();
    s = (uint64_t)me();

#ifdef TX_IN_MRAM
    TxInit(&t_mram[tid], tid);
#else
    TxInit(&tx, tid);
#endif

    // -------------------------------------------------------------------

    if (MIN_N_CLUSTERS != MAX_N_CLUSTERS || USE_ZSCORE_TRANSFORM != 0)
    {
        assert(0);
    }

    if (tid == 0)
    {
        for (int i = 0; i < NUM_OBJECTS_PER_DPU; ++i)
        {
            membership[i] = -1;
        }

        for (int i = 0; i < N_CLUSTERS; ++i)
        {
            local_centers_len[i] = 0;
        }
    }
    barrier_wait(&kmeans_barr);

    // ==========================================================================

    delta_per_thread[tid] = 0;

    for (int i = tid; i < NUM_OBJECTS_PER_DPU; i += NR_TASKLETS)
    {
        mram_read(&attributes[i * NUM_ATTRIBUTES], tmp_point, sizeof(tmp_point));

        index = find_nearest_center(tmp_point, current_cluster_centers);
        // printf(">> %d\n", index);

        if (membership[i] != index)
        {
            delta_per_thread[tid] += 1.0;
        }

        membership[i] = index;

#ifdef TX_IN_MRAM
        START(&(t_mram[tid]));
#else
        START(&tx);
#endif

#ifdef TX_IN_MRAM
        tmp_center_len = LOAD(&(t_mram[tid]), &local_centers_len[index]);
#else
        tmp_center_len = LOAD(&tx, &local_centers_len[index]);
#endif

        tmp_center_len++;

#ifdef TX_IN_MRAM
        STORE(&(t_mram[tid]), &local_centers_len[index], tmp_center_len);
#else
        STORE(&tx, &local_centers_len[index], tmp_center_len);
#endif
        for (int j = 0; j < NUM_ATTRIBUTES; ++j)
        {
#ifdef TX_IN_MRAM
            intptr_t tmp = LOAD_LOOP(
                &(t_mram[tid]), &local_cluster_centers[(index * NUM_ATTRIBUTES) + j]);
#else
            intptr_t tmp =
                LOAD_LOOP(&tx, &local_cluster_centers[(index * NUM_ATTRIBUTES) + j]);
#endif
            tmp_center_attr = intp2double(tmp) + tmp_point[j];

#ifdef TX_IN_MRAM
            STORE_LOOP(&(t_mram[tid]),
                       &local_cluster_centers[(index * NUM_ATTRIBUTES) + j],
                       double2intp(tmp_center_attr));
#else
            STORE_LOOP(&tx, &local_cluster_centers[(index * NUM_ATTRIBUTES) + j],
                       double2intp(tmp_center_attr));
#endif
        }

#ifdef TX_IN_MRAM
        if (tx_mram[tid].status == 4)
        {
            continue;
        }
#else
        if (tx.status == 4)
        {
            continue;
        }
#endif

#ifdef TX_IN_MRAM
        COMMIT(&(t_mram[tid]));
#else
        COMMIT(&tx);
#endif
    }
    barrier_wait(&kmeans_barr);

    // ==========================================================================

    if (tid == 0)
    {
        agregated_delta = 0;
        for (int i = 0; i < NR_TASKLETS; ++i)
        {
            agregated_delta += delta_per_thread[i];
        }
    }
    barrier_wait(&kmeans_barr);

    return 0;
}

float
euclidian_distance(float *pt1, float *pt2)
{
    float ans = 0.0F;

    for (int i = 0; i < NUM_ATTRIBUTES; ++i)
    {
        ans += (pt1[i] - pt2[i]) * (pt1[i] - pt2[i]);
    }

    return ans;
}

int
find_nearest_center(float *pt, float *centers)
{
    int index = -1;
    float max_dist = 3.402823466e+38F; // TODO: might be a bug

    /* Find the cluster center id with min distance to pt */
    for (int i = 0; i < N_CLUSTERS; ++i)
    {
        float dist;
        /* no need square root */
        dist = euclidian_distance(pt, &centers[i * NUM_ATTRIBUTES]);

        if (dist < max_dist)
        {
            max_dist = dist;
            index = i;
            if (max_dist == 0)
            {
                break;
            }
        }
    }

    return index;
}
