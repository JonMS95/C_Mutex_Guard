// Compile with: gcc -g -Wall src/*.c -lpthread -o exe/main (use -D__DBG_MTX_FL__ too if required).

/********** Include statements ***********/

#include "TestDbgMutexes.h"

/*****************************************/

int main()
{
    TestDbgMutexes();

    return 0;
}