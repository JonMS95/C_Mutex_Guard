/********** Include statements ***********/

#include <CUnit/Basic.h>
#include "MutexGuard_api.h"
#include "TestReturnValues.h"

/*****************************************/

/*********** Test definitions ************/



int main()
{
    if(CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();
    
    GetReturnValueTestsSuite();
    
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}