// Compile with: gcc -g -Wall src/*.c -lpthread -o exe/main (use -D__MTX_GRD_FULL_BT__ too if required).

/********** Include statements ***********/

#include "MutexGuard_api.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

/*****************************************/

#define TEST_FN_TIME(function)   TestFunctionElapsedTime(function, #function)
#define NUM_OF_TESTS    100

#ifndef __MTX_GRD_TOUT_SECS__
#define __MTX_GRD_TOUT_SECS__   1
#endif

#define MTX_GRD_DEADLOCK_SLEEP_US   (uint32_t)1000000
#define MTX_GRD_TOUT_1_SEC_AS_NS    (uint64_t)1000000000
#define MTX_GRD_DEFAULT_TOUT        (uint64_t)(MTX_GRD_TOUT_1_SEC_AS_NS * __MTX_GRD_TOUT_SECS__)

#define MTX_GRD_MSG_ERR_THREAD_CREATION "Could not initialize thread <%d>.\r\n"

typedef struct
{
    MTX_GRD* p_mtx_grd_first;
    MTX_GRD* p_mtx_grd_second;
} MTX_GRD_TEST_PAIR;

typedef struct
{
    pthread_mutex_t* p_mtx_first;
    pthread_mutex_t* p_mtx_second;
} COMMON_MTX_TEST_PAIR;

static void* TestDbgThreadRoutine(void* arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    MTX_GRD_TEST_PAIR* p_mtx_grd_test_pair = (MTX_GRD_TEST_PAIR*)arg;

    MTX_GRD_TRY_LOCK_SC(p_mtx_grd_test_pair->p_mtx_grd_first, cleanup_var_first);

    usleep(MTX_GRD_DEADLOCK_SLEEP_US); // Let the current thread take a nap so as to cause a deadlock.

    MTX_GRD_TIMED_LOCK_SC(p_mtx_grd_test_pair->p_mtx_grd_second, MTX_GRD_DEFAULT_TOUT, cleanup_var_second);

    return NULL;
}

static void* TestCommonThreadRoutine(void* arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    COMMON_MTX_TEST_PAIR* p_mtx_test_pair = (COMMON_MTX_TEST_PAIR*)arg;

    int ret_lock = pthread_mutex_trylock(p_mtx_test_pair->p_mtx_first);

    usleep(MTX_GRD_DEADLOCK_SLEEP_US); // Let the current thread take a nap so as to cause a deadlock.

    struct timespec lock_timeout;
    clock_gettime(CLOCK_REALTIME, &lock_timeout);
    
    // Add the timeout (in nanoseconds) to the current time
    lock_timeout.tv_sec += MTX_GRD_DEFAULT_TOUT / MTX_GRD_TOUT_1_SEC_AS_NS;
    lock_timeout.tv_nsec += MTX_GRD_DEFAULT_TOUT % MTX_GRD_TOUT_1_SEC_AS_NS;
    
    // Handle possible overflow in nanoseconds (if it's >= 1 second)
    if (lock_timeout.tv_nsec >= MTX_GRD_TOUT_1_SEC_AS_NS)
    {
        lock_timeout.tv_nsec -= MTX_GRD_TOUT_1_SEC_AS_NS;
        lock_timeout.tv_sec += 1;
    }

    int timed_lock = pthread_mutex_timedlock(p_mtx_test_pair->p_mtx_second, &lock_timeout);

    if(!timed_lock)
    {
        printf("TO failed\n");
        pthread_mutex_unlock(p_mtx_test_pair->p_mtx_second);
    }
    
    if(!ret_lock)
        pthread_mutex_unlock(p_mtx_test_pair->p_mtx_first);

    return NULL;
}

void TestMutexGuard(void)
{
    pthread_t t_0, t_1;
    
    MTX_GRD_CREATE(mutex_guard_0);
    MTX_GRD_CREATE(mutex_guard_1);

    MTX_GRD_INIT_SC(&mutex_guard_0, cleanup_var_0);
    MTX_GRD_INIT_SC(&mutex_guard_1, cleanup_var_1);

    MTX_GRD_TEST_PAIR mutex_guard_test_pair_0 =
    {
        .p_mtx_grd_first    = &mutex_guard_0,
        .p_mtx_grd_second   = &mutex_guard_1,
    };

    MTX_GRD_TEST_PAIR mutex_guard_test_pair_1 =
    {
        .p_mtx_grd_first    = &mutex_guard_1,
        .p_mtx_grd_second   = &mutex_guard_0,
    };

    if(pthread_create(&t_0, NULL, TestDbgThreadRoutine, &mutex_guard_test_pair_0))
    {
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 0);

        return;
    }

    if(pthread_create(&t_1, NULL, TestDbgThreadRoutine, &mutex_guard_test_pair_1))
    {
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 1);
        pthread_cancel(t_0);

        return;
    }

    pthread_join(t_0, NULL);
    pthread_join(t_1, NULL);
}

void TestCommonMutexes(void)
{
    pthread_t t_0, t_1;

    pthread_mutex_t common_mutex_0, common_mutex_1;

    pthread_mutex_init(&common_mutex_0, NULL);
    pthread_mutex_init(&common_mutex_1, NULL);

    COMMON_MTX_TEST_PAIR common_mutex_test_pair_0 =
    {
        .p_mtx_first    = &common_mutex_0,
        .p_mtx_second   = &common_mutex_1,
    };

    COMMON_MTX_TEST_PAIR common_mutex_test_pair_1 =
    {
        .p_mtx_first    = &common_mutex_1,
        .p_mtx_second   = &common_mutex_0,
    };

    if(pthread_create(&t_0, NULL, TestCommonThreadRoutine, &common_mutex_test_pair_0))
    {
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 0);
        pthread_mutex_destroy(&common_mutex_0);
        pthread_mutex_destroy(&common_mutex_1);

        return;
    }

    if(pthread_create(&t_1, NULL, TestCommonThreadRoutine, &common_mutex_test_pair_1))
    {
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 1);
        pthread_cancel(t_0);
        pthread_mutex_destroy(&common_mutex_0);
        pthread_mutex_destroy(&common_mutex_1);

        return;
    }

    pthread_join(t_0, NULL);
    pthread_join(t_1, NULL);

    pthread_mutex_destroy(&common_mutex_0);
    pthread_mutex_destroy(&common_mutex_1);
}

double TestFunctionElapsedTime(void test_fn (void), char* test_fn_name)
{
    struct timespec start, end;
    double time_taken;

    // Record the start time
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Call the function you want to measure
    test_fn();

    // Record the end time
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate the elapsed time in seconds (and nanoseconds)
    time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[ %s ]\tTime taken: %f seconds\n", test_fn_name, time_taken);

    return time_taken;
}

int main()
{
    // double avg_time_common  = 0;
    // double avg_time_debug   = 0;

    // for(int i = 0; i < NUM_OF_TESTS; i++)
    // {
    //     avg_time_debug += (TEST_FN_TIME(TestMutexGuard) / NUM_OF_TESTS);
    //     avg_time_common += (TEST_FN_TIME(TestCommonMutexes) / NUM_OF_TESTS);
    // }

    // printf("Average time taken (common): %f\r\n", avg_time_common);
    // printf("Average time taken (debug):  %f\r\n", avg_time_debug);
    
    TestMutexGuard();

    return 0;
}