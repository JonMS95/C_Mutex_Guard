#ifndef TEST_MACROS_H
#define TEST_MACROS_H

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

#endif
