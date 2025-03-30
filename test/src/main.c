/********** Include statements ***********/

#include <CUnit/Basic.h>
#include "MutexGuard_api.h"
#include "TestReturnValues.h"
#include "TestErrorCodes.h"

/*****************************************/

/*************** Run Tests ***************/

int main()
{
    if(CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();
    
    CreateReturnValueTestsSuite();
    CreateErrorCodeTestsSuite();
    
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}