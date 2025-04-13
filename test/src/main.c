/********** Include statements ***********/

#include <CUnit/Basic.h>
#include "MutexGuard_api.h"
#include "TestReturnValues.h"
#include "TestErrorCodes.h"
#include "TestDemos.h"

/*****************************************/


/*********** Define statements ***********/

#define TEST_UNIT_TESTS_HEADER  "\r\n**********\r\nUNIT TESTS\r\n**********\r\n"        
#define TEST_USAGE_DEMOS_HEADER "\r\n***********\r\nUSAGE DEMOS\r\n***********\r\n"

/*****************************************/

/********** Function definitions *********/

int main()
{
    // Unit tests
    if(CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();
    
    CreateReturnValueTestsSuite();
    CreateErrorCodeTestsSuite();
    
    printf("%s", TEST_UNIT_TESTS_HEADER);
    CU_basic_run_tests();
    CU_cleanup_registry();

    // Usage demos
    printf("%s", TEST_USAGE_DEMOS_HEADER);
    TestDemo();

    return 0;
}

/*****************************************/
