/********** Include statements ***********/

#include "TestCommonDefs.h"
#include "TestReturnValues.h"

/*****************************************/

/********** Return Value Tests ***********/

static void TestSetPrintStatus()
{
    CU_ASSERT_EQUAL(MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_MIN - 1), -1);
    CU_ASSERT_EQUAL(MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_MAX + 1), -1);

    CU_ASSERT_EQUAL(MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_LOCK_ERROR), 0);
    CU_ASSERT_EQUAL(MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_BT),         0);
    CU_ASSERT_EQUAL(MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_ALL),        0);
    CU_ASSERT_EQUAL(MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT),     0);
}

static void TestAttrInit()
{
    CU_ASSERT_EQUAL(MutexGuardAttrInit(NULL, 0, 0, 0), -1);
    
    MTX_GRD_CREATE(test_mtx_grd);
    
    CU_ASSERT_EQUAL(MutexGuardAttrInit(&test_mtx_grd, -1, 0, 0), -2);
    CU_ASSERT_EQUAL(MutexGuardAttrInit(&test_mtx_grd, 0, -1, 0), -2);
    CU_ASSERT_EQUAL(MutexGuardAttrInit(&test_mtx_grd, 0, 0, -1), -2);

    CU_ASSERT_EQUAL(MutexGuardAttrInit(&test_mtx_grd, 0, 0, 0), 0);
}

static void TestAttrInitAddr()
{
    CU_ASSERT_PTR_NULL(MutexGuardAttrInitAddr(NULL, 0, 0, 0));
    
    MTX_GRD_CREATE(test_mtx_grd);
    
    CU_ASSERT_PTR_NULL(MutexGuardAttrInitAddr(&test_mtx_grd, -1, 0, 0));
    CU_ASSERT_PTR_NULL(MutexGuardAttrInitAddr(&test_mtx_grd, 0, -1, 0));
    CU_ASSERT_PTR_NULL(MutexGuardAttrInitAddr(&test_mtx_grd, 0, 0, -1));

    CU_ASSERT_PTR_NOT_NULL(MutexGuardAttrInitAddr(&test_mtx_grd, 0, 0, 0));
}

static void TestInit()
{
    CU_ASSERT_NOT_EQUAL(MutexGuardInit(NULL), 0);

    MTX_GRD_CREATE(test_mtx_grd);

    CU_ASSERT_EQUAL(MutexGuardInit(&test_mtx_grd), 0);
}

static void TestInitAddr()
{
    CU_ASSERT_PTR_NULL(MutexGuardInitAddr(NULL));

    MTX_GRD_CREATE(test_mtx_grd);

    CU_ASSERT_PTR_NOT_NULL(MutexGuardInitAddr(&test_mtx_grd));
}

static void TestGetLockError()
{
    CU_ASSERT_EQUAL(MutexGuardGetLockError(NULL, 0, NULL, 0), -1);
    
    MTX_GRD_CREATE(test_mtx_grd);
    MTX_GRD_INIT_SC(&test_mtx_grd, dummy);
    
    CU_ASSERT_EQUAL(MutexGuardGetLockError(&test_mtx_grd, 0, NULL, 0), -2);
    
    char str[10] = {0};

    CU_ASSERT_EQUAL(MutexGuardGetLockError(&test_mtx_grd, 0, str, 0), 0);
    CU_ASSERT_EQUAL(MutexGuardGetLockError(&test_mtx_grd, 0, str, 1), 0);
}

static void TestLock()
{
    CU_ASSERT_EQUAL(MutexGuardLock(NULL, NULL, 0, 0), -1);
    
    MTX_GRD_CREATE(test_mtx_grd);
    MTX_GRD_INIT_SC(&test_mtx_grd, dummy);

    CU_ASSERT_EQUAL(MutexGuardLock(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_MIN - 1), -2);
    CU_ASSERT_EQUAL(MutexGuardLock(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_MAX + 1), -2);

    CU_ASSERT_EQUAL(MutexGuardLock(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_TRY), 0);
    CU_ASSERT_NOT_EQUAL(MutexGuardLock(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_TRY), 0);
}

static void TestLockAddr()
{
    CU_ASSERT_PTR_NULL(MutexGuardLockAddr(NULL, NULL, 0, 0));
    
    MTX_GRD_CREATE(test_mtx_grd);
    MTX_GRD_INIT_SC(&test_mtx_grd, dummy);

    CU_ASSERT_PTR_NULL(MutexGuardLockAddr(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_MIN - 1));
    CU_ASSERT_PTR_NULL(MutexGuardLockAddr(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_MAX + 1));

    CU_ASSERT_PTR_NOT_NULL(MutexGuardLockAddr(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_TRY));
    CU_ASSERT_PTR_NULL(MutexGuardLockAddr(&test_mtx_grd, NULL, 0, MTX_GRD_LOCK_TYPE_TRY));
}

static void* TestUnlockHelper(void* arg)
{
    TEST_UNLOCK_HELPER_STRUCT* test_st = (TEST_UNLOCK_HELPER_STRUCT*)arg;
    test_st->test_value = MTX_GRD_UNLOCK(&test_st->mtx_grd);

    return NULL;
}

static void TestUnlock()
{
    CU_ASSERT_EQUAL(MutexGuardUnlock(NULL), -1);

    {
        TEST_UNLOCK_HELPER_STRUCT test_unlock_helper_struct = {0};
        MTX_GRD_INIT_SC(&test_unlock_helper_struct.mtx_grd, dummy_unlock_helper);
        MTX_GRD_LOCK_SC(&test_unlock_helper_struct.mtx_grd, test_lock_dummy);
        pthread_t thread_0;
        pthread_create(&thread_0, NULL, TestUnlockHelper, &test_unlock_helper_struct);

        pthread_join(thread_0, NULL);

        CU_ASSERT_EQUAL(test_unlock_helper_struct.test_value, -2);
    }

    MTX_GRD_CREATE(test_mtx_grd);
    MTX_GRD_INIT_SC(&test_mtx_grd, dummy_mtx);
    MTX_GRD_LOCK_SC(&test_mtx_grd, dummy_lock_1);

    CU_ASSERT_EQUAL(MutexGuardUnlock(&test_mtx_grd), 0);
}

int CreateReturnValueTestsSuite()
{
    CU_pSuite pReturnValueTestsSuite;

    ADD_SUITE(pReturnValueTestsSuite, "Return values");

    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestSetPrintStatus);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestAttrInit);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestAttrInitAddr);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestInit);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestInitAddr);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestGetLockError);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestLock);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestLockAddr);
    ADD_TEST_2_SUITE(pReturnValueTestsSuite, TestUnlock);

    return 0;
}
/*****************************************/