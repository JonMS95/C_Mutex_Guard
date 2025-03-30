/********** Include statements ***********/

#include <pthread.h>
#include <unistd.h>
#include "TestCommonDefs.h"
#include "TestErrorCodes.h"

/*****************************************/

/********** Return Value Tests ***********/

static void TestSetPrintStatus()
{
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_MIN - 1);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1000);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Invalid verbosity level");

    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_MIN + 1);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1000);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Invalid verbosity level");
}

static void TestAttrInit()
{
    MutexGuardAttrInit(NULL, 0, 0, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");

    MTX_GRD_CREATE(test_mtx_grd);
    
    MutexGuardAttrInit(&test_mtx_grd, -1, 0, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1006);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Attribute setting failed");
}

static void TestAttrInitAddr()
{
    MutexGuardAttrInitAddr(NULL, 0, 0, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");
    
    MTX_GRD_CREATE(test_mtx_grd);
    
    MutexGuardAttrInitAddr(&test_mtx_grd, -1, 0, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1006);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Attribute setting failed");
}

static void TestInit()
{
    MutexGuardInit(NULL);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");
}

static void TestInitAddr()
{
    MutexGuardInitAddr(NULL);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");
}

static void TestGetLockError()
{
    MutexGuardGetLockError(NULL, 0, NULL, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");

    MTX_GRD_CREATE(test_mtx_grd);
    MTX_GRD_INIT_SC(&test_mtx_grd, dummy_mtx_grd);
    
    MutexGuardGetLockError(&test_mtx_grd, 0, NULL, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1004);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Target string to write is null");
}

static void TestLock()
{
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT);

    MutexGuardLock(NULL, NULL, 0, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");

    {
        MTX_GRD_CREATE(test_mtx_grd_0);
        MTX_GRD_INIT_SC(&test_mtx_grd_0, dummy_0);
        MutexGuardLock(&test_mtx_grd_0, NULL, 0, MTX_GRD_LOCK_TYPE_MIN - 1);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1008);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Invalid lock type provided");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_1);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_1, PTHREAD_MUTEX_TIMED_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_1);
        MTX_GRD_INIT_SC(&test_mtx_grd_1, dummy_1);
        MTX_GRD_TRY_LOCK(&test_mtx_grd_1);
        MTX_GRD_TRY_LOCK(&test_mtx_grd_1);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1009);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Could not lock target mutex (use MutexGuardGetLockError for more info)");

        char lock_error[200] = {0};
        MutexGuardGetLockError(&test_mtx_grd_1, 0, lock_error, sizeof(lock_error));

        CU_ASSERT_PTR_NOT_NULL(strstr(lock_error, "Thread with ID "));
        CU_ASSERT_PTR_NOT_NULL(strstr(lock_error, " cannot acquire mutex at "));
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_2);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_2, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_2);
        MTX_GRD_INIT_SC(&test_mtx_grd_2, dummy_2);
        
        for(int i = 0; i < __MTX_GRD_ADDR_NUM__; i++)
            ++test_mtx_grd_2.mutex_acq_location.addresses[i];
        
        MTX_GRD_TRY_LOCK(&test_mtx_grd_2);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1007);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "No space available for more addresses");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_3);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_3, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_3);
        MTX_GRD_INIT_SC(&test_mtx_grd_3, dummy_3);
        
        for(int i = 0; i < (__MTX_GRD_ADDR_NUM__ + 1); i++)
            MTX_GRD_TRY_LOCK(&test_mtx_grd_3);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1017);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Address counter is out of boundaries");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_4);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_4, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_4);
        MTX_GRD_INIT_SC(&test_mtx_grd_4, dummy_4);
        
        test_mtx_grd_4.lock_counter = __MTX_GRD_ADDR_NUM__ + 1;

        MTX_GRD_TRY_LOCK(&test_mtx_grd_4);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1017);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Address counter is out of boundaries");
    }
}

