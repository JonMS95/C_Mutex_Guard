/********** Include statements ***********/

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/*****************************************/

/*********** Define statements ***********/

#ifndef __DBG_MTX_TOUT_SECS__
#define __DBG_MTX_TOUT_SECS__   1
#endif

#define DBG_MTX_MAX_FILE_NAME_LEN   (uint8_t)100
#define DBG_MTX_DEADLOCK_SLEEP_US   (uint16_t)1000000
#define DBG_MTX_TOUT_1_SEC_AS_NS    (uint64_t)1000000000
#define DBG_MTX_DEFAULT_TOUT        (uint64_t)(DBG_MTX_TOUT_1_SEC_AS_NS * __DBG_MTX_TOUT_SECS__)

#define DBG_MTX_MSG_ERR_THREAD_CREATION "Could not initialize thread <%d>.\r\n"

#define DBG_MTX_MAX_FILE_AND_LINE_LEN       (uint8_t)200
#define DBG_MTX_PROC_MAX_LEN                (uint16_t)256
#define DBG_MTX_PROC_MAPS_PATH              "/proc/self/maps"
#define DBG_MTX_MSG_ERR_OPEN_PROC_MAPS      "Failed to open /proc/self/maps"
#define DBG_MTX_CURRENT_PROC_PATH           "/proc/self/exe"

#define DBG_MTX_ADDR2LINE_CMD_FORMAT_LEN        (uint8_t)26
#define DBG_MTX_ADDR2LINE_CMD_FORMAT            "addr2line -f --exe=%s +%p"
#define DBG_MTX_ADDR2LINE_CMD_FORMAT_MAX_LEN    DBG_MTX_ADDR2LINE_CMD_FORMAT_LEN + PATH_MAX
#define DBG_MTX_MSG_ERR_ADDR2LINE               "Could not execute addr2line command."
#define DBG_MTX_FN_PATH_LINE_DELIMITER          ":"
#define DBG_MTX_FN_PATH_HALF_DELIMITER          "%s defined at "

#define DBG_MTX_MSG_ERR_MUTEX_ACQUISITION       "%sThread with ID <0x%lx> cannot acquire mutex at <%p> (%s). Locked previously at: <%s (%p)> by thread with ID: <0x%lx>\r\n"

#define DBG_MTX_MSG_ERR_MUTEX_TIMEOUT           "Timeout elapsed (%lu s, %lu ns). "
#define DBG_MTX_MSG_ERR_MUTEX_TIMEOUT_STR_LEN   80

/**************** Macros *****************/

#define DBG_MTX_CREATE(var_name)                DBG_MTX var_name = {0}

#define DBG_MTX_ATTR_INIT(\
p_dbg_mtx, mutex_type, priority, proc_sharing)  DbgMutexAttrInit(p_dbg_mtx, mutex_type, priority, proc_sharing)
#define DBG_MTX_INIT(p_dbg_mtx)                 DbgMutexInit(p_dbg_mtx)

#define DBG_MTX_ATTR_INIT_FN(p_dbg_mtx, mutex_type, priority, proc_sharing, cleanup_var_name)\
DBG_MTX* cleanup_var_name __attribute__((cleanup(DbgDestroyMutexAttrCleanup))) = \
(DbgMutexAttrInit(p_dbg_mtx, mutex_type, priority, proc_sharing, cleanup_var_name))
#define DBG_MTX_INIT_FN(p_dbg_mtx, cleanup_var_name)\
DBG_MTX* cleanup_var_name __attribute__((cleanup(DbgDestroyMutexCleanup))) = \
(DbgMutexInitAddr(p_dbg_mtx))

#define DBG_MTX_LOCK(p_dbg_mtx)                 DbgMutexLock((p_dbg_mtx), TestDbgGetFuncRetAddr(), 0)
#define DBG_MTX_TIMED_LOCK(p_dbg_mtx, tout_ns)  DbgMutexLock((p_dbg_mtx), TestDbgGetFuncRetAddr(), tout_ns)

#define DBG_MTX_LOCK_FN(p_dbg_mtx, cleanup_var_name)\
DBG_MTX* cleanup_var_name __attribute__((cleanup(DbgReleaseMutexCleanup))) = \
(DbgMutexLockAddr(p_dbg_mtx, TestDbgGetFuncRetAddr(), 0))
#define DBG_MTX_TIMED_LOCK_FN(p_dbg_mtx, tout_ns, cleanup_var_name)\
DBG_MTX* cleanup_var_name __attribute__((cleanup(DbgReleaseMutexCleanup))) = \
(DbgMutexLockAddr(p_dbg_mtx, TestDbgGetFuncRetAddr(), tout_ns))

