// Compile with: gcc -g -Wall src/*.c -lpthread -o exe/main (use -D__MTX_GRD_FULL_BT__ too if required).

/********** Include statements ***********/

#include "TestDbgMutexes.h"
#include <stdio.h>
#include <time.h>

/*****************************************/

#define TEST_FN_TIME(function)   TestFunctionElapsedTime(function, #function)
#define NUM_OF_TESTS    100

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