/********** Include statements ***********/

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include "MutexGuard_api.h"

/*****************************************/

/*********** Define statements ***********/

#define TEST_1_SEC_AS_NS    (uint64_t)1000000000

#define TEST_DEADLOCK_DEMO_HEADER       "\r\n********\r\nDEADLOCK\r\n********\r\n"
#define TEST_SELF_DEADLOCK_DEMO_HEADER  "\r\n*************\r\nSELF-DEADLOCK\r\n*************\r\n"
#define TEST_SCOPED_LIFETIME_MUTEX      "\r\n*********************\r\nSCOPED LIFETIME MUTEX\r\n*********************\r\n"

#define TEST_ERROR_MSG_LEN  1000

/*****************************************/

/******* Private type definitions ********/

typedef struct __attribute__((aligned(sizeof(size_t))))
{
    MTX_GRD* p_mtx_grd_0;
    MTX_GRD* p_mtx_grd_1;
} MTX_GRD_TEST_PAIR;

/*****************************************/

/****** Private function prototypes ******/

static void* testMutexGuardDeadlockRoutine(void* arg);
static void TestDemoDeadlock();
static void TestDemoSelfDeadlock();

/*****************************************/

/********** Function definitions *********/


/// @brief A helper function meant to be used alongside TestDemoDeadlock.
/// @param arg A MTX_GRD_TEST_PAIR struct instance containing pointers to a couple of MTX_GRD pointers.
/// @return NULL, simply to comply with thread routine standards.
static void* testMutexGuardDeadlockRoutine(void* arg)
{
    // Set the routine-executing thread as cancellable. 
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    // Cast arg as a pointer to MTX_GRD_TEST_PAIR.
    MTX_GRD_TEST_PAIR* test_pair = (MTX_GRD_TEST_PAIR*)arg;

    // Try to lock mutex (scoped).
    MTX_GRD_TRY_LOCK_SC(test_pair->p_mtx_grd_0, cleanup_ptr_0);
    
    // Wait for a second so that each thread has locked one of the mutexes (this will cause the deadlock).
    sleep(1);

    // Try to lock it for a given period of time. It won't happen, since the other thread has already locked it.
    MTX_GRD_TIMED_LOCK_SC(test_pair->p_mtx_grd_0, TEST_1_SEC_AS_NS, cleanup_ptr_1);

    return NULL;
}

/// @brief Intentionally causes a deadlock between two threads, which try to lock two mutexes in different order each time.
static void TestDemoDeadlock()
{
    printf("%s\r\n", TEST_DEADLOCK_DEMO_HEADER);

    // Create two MTX_GRD variables.
    MTX_GRD_CREATE(mtx_grd_0);
    MTX_GRD_CREATE(mtx_grd_1);

    // Initialize MTX_GRD variables by ensuring they will be destroyed when the current function ends.
    MTX_GRD_INIT_SC(&mtx_grd_0, p_dummy_0);
    MTX_GRD_INIT_SC(&mtx_grd_1, p_dummy_1);

    // Create two different couples of pointer pairs to the previously created MTX_GRD variables.
    // Choose a different order for each. 
    MTX_GRD_TEST_PAIR test_pair_0 =
    {
        .p_mtx_grd_0 = &mtx_grd_0,
        .p_mtx_grd_1 = &mtx_grd_1,
    };

    MTX_GRD_TEST_PAIR test_pair_1 =
    {
        .p_mtx_grd_0 = &mtx_grd_1,
        .p_mtx_grd_1 = &mtx_grd_0,
    };

    // Create two thread type variables.
    pthread_t t_0, t_1;

    // Update level for lock errors to be automatically displayed.
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_LOCK_ERROR);

    // Create (start) the threads. Both will execute the same routine, but they will try to unlock the mutexes in different order in each case.
    if(pthread_create(&t_0, NULL, testMutexGuardDeadlockRoutine, &test_pair_0))
        return;
    
    if(pthread_create(&t_1, NULL, testMutexGuardDeadlockRoutine, &test_pair_1))
    {
        pthread_cancel(t_0);
        return;
    }
    
    // Wait for the priorly created threads to join the main one.
    pthread_join(t_0, NULL);
    pthread_join(t_1, NULL);

    // Set verbosity level as silent again.
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT);
}

/// @brief A function causing a self-deadlock caused by a single thread trying to lock a non-recursive MTX_GRD twice in a row.
static void TestDemoSelfDeadlock()
{
    printf("%s\r\n", TEST_SELF_DEADLOCK_DEMO_HEADER);

    // Ensure no error is automatically shown this time. Instead, it's going to be retrieved by using proper functions then printed.
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT);

    // Create a MTX_GRD variable.
    MTX_GRD_CREATE(mtx_grd_0);

    // Initialize its attributes so as to make the mutex non-recursive by making sure it will be destroyed when the current function ends. 
    MTX_GRD_ATTR_INIT_SC(&mtx_grd_0, PTHREAD_MUTEX_TIMED_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_0);

    // Initialize MTX_GRD variable. Same as the attributes variable, it will be destroyed afterwards.
    MTX_GRD_INIT_SC(&mtx_grd_0, p_dummy_0);

    // Now try to acquire the mutex twice. As it's non-recursive, a deadlock will be caused.    
    // If any error happens, then copy it to the error string so as to clarify what was the reason.
    if(MTX_GRD_LOCK(&mtx_grd_0))
    {
        printf("%s\r\n", MTX_GRD_GET_LAST_ERR_STR);
        return;
    }

    // If no error happened, go ahead and try to lock it again (it won't be possible since the mutex has been initialized as non-recursive).
    if(MTX_GRD_TRY_LOCK(&mtx_grd_0))
        printf("%s\r\n", MTX_GRD_GET_LAST_ERR_STR);
    
    MTX_GRD_UNLOCK(&mtx_grd_0);

    // No error should occur after locking the mutex priorly declared beyond this point.
    MTX_GRD_LOCK_SC(&mtx_grd_0, p_dummy_lock_0);
    if(!p_dummy_lock_0)
        printf("%s\r\n", MTX_GRD_GET_LAST_ERR_STR);
}

/// @brief Creates a scoped MTX_GRD just to test that the mutex is automatically unlocked if necessary. 
static void TestScopeLifetimeMutex(void)
{
    printf("%s\r\n", TEST_SCOPED_LIFETIME_MUTEX);

    MTX_GRD_CREATE(mtx_grd);

    {
        MTX_GRD_ATTR_INIT_SC(&mtx_grd, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, p_mtx_grd_attr);
        MTX_GRD_INIT_SC(&mtx_grd, p_mtx_grd);

        for(int lock_idx = 0; lock_idx < __MTX_GRD_ADDR_NUM__; lock_idx++)
            MTX_GRD_TRY_LOCK(p_mtx_grd);
    }

    if(MTX_GRD_UNLOCK(&mtx_grd) < 0)
        printf("%s\r\n", MTX_GRD_GET_LAST_ERR_STR);
}

void TestDemo()
{
    TestDemoDeadlock();
    TestDemoSelfDeadlock();
    TestScopeLifetimeMutex();
}

/*****************************************/