#define DBG_MTX_UNLOCK(p_dbg_mtx)               DbgMutexUnlock(p_dbg_mtx)
#define DBG_MTX_DESTROY(p_dbg_mtx)              DbgMutexDestroy(p_dbg_mtx)
#define DBG_MTX_ATTR_DESTROY(p_dbg_mtx)         DbgMutexAttrDestroy(p_dbg_mtx)

/*****************************************/

/******* Private type definitions ********/

typedef struct /* __attribute__((packed)) */
{
    void*           address;
    pthread_t       thread_id;
} DBG_MTX_ACQ_LOCATION;

typedef struct /* __attribute__((packed)) */
{
    pthread_mutex_t         mutex;
    pthread_mutexattr_t     mutex_attr;
    DBG_MTX_ACQ_LOCATION    mutex_acq_location;
} DBG_MTX;

typedef struct /* __attribute__((packed)) */
{
    DBG_MTX* p_dbg_mtx_first;
    DBG_MTX* p_dbg_mtx_second;
} DBG_MTX_TEST_PAIR;

/*****************************************/

/****** Private function prototypes ******/

static void*    TestDbgThreadRoutine(void* arg);
static int      DbgMutexAttrInit(DBG_MTX* p_debug_mutex, int mutex_type, int priority, int proc_sharing);
static DBG_MTX* __attribute__((unused)) DbgMutexAttrInitAddr(DBG_MTX* p_debug_mutex, int mutex_type, int priority, int proc_sharing);
static int      DbgMutexInit(DBG_MTX* p_debug_mutex);
static DBG_MTX* DbgMutexInitAddr(DBG_MTX* p_debug_mutex);
static int      PrintFileAndLineFromAddr(void* addr, char* output_buffer, size_t buffer_size);
static size_t   GetExecutableBaseddress(void);
static void*    __attribute__((noinline)) TestDbgGetFuncRetAddr(void);
static int      DbgMutexLock(DBG_MTX* DBG_MTX, void* address, uint64_t timeout_ns);
static DBG_MTX* DbgMutexLockAddr(DBG_MTX* p_debug_mutex, void* address, uint64_t timeout_ns);
static int      DbgMutexUnlock(DBG_MTX* p_dbg_mtx);
static void     DbgReleaseMutexCleanup(void* ptr);
static int      __attribute__((unused)) DbgMutexAttrDestroy(DBG_MTX* p_dbg_mtx);
static void     __attribute__((unused)) DbgDestroyMutexAttrCleanup(void* ptr);
static int      DbgMutexDestroy(DBG_MTX* p_dbg_mtx);
static void     __attribute__((unused)) DbgDestroyMutexCleanup(void* ptr);

/*****************************************/

/********** Function definitions *********/

static int DbgMutexAttrInit(DBG_MTX* p_debug_mutex, int mutex_type, int priority, int proc_sharing)
{
    if(!p_debug_mutex)
        return -1;
    
    int set_type        = pthread_mutexattr_settype(&p_debug_mutex->mutex_attr, mutex_type);
    int set_priority    = pthread_mutexattr_setprotocol(&p_debug_mutex->mutex_attr, priority);
    int set_proc_share  = pthread_mutexattr_setpshared(&p_debug_mutex->mutex_attr, proc_sharing);

    return (set_type | set_priority | set_proc_share);
}

static __attribute__((unused)) DBG_MTX* DbgMutexAttrInitAddr(DBG_MTX* p_debug_mutex, int mutex_type, int priority, int proc_sharing)
{
    if(DbgMutexAttrInit(p_debug_mutex, mutex_type, priority, proc_sharing))
        return NULL;

    return p_debug_mutex;
}

static int DbgMutexInit(DBG_MTX* p_debug_mutex)
{
    if(!p_debug_mutex)
        return -1;
    
    return pthread_mutex_init(&p_debug_mutex->mutex, &p_debug_mutex->mutex_attr);
}

static DBG_MTX* DbgMutexInitAddr(DBG_MTX* p_debug_mutex)
{
    if(DbgMutexInit(p_debug_mutex))
        return NULL;

    return p_debug_mutex;
}