static void TestLockAddr()
{
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT);

    MutexGuardLockAddr(NULL, NULL, 0, 0);
    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");

    {
        MTX_GRD_CREATE(test_mtx_grd_0);
        MTX_GRD_INIT_SC(&test_mtx_grd_0, dummy_0);
        MutexGuardLockAddr(&test_mtx_grd_0, NULL, 0, MTX_GRD_LOCK_TYPE_MIN - 1);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1008);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Invalid lock type provided");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_1);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_1, PTHREAD_MUTEX_TIMED_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_1);
        MTX_GRD_INIT_SC(&test_mtx_grd_1, dummy_1);
        MTX_GRD_TRY_LOCK_SC(&test_mtx_grd_1, dummy_lock_1_0);
        MTX_GRD_TRY_LOCK_SC(&test_mtx_grd_1, dummy_lock_1_1);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1009);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Could not lock target mutex (use MutexGuardGetLockError for more info)");

        char lock_error[200] = {0};
        MutexGuardGetLockError(&test_mtx_grd_1, 0, lock_error, sizeof(lock_error));

        CU_ASSERT_PTR_NOT_NULL(strstr(lock_error, "Thread with ID "));
        CU_ASSERT_PTR_NOT_NULL(strstr(lock_error, " cannot acquire mutex at "));
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_2);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_2, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_2);
        MTX_GRD_INIT_SC(&test_mtx_grd_2, dummy_2);
        
        for(int i = 0; i < (__MTX_GRD_ADDR_NUM__ + 1); i++)
            ++test_mtx_grd_2.mutex_acq_location.addresses[i];

        MTX_GRD_TRY_LOCK_SC(&test_mtx_grd_2, dummy_lock_2);
        
        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1007);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "No space available for more addresses");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_3);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_3, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_3);
        MTX_GRD_INIT_SC(&test_mtx_grd_3, dummy_3);
        
        for(int i = 0; i < (__MTX_GRD_ADDR_NUM__ + 1); i++)
            MutexGuardLockAddr(&test_mtx_grd_3, MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_TRY);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1017);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Address counter is out of boundaries");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_4);
        MTX_GRD_ATTR_INIT_SC(&test_mtx_grd_4, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_PRIO_NONE, PTHREAD_PROCESS_PRIVATE, dummy_mtx_grd_attr_4);
        MTX_GRD_INIT_SC(&test_mtx_grd_4, dummy_4);
        
        test_mtx_grd_4.lock_counter = __MTX_GRD_ADDR_NUM__ + 1;

        MTX_GRD_TRY_LOCK(&test_mtx_grd_4);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1017);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Address counter is out of boundaries");
    }
}

static void* TestEvalHelper(void* arg)
{
    TEST_UNLOCK_HELPER_STRUCT* test_st = (TEST_UNLOCK_HELPER_STRUCT*)arg;
    test_st->fnMutexGuard(&test_st->mtx_grd);
    test_st->test_value = MutexGuardGetErrorCode();

    return NULL;
}

