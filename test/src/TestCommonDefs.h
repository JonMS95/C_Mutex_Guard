#ifndef TEST_COMMON_DEFS_H
#define TEST_COMMON_DEFS_H

/********** Include statements ***********/

#include "MutexGuard_api.h"

/*****************************************/

/**************** Macros *****************/

#define ADD_SUITE(p_suite, suite_str)                       \
do                                                          \
{                                                           \
    if((p_suite = CU_add_suite(suite_str, 0, 0)) == NULL)   \
    {                                                       \
        CU_cleanup_registry();                              \
        return CU_get_error();                              \
    }                                                       \
} while (0);

#define ADD_TEST_2_SUITE(p_suite, test)             \
do                                                  \
{                                                   \
    if(CU_add_test(p_suite, #test, test) == NULL)   \
    {                                               \
        CU_cleanup_registry();                      \
        return CU_get_error();                      \
    }                                               \
} while(0);

/*****************************************/

/******** Common type definitions ********/

typedef struct __attribute__((aligned(sizeof(size_t))))
{
    MTX_GRD mtx_grd;
    int test_value;
} TEST_UNLOCK_HELPER_STRUCT;

/*****************************************/

#endif