static int DbgMutexLock(DBG_MTX* p_debug_mutex, void* address, uint64_t timeout_ns)
{
    if(!p_debug_mutex)
        return -1;

    int try_lock;

    if(timeout_ns == 0)
    {
        try_lock = pthread_mutex_trylock(&p_debug_mutex->mutex);
    }
    else
    {
        struct timespec lock_timeout;
        clock_gettime(CLOCK_REALTIME, &lock_timeout);
        
        // Add the timeout (in nanoseconds) to the current time
        lock_timeout.tv_sec += timeout_ns / DBG_MTX_TOUT_1_SEC_AS_NS;
        lock_timeout.tv_nsec += timeout_ns % DBG_MTX_TOUT_1_SEC_AS_NS;
        
        // Handle possible overflow in nanoseconds (if it's >= 1 second)
        if (lock_timeout.tv_nsec >= DBG_MTX_TOUT_1_SEC_AS_NS)
        {
            lock_timeout.tv_nsec -= DBG_MTX_TOUT_1_SEC_AS_NS;
            lock_timeout.tv_sec += 1;
        }

        try_lock = pthread_mutex_timedlock(&p_debug_mutex->mutex, &lock_timeout);
    }

    if(try_lock)
    {
        #ifndef __DBG_MTX_FL__
        char file_and_line[DBG_MTX_MAX_FILE_AND_LINE_LEN + 1] = "";

        PrintFileAndLineFromAddr(   p_debug_mutex->mutex_acq_location.address  ,
                                    file_and_line                           ,
                                    DBG_MTX_MAX_FILE_AND_LINE_LEN         );
        #endif
        
        char timeout_elapsed_str[DBG_MTX_MSG_ERR_MUTEX_TIMEOUT_STR_LEN + 1] = "";
        
        if(timeout_ns)
            snprintf(   timeout_elapsed_str                     ,
                        DBG_MTX_MSG_ERR_MUTEX_TIMEOUT_STR_LEN   ,
                        DBG_MTX_MSG_ERR_MUTEX_TIMEOUT           ,
                        timeout_ns / DBG_MTX_TOUT_1_SEC_AS_NS   ,
                        timeout_ns % DBG_MTX_TOUT_1_SEC_AS_NS   );

        printf( DBG_MTX_MSG_ERR_MUTEX_ACQUISITION           ,
                timeout_elapsed_str                         ,
                pthread_self()                              ,
                &p_debug_mutex->mutex                       ,
                strerror(try_lock)                          ,
                file_and_line                               ,
                p_debug_mutex->mutex_acq_location.address   ,
                p_debug_mutex->mutex_acq_location.thread_id);

        return try_lock;
    }

    p_debug_mutex->mutex_acq_location.address      = address;
    p_debug_mutex->mutex_acq_location.thread_id    = pthread_self();
    
    return try_lock;
}

static DBG_MTX* DbgMutexLockAddr(DBG_MTX* p_debug_mutex, void* address, uint64_t timeout_ns)
{
    if(DbgMutexLock(p_debug_mutex, address, timeout_ns))
        return NULL;

    printf("TID: 0x%lx Locked mutex at: %p\r\n", pthread_self(), &p_debug_mutex->mutex);

    return p_debug_mutex;
}

#ifndef __DBG_MTX_FL__
static size_t GetExecutableBaseddress(void)
{
    FILE *maps = fopen(DBG_MTX_PROC_MAPS_PATH, "r");
    
    if (!maps)
    {
        perror(DBG_MTX_MSG_ERR_OPEN_PROC_MAPS);
        return 0;
    }

    char line[DBG_MTX_PROC_MAX_LEN];
    size_t start = 0;

    while (fgets(line, sizeof(line), maps))
    {
        if (strstr(line, "/"))
        {
            sscanf(line, "%lx", &start);
            break;
        }
    }

    fclose(maps);

    return start;
}

static int PrintFileAndLineFromAddr(void* addr, char* output_buffer, size_t buffer_size)
{
    char cmd[DBG_MTX_ADDR2LINE_CMD_FORMAT_MAX_LEN + 1] = "";
    char full_path[PATH_MAX + 1] = "";
    
    if(readlink(DBG_MTX_CURRENT_PROC_PATH, full_path, sizeof(full_path) - 1) < 0)
        return -1;

    snprintf(   cmd                                                 ,
                DBG_MTX_ADDR2LINE_CMD_FORMAT_MAX_LEN                ,
                DBG_MTX_ADDR2LINE_CMD_FORMAT                        ,
                full_path                                           ,
                (void*)((size_t)addr - GetExecutableBaseddress())   );

    FILE *fp = popen(cmd, "r");
    
    if (fp == NULL)
    {
        perror(DBG_MTX_MSG_ERR_ADDR2LINE);
        return -1;
    }

    char line[PATH_MAX + 1] = "";
    output_buffer[0] = '\0'; // Clear the buffer initially
    
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = 0;
        
        char* at_pos = strstr(line, DBG_MTX_FN_PATH_LINE_DELIMITER);
        if(!at_pos)
            snprintf(output_buffer, buffer_size, DBG_MTX_FN_PATH_HALF_DELIMITER, line);
        else
            strncat(output_buffer, line, buffer_size - strlen(output_buffer) - 1);
    }

    pclose(fp);
    return 0;
}