static void TestUnlock()
{
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT);

    MutexGuardUnlock(NULL);

    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");

    {
        MTX_GRD_CREATE(test_mtx_grd_0);
        MTX_GRD_UNLOCK(&test_mtx_grd_0);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1011);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Owner thread's TID does not match current one");
    }
    
    {
        TEST_UNLOCK_HELPER_STRUCT test_unlock_helper_struct = { .fnMutexGuard = &MutexGuardUnlock };
        MTX_GRD_INIT_SC(&test_unlock_helper_struct.mtx_grd, dummy_unlock_helper);
        MTX_GRD_LOCK(&test_unlock_helper_struct.mtx_grd);
        pthread_t thread_0;
        pthread_create(&thread_0, NULL, TestEvalHelper, &test_unlock_helper_struct);

        pthread_join(thread_0, NULL);
        MTX_GRD_UNLOCK(&test_unlock_helper_struct.mtx_grd);

        CU_ASSERT_EQUAL(test_unlock_helper_struct.test_value, 1011);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(test_unlock_helper_struct.test_value), "Owner thread's TID does not match current one");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_1);
        MTX_GRD_INIT_SC(&test_mtx_grd_1, dummy_1);
        MTX_GRD_LOCK_SC(&test_mtx_grd_1, dummy_lock_1);
        test_mtx_grd_1.mutex_acq_location.addresses[0] = NULL;
        MTX_GRD_UNLOCK(&test_mtx_grd_1);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1007);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "No space available for more addresses");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_2);
        MTX_GRD_INIT_SC(&test_mtx_grd_2, dummy_2);
        MTX_GRD_LOCK_SC(&test_mtx_grd_2, dummy_lock_2);
        test_mtx_grd_2.lock_counter = 0;
        MTX_GRD_UNLOCK(&test_mtx_grd_2);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1017);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Address counter is out of boundaries");
    }
}

static void TestAttrDestroy()
{
    CU_ASSERT_EQUAL(MutexGuardAttrDestroy(NULL), -1);

    MTX_GRD_CREATE(test_mtx_grd);
    MTX_GRD_ATTR_INIT(&test_mtx_grd, 0, 0, 0);
    MTX_GRD_INIT(&test_mtx_grd);

    CU_ASSERT_EQUAL(MutexGuardAttrDestroy(&test_mtx_grd), 0);
}

static void TestDestroy()
{
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT);

    MutexGuardDestroy(NULL);

    CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1001);
    CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "MTX_GRD null pointer");
    
    {
        TEST_UNLOCK_HELPER_STRUCT test_unlock_helper_struct = { .fnMutexGuard = &MutexGuardDestroy };
        MTX_GRD_INIT_SC(&test_unlock_helper_struct.mtx_grd, dummy_unlock_helper);
        MTX_GRD_LOCK(&test_unlock_helper_struct.mtx_grd);
        pthread_t thread_0;
        pthread_create(&thread_0, NULL, TestEvalHelper, &test_unlock_helper_struct);

        pthread_join(thread_0, NULL);

        CU_ASSERT_EQUAL(test_unlock_helper_struct.test_value, 1011);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(test_unlock_helper_struct.test_value), "Owner thread's TID does not match current one");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_1);
        MTX_GRD_INIT_SC(&test_mtx_grd_1, dummy_1);
        MTX_GRD_LOCK_SC(&test_mtx_grd_1, dummy_lock_1);
        test_mtx_grd_1.mutex_acq_location.addresses[0] = NULL;
        MTX_GRD_DESTROY(&test_mtx_grd_1);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1007);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "No space available for more addresses");
    }

    {
        MTX_GRD_CREATE(test_mtx_grd_2);
        MTX_GRD_INIT_SC(&test_mtx_grd_2, dummy_2);
        MTX_GRD_LOCK_SC(&test_mtx_grd_2, dummy_lock_2);
        test_mtx_grd_2.lock_counter = 0;
        MTX_GRD_DESTROY(&test_mtx_grd_2);

        CU_ASSERT_EQUAL(MutexGuardGetErrorCode(), 1010);
        CU_ASSERT_STRING_EQUAL(MutexGuardGetErrorString(MutexGuardGetErrorCode()), "Standard error code (use strerror for more info)");
    }
}

int CreateErrorCodeTestsSuite()
{
    CU_pSuite pErrorCodeTestsSuite;

    ADD_SUITE(pErrorCodeTestsSuite, "Error codes");

    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestSetPrintStatus);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestAttrInit);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestAttrInitAddr);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestInit);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestInitAddr);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestGetLockError);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestLock);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestLockAddr);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestUnlock);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestAttrDestroy);
    ADD_TEST_2_SUITE(pErrorCodeTestsSuite, TestDestroy);

    return 0;
}
/*****************************************/