static __attribute__((noinline)) void* TestDbgGetFuncRetAddr(void)
{
    return __builtin_return_address(0); 
}
#endif

static int DbgMutexUnlock(DBG_MTX* p_dbg_mtx)
{
    if(!p_dbg_mtx)
        return -1;

    memset(&p_dbg_mtx->mutex_acq_location, 0, sizeof(DBG_MTX_ACQ_LOCATION));
    
    printf("TID: 0x%lx released mutex at %p\r\n", pthread_self(), &p_dbg_mtx->mutex);

    return pthread_mutex_unlock(&p_dbg_mtx->mutex);
}

static void DbgReleaseMutexCleanup(void* ptr)
{
    if(!ptr)
        return;
    
    DbgMutexUnlock(*(DBG_MTX**)ptr);
}

static __attribute__((unused)) int DbgMutexAttrDestroy(DBG_MTX* p_dbg_mtx)
{
    return pthread_mutexattr_destroy(&p_dbg_mtx->mutex_attr);
}

static __attribute__((unused)) void DbgDestroyMutexAttrCleanup(void* ptr)
{
    if(!ptr)
        return;

    printf("Destroying mutex attr at: 0x%p\r\n", &(*(DBG_MTX**)ptr)->mutex_attr);

    DbgMutexAttrDestroy(*(DBG_MTX**)ptr);
}

static int DbgMutexDestroy(DBG_MTX* p_dbg_mtx)
{
    return pthread_mutex_destroy(&p_dbg_mtx->mutex);
}

static __attribute__((unused)) void DbgDestroyMutexCleanup(void* ptr)
{
    if(!ptr)
        return;
    
    printf("Destroying mutex at: 0x%p\r\n", &(*(DBG_MTX**)ptr)->mutex);

    DbgMutexDestroy(*(DBG_MTX**)ptr);
}

static void* TestDbgThreadRoutine(void* arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    DBG_MTX_TEST_PAIR* p_dbg_mtx_test_pair = (DBG_MTX_TEST_PAIR*)arg;

    DBG_MTX_LOCK_FN(p_dbg_mtx_test_pair->p_dbg_mtx_first, cleanup_var_first);

    usleep(DBG_MTX_DEADLOCK_SLEEP_US); // Let the current thread take a nap so as to cause a deadlock.

    DBG_MTX_TIMED_LOCK_FN(p_dbg_mtx_test_pair->p_dbg_mtx_second, DBG_MTX_DEFAULT_TOUT, cleanup_var_second);

    usleep(DBG_MTX_DEADLOCK_SLEEP_US);

    return NULL;
}

void TestDbgMutexes(void)
{
    pthread_t t_0, t_1;

    DBG_MTX_CREATE(debug_mutex_0);
    DBG_MTX_CREATE(debug_mutex_1);

    DBG_MTX_INIT_FN(&debug_mutex_0, cleanup_var_0);
    DBG_MTX_INIT_FN(&debug_mutex_1, cleanup_var_1);

    DBG_MTX_TEST_PAIR debug_mutex_test_pair_0 =
    {
        .p_dbg_mtx_first    = &debug_mutex_0,
        .p_dbg_mtx_second   = &debug_mutex_1,
    };

    DBG_MTX_TEST_PAIR debug_mutex_test_pair_1 =
    {
        .p_dbg_mtx_first    = &debug_mutex_1,
        .p_dbg_mtx_second   = &debug_mutex_0,
    };

    if(pthread_create(&t_0, NULL, TestDbgThreadRoutine, &debug_mutex_test_pair_0))
    {
        printf(DBG_MTX_MSG_ERR_THREAD_CREATION, 0);

        return;
    }

    if(pthread_create(&t_1, NULL, TestDbgThreadRoutine, &debug_mutex_test_pair_1))
    {
        printf(DBG_MTX_MSG_ERR_THREAD_CREATION, 1);
        pthread_cancel(t_0);

        return;
    }

    pthread_join(t_0, NULL);
    pthread_join(t_1, NULL);

    printf("Ending test ...\r\n");
}

/*****************************************/