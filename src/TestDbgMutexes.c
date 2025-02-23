/********** Include statements ***********/

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#ifdef __DBG_MTX_FULL_BT__
#include <execinfo.h>
#include <stdbool.h>
#endif

/*****************************************/

/*********** Define statements ***********/

#ifndef __DBG_MTX_TOUT_SECS__
#define __DBG_MTX_TOUT_SECS__   1
#endif

#ifndef __DBG_MTX_ADDR_NUM__
#define __DBG_MTX_ADDR_NUM__    10
#endif

#ifdef  __DBG_MTX_FULL_BT__
#ifndef __DBG_MTX_FULL_BT_MAX_SIZE__
#define __DBG_MTX_FULL_BT_MAX_SIZE__    100
#endif
#endif

#define DBG_MTX_DEADLOCK_SLEEP_US   (uint32_t)1000000
#define DBG_MTX_TOUT_1_SEC_AS_NS    (uint64_t)1000000000
#define DBG_MTX_DEFAULT_TOUT        (uint64_t)(DBG_MTX_TOUT_1_SEC_AS_NS * __DBG_MTX_TOUT_SECS__)

#define DBG_MTX_MSG_ERR_THREAD_CREATION "Could not initialize thread <%d>.\r\n"

#define DBG_MTX_PROC_MAX_LEN                (uint16_t)256
#define DBG_MTX_PROC_MAPS_PATH              "/proc/self/maps"
#define DBG_MTX_MSG_ERR_OPEN_PROC_MAPS      "Failed to open /proc/self/maps"
#define DBG_MTX_CURRENT_PROC_PATH           "/proc/self/exe"

#define DBG_MTX_ADDR2LINE_CMD_FORMAT_LEN        (uint8_t)26
#define DBG_MTX_ADDR2LINE_CMD_FORMAT            "addr2line -f --exe=%s +%p"
#define DBG_MTX_ADDR2LINE_CMD_FORMAT_MAX_LEN    DBG_MTX_ADDR2LINE_CMD_FORMAT_LEN + PATH_MAX
#define DBG_MTX_MSG_ERR_ADDR2LINE               "Could not execute addr2line command."
#define DBG_MTX_FN_LINE_DELIMITER               ':'

#define DBG_MTX_ACQ_LOCATION_FULL_FORMAT        "#%u %p (+%p): %s defined at %s:%llu\r\n"

#define DBG_MTX_MSG_ERR_MUTEX_HEADER            "*********************************\r\n"
#define DBG_MTX_MSG_ERR_MUTEX_TIMEOUT           "Timeout elapsed (%lu s, %lu ns). "
#define DBG_MTX_MSG_ERR_MUTEX_ACQUISITION       "Thread with ID <0x%lx> cannot acquire mutex at <%p> (%s).\r\nLocked previously by thread with ID: <0x%lx> at the following address(es):\r\n"
#define DBG_MTX_MSG_ERR_MUTEX_FOOTER            "---------------------------------\r\n"

#define DBG_MTX_ACQ_FN_NAME_LEN 100

#define DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN  1024

#ifdef  __DBG_MTX_FULL_BT__
#define DBG_MTX_BT_ID_LEN           100
#define DBG_MTX_BT_ID_LOCK_STR      "LOCK BT"
#define DBG_MTX_BT_ID_UNLOCK_STR    "UNLOCK BT"
#define DBG_MTX_BT_ID_FORMAT        "|TID: <0x%lx>, MUTEX ADDR: <%p>, %s| "

#define DBG_MTX_BT_HEADER   "BT START"

#define DBG_MTX_BT_REL_ADDR_BEGIN_STR   "(+0x"
#define DBG_MTX_BT_REL_ADDR_END_STR     ") ["
#define DBG_MTX_BT_REL_ADDR_HEX_BASE    16

#define DBG_MTX_BT_FRAME_INFO   "#%llu %s"

#define DBG_MTX_BT_NOT_FOUND_SYMBOL         "??"
#define DBG_MTX_BT_FRAME_TO_FILE_FORMAT     " -> %s"
#define DBG_MTX_BT_FRAME_TO_LINE_FORMAT     ":%llu"
#define DBG_MTX_BT_FRAME_TO_FUNCTION_FORMAT " (%s)"

#define DBG_MTX_BT_FOOTER   "BT END" 
#endif

/**************** Macros *****************/

#define DBG_MTX_CREATE(var_name)                DBG_MTX var_name = {0}

#define DBG_MTX_ATTR_INIT(p_dbg_mtx, mutex_type, priority, proc_sharing)\
(DbgMutexAttrInit(p_dbg_mtx, mutex_type, priority, proc_sharing))
#define DBG_MTX_INIT(p_dbg_mtx)                 DbgMutexInit(p_dbg_mtx)

#define DBG_MTX_ATTR_INIT_FN(p_dbg_mtx, mutex_type, priority, proc_sharing, cleanup_var_name)\
DBG_MTX* cleanup_var_name __attribute__((cleanup(DbgDestroyMutexAttrCleanup))) = \
(DbgMutexAttrInitAddr(p_dbg_mtx, mutex_type, priority, proc_sharing))
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

typedef struct
{
    void*           addresses[__DBG_MTX_ADDR_NUM__];
    pthread_t       thread_id;
} DBG_MTX_ACQ_LOCATION;

typedef struct
{
    pthread_mutex_t         mutex;
    pthread_mutexattr_t     mutex_attr;
    DBG_MTX_ACQ_LOCATION    mutex_acq_location;
} DBG_MTX;

typedef struct
{
    DBG_MTX* p_dbg_mtx_first;
    DBG_MTX* p_dbg_mtx_second;
} DBG_MTX_TEST_PAIR;

typedef struct
{
    pthread_mutex_t* p_mtx_first;
    pthread_mutex_t* p_mtx_second;
} COMMON_MTX_TEST_PAIR;

typedef struct
{
    void*               relative_address;
    char                function_name[DBG_MTX_ACQ_FN_NAME_LEN + 1];
    unsigned long long  line;
    char                file_path[PATH_MAX + 1];
} DBG_MTX_ACQ_LOCATION_DETAIL;

typedef struct timespec mtx_to_t;

/*****************************************/

/****** Private function prototypes ******/

static          void        DbgMutexInitModule(void) __attribute__((constructor));
static          void        DbgMutexEndModule(void) __attribute__((destructor));
static          int         DbgMutexAttrInit(   DBG_MTX* restrict p_debug_mutex ,
                                                const int mutex_type            ,
                                                const int priority              ,
                                                const int proc_sharing          );
static          DBG_MTX*    DbgMutexAttrInitAddr(   DBG_MTX* restrict p_debug_mutex ,
                                                    const int mutex_type            , 
                                                    const int priority              ,
                                                    const int proc_sharing          );
static inline   int         DbgMutexInit(DBG_MTX* restrict p_debug_mutex);
static inline   DBG_MTX*    DbgMutexInitAddr(DBG_MTX* restrict p_debug_mutex);
static          int         DbgMutexStoreNewAddress(DBG_MTX* restrict p_debug_mutex, void* address);
static          int         DbgMutexRemoveLatestAddress(DBG_MTX* restrict p_debug_mutex);
static          mtx_to_t    DbgMutexGenTimespec(const uint64_t timeout_ns);
static          void        DbgMutexPrintLockTimeout(   const DBG_MTX_ACQ_LOCATION* restrict p_debug_mutex_acq_location ,
                                                        const pthread_mutex_t* restrict mutex_address                   ,
                                                        const uint64_t timeout_ns                                       ,
                                                        const int try_lock                                              ,
                                                        char* lock_error_string                                         );
static          void        DbgMutexPrintLockAddresses(const DBG_MTX_ACQ_LOCATION* p_debug_mutex_acq_location, char* lock_error_string);
static          void        DbgMutexPrintLockError( const DBG_MTX_ACQ_LOCATION* restrict p_debug_mutex_acq_location ,
                                                    const pthread_mutex_t* restrict target_mutex_addr               ,
                                                    const uint64_t timeout_ns                                       ,
                                                    const int try_lock                                              );
static          int         DbgMutexLock(DBG_MTX* p_debug_mutex, void* restrict address, const uint64_t timeout_ns);
static inline   DBG_MTX*    DbgMutexLockAddr(DBG_MTX* restrict p_debug_mutex, void* restrict address, const uint64_t timeout_ns);
static          size_t      GetExecutableBaseddress(void);
static          int         DbgMutexGetLockDetailFromAddr(const void* restrict addr, DBG_MTX_ACQ_LOCATION_DETAIL* restrict detail) __attribute__((unused)) ;
static          int         PrintFileAndLineFromAddr(   const void* restrict addr                   ,
                                                        char* output_buffer                         ,
                                                        const unsigned int address_index            ,
                                                        DBG_MTX_ACQ_LOCATION_DETAIL* restrict detail)
                                                        __attribute__((unused));
static          void*       TestDbgGetFuncRetAddr(void) __attribute__((noinline)) ;
static          int         DbgMutexUnlock(DBG_MTX* restrict p_dbg_mtx);
static          void        DbgReleaseMutexCleanup(void* ptr);
static inline   int         DbgMutexAttrDestroy(DBG_MTX* restrict p_dbg_mtx) __attribute__((unused));
static          void        DbgDestroyMutexAttrCleanup(void* ptr) __attribute__((unused));
static inline   int         DbgMutexDestroy(DBG_MTX* restrict p_dbg_mtx);
static          void        DbgDestroyMutexCleanup(void* ptr) __attribute__((unused));
static          void*       TestDbgThreadRoutine(void* arg);

#ifdef __DBG_MTX_FULL_BT__
static void DbgMutexShowBacktrace(const pthread_mutex_t* restrict p_locked_mutex, const bool is_lock);
#endif

/*****************************************/

/*********** Private variables ***********/

static DBG_MTX acq_info_lock;

/*****************************************/

/********** Function definitions *********/

static void __attribute__((constructor)) DbgMutexInitModule(void)
{
    DBG_MTX_ATTR_INIT_FN(   &acq_info_lock          ,
                            PTHREAD_MUTEX_ERRORCHECK,
                            PTHREAD_PRIO_INHERIT    ,
                            PTHREAD_PROCESS_PRIVATE ,
                            p_acq_info_lock_attr    );

    DBG_MTX_INIT(&acq_info_lock);
}

static void __attribute__((destructor)) DbgMutexEndModule(void)
{
    DBG_MTX_DESTROY(&acq_info_lock);
}

static int DbgMutexAttrInit(DBG_MTX* restrict p_debug_mutex ,
                            const int mutex_type            ,
                            const int priority              ,
                            const int proc_sharing          )
{
    if(!p_debug_mutex)
        return -1;
    
    int set_type        = pthread_mutexattr_settype(&p_debug_mutex->mutex_attr, mutex_type);
    int set_priority    = pthread_mutexattr_setprotocol(&p_debug_mutex->mutex_attr, priority);
    int set_proc_share  = pthread_mutexattr_setpshared(&p_debug_mutex->mutex_attr, proc_sharing);

    if( (set_type < 0) || (set_priority < 0) || (set_proc_share < 0))
        return -2;

    return 0;
}

static inline DBG_MTX* DbgMutexAttrInitAddr(DBG_MTX* restrict p_debug_mutex ,
                                            const int mutex_type            ,
                                            const int priority              ,
                                            const int proc_sharing          )
{
    return (DbgMutexAttrInit(p_debug_mutex, mutex_type, priority, proc_sharing) ? NULL : p_debug_mutex);
}

static inline int DbgMutexInit(DBG_MTX* restrict p_debug_mutex)
{
    return (p_debug_mutex ? pthread_mutex_init(&p_debug_mutex->mutex, &p_debug_mutex->mutex_attr) : -1);
}

static inline DBG_MTX* DbgMutexInitAddr(DBG_MTX* restrict p_debug_mutex)
{
    return (DbgMutexInit(p_debug_mutex) ? NULL : p_debug_mutex);
}

static int DbgMutexStoreNewAddress(DBG_MTX* restrict p_debug_mutex, void* address)
{
    for(int address_index = 0; address_index < __DBG_MTX_ADDR_NUM__; address_index++)
    {
        if(!p_debug_mutex->mutex_acq_location.addresses[address_index])
            p_debug_mutex->mutex_acq_location.addresses[address_index] = address;
        return 0;
    }
    
    return -1;
}

static int DbgMutexRemoveLatestAddress(DBG_MTX* restrict p_debug_mutex)
{
    for(int address_index = (__DBG_MTX_ADDR_NUM__ - 1); address_index >= 0; address_index--)
    {
        if( p_debug_mutex->mutex_acq_location.addresses[address_index])
            p_debug_mutex->mutex_acq_location.addresses[address_index] = NULL;
        return 0;
    }
    
    return -1;
}

static mtx_to_t DbgMutexGenTimespec(const uint64_t timeout_ns)
{
    mtx_to_t lock_timeout;
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

    return lock_timeout;
}

static void DbgMutexPrintLockTimeout(   const DBG_MTX_ACQ_LOCATION* restrict p_debug_mutex_acq_location  ,
                                        const pthread_mutex_t* restrict mutex_address           ,
                                        const uint64_t timeout_ns                               ,
                                        const int try_lock                                      ,
                                        char* lock_error_string                                 )
{
    if(timeout_ns > 0)
        snprintf(   lock_error_string + strlen(lock_error_string)                       ,
                    (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                    DBG_MTX_MSG_ERR_MUTEX_TIMEOUT                                       ,
                    timeout_ns / DBG_MTX_TOUT_1_SEC_AS_NS                               ,
                    timeout_ns % DBG_MTX_TOUT_1_SEC_AS_NS                               );

    snprintf(   lock_error_string + strlen(lock_error_string)                       ,
                (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                DBG_MTX_MSG_ERR_MUTEX_ACQUISITION                                   ,
                pthread_self()                                                      ,
                mutex_address                                                       ,
                strerror(try_lock)                                                  ,
                p_debug_mutex_acq_location->thread_id                               );
}

static void DbgMutexPrintLockAddresses(const DBG_MTX_ACQ_LOCATION* p_debug_mutex_acq_location, char* lock_error_string)
{
    DBG_MTX_ACQ_LOCATION_DETAIL detail = {0};

    for(unsigned int adress_index = 0; adress_index < __DBG_MTX_ADDR_NUM__; adress_index++)
    {
        if(!p_debug_mutex_acq_location->addresses[adress_index])
            return;
        
        PrintFileAndLineFromAddr(   p_debug_mutex_acq_location->addresses[adress_index] ,
                                    lock_error_string                                   ,
                                    adress_index                                        ,
                                    &detail                                             );
        
        memset(detail.function_name, 0, strlen(detail.function_name));
        memset(detail.file_path, 0, strlen(detail.file_path));
        detail.line = 0;
        detail.relative_address = NULL;
    }
}

static void DbgMutexPrintLockError( const DBG_MTX_ACQ_LOCATION* restrict p_debug_mutex_acq_location ,
                                    const pthread_mutex_t* restrict target_mutex_addr               ,
                                    const uint64_t timeout_ns                                       ,
                                    const int try_lock                                              )
{
    char lock_error_string[DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN] = {0};

    strncat((lock_error_string + strlen(lock_error_string))                     ,
            DBG_MTX_MSG_ERR_MUTEX_HEADER                                        ,
            (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    DbgMutexPrintLockTimeout(p_debug_mutex_acq_location, target_mutex_addr, timeout_ns, try_lock, lock_error_string + strlen(lock_error_string));
    DbgMutexPrintLockAddresses(p_debug_mutex_acq_location, lock_error_string + strlen(lock_error_string));

    strncat((lock_error_string + strlen(lock_error_string))                     ,
            DBG_MTX_MSG_ERR_MUTEX_FOOTER                                        ,
            (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    printf("%s", lock_error_string);
}

#ifdef __DBG_MTX_FULL_BT__
static void DbgMutexShowBacktrace(const pthread_mutex_t* restrict p_locked_mutex, const bool is_lock)
{
    void* call_stack[__DBG_MTX_FULL_BT_MAX_SIZE__ + 1] = {0};
    int call_stack_size = backtrace(call_stack, __DBG_MTX_FULL_BT_MAX_SIZE__ + 1);
    char** call_stack_symbols = backtrace_symbols(call_stack, call_stack_size);

    if(call_stack_symbols && call_stack_symbols[1])
    {
        char bt_id_str[DBG_MTX_BT_ID_LEN] = {0};

        snprintf(   bt_id_str                                                   ,
                    DBG_MTX_BT_ID_LEN                                           ,
                    DBG_MTX_BT_ID_FORMAT                                        ,
                    pthread_self()                                              ,
                    p_locked_mutex                                              ,
                    is_lock ? DBG_MTX_BT_ID_LOCK_STR : DBG_MTX_BT_ID_UNLOCK_STR );

        printf("%s%s\r\n", bt_id_str, DBG_MTX_BT_HEADER);

        unsigned bt_id_str_len = strlen(bt_id_str);

        char lock_error_string[DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN] = {0};
        strncpy(lock_error_string, bt_id_str, DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string));
        DBG_MTX_ACQ_LOCATION_DETAIL detail = {0};

        for(unsigned long long call_stack_index = 1; call_stack_index < call_stack_size; call_stack_index++)
        {
            snprintf((lock_error_string + strlen(lock_error_string))                    ,
                    (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                    DBG_MTX_BT_FRAME_INFO                                               ,
                    (call_stack_index - 1)                                              ,
                    call_stack_symbols[call_stack_index]                                );

            char* rel_addr_begin = strstr(call_stack_symbols[call_stack_index], DBG_MTX_BT_REL_ADDR_BEGIN_STR);
            char* rel_addr_end = strstr(call_stack_symbols[call_stack_index], DBG_MTX_BT_REL_ADDR_END_STR);
            
            if(!rel_addr_begin || !rel_addr_end)
            {
                printf("\r\n");
                continue;
            }

            char* end_ptr;

            *rel_addr_end = 0;
            unsigned long long rel_addr = strtoull( rel_addr_begin + strlen(DBG_MTX_BT_REL_ADDR_BEGIN_STR),
                                                    &end_ptr,
                                                    DBG_MTX_BT_REL_ADDR_HEX_BASE);
            
            if(*end_ptr)
            {
                printf("\r\n");
                continue;
            }

            detail.relative_address = (void*)rel_addr;
            
            int __attribute__((unused)) ret = DbgMutexGetLockDetailFromAddr(NULL, &detail);

            if(strcmp(detail.file_path, DBG_MTX_BT_NOT_FOUND_SYMBOL))
                snprintf(   (lock_error_string + strlen(lock_error_string))                     ,
                            (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                            DBG_MTX_BT_FRAME_TO_FILE_FORMAT                                     ,
                            detail.file_path                                                    );

            if(detail.line)
                snprintf(   (lock_error_string + strlen(lock_error_string))                     ,
                            (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                             DBG_MTX_BT_FRAME_TO_LINE_FORMAT                                    ,
                            detail.line                                                         );
            
            if(strcmp(detail.function_name, DBG_MTX_BT_NOT_FOUND_SYMBOL))
                snprintf(   (lock_error_string + strlen(lock_error_string))                     ,
                            (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                            DBG_MTX_BT_FRAME_TO_FUNCTION_FORMAT                                       ,
                            detail.function_name                                                );

            printf("%s\r\n", lock_error_string);
            memset(lock_error_string + bt_id_str_len, 0, strlen(lock_error_string + bt_id_str_len));
        }

        printf("%s%s\r\n", bt_id_str, DBG_MTX_BT_FOOTER);
    }
}
#endif

static int DbgMutexLock(DBG_MTX* p_debug_mutex, void* restrict address, const uint64_t timeout_ns)
{
    if(!p_debug_mutex)
        return -1;

    if(p_debug_mutex == &acq_info_lock)
        return -2;

    // The mutex below is used for other threads not to coincidentally modify the content within the provided DBG_MTX variable. 
    // pthread_mutex_t* p_acq_info_lock __attribute__((cleanup(DbgMutexAcqInfoLockCleanup))) = &acq_info_lock;
    DBG_MTX_ACQ_LOCATION target_mutex_acq_location;

    int try_lock;

    DBG_MTX_LOCK_FN(&acq_info_lock, p_acq_info_lock);
    memcpy(&target_mutex_acq_location, &p_debug_mutex->mutex_acq_location, sizeof(DBG_MTX_ACQ_LOCATION));
    DBG_MTX_UNLOCK(&acq_info_lock);

    if(timeout_ns == 0)
    {
        try_lock = pthread_mutex_trylock(&p_debug_mutex->mutex);
    }
    else
    {
        const mtx_to_t timed_lock_timeout = DbgMutexGenTimespec(timeout_ns);
        try_lock = pthread_mutex_timedlock(&p_debug_mutex->mutex, &timed_lock_timeout);
    }

    if(try_lock)
    {
        DBG_MTX_ACQ_LOCATION info_acq_location  = p_debug_mutex->mutex_acq_location;
        pthread_mutex_t* target_mutex_addr      = &p_debug_mutex->mutex;
        
        // DbgMutexPrintLockError(&info_acq_location, target_mutex_addr, timeout_ns, try_lock);

        return try_lock;
    }

    int __attribute__((unused)) store_addr = DbgMutexStoreNewAddress(p_debug_mutex, address);

    p_debug_mutex->mutex_acq_location.thread_id = pthread_self();
    
    #ifdef __DBG_MTX_FULL_BT__ 
    DbgMutexShowBacktrace(&p_debug_mutex->mutex, true);
    #endif

    return try_lock;
}

static inline DBG_MTX* DbgMutexLockAddr(DBG_MTX* restrict p_debug_mutex, void* restrict address, const uint64_t timeout_ns)
{
    return (DbgMutexLock(p_debug_mutex, address, timeout_ns) ? NULL : p_debug_mutex);
}

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

static int __attribute__((unused)) DbgMutexGetLockDetailFromAddr(const void* restrict addr, DBG_MTX_ACQ_LOCATION_DETAIL* restrict detail)
{
    char cmd[DBG_MTX_ADDR2LINE_CMD_FORMAT_MAX_LEN + 1] = {0};
    char full_path[PATH_MAX + 1] = {0};
    
    if(readlink(DBG_MTX_CURRENT_PROC_PATH, full_path, sizeof(full_path) - 1) < 0)
        return -1;
    
    if(!detail->relative_address)
        detail->relative_address = (void*)((size_t)addr - GetExecutableBaseddress());

    snprintf(   cmd                                 ,
                DBG_MTX_ADDR2LINE_CMD_FORMAT_MAX_LEN,
                DBG_MTX_ADDR2LINE_CMD_FORMAT        ,
                full_path                           ,
                detail->relative_address            );

    FILE *fp = popen(cmd, "r");
    
    if (fp == NULL)
    {
        perror(DBG_MTX_MSG_ERR_ADDR2LINE);
        return -1;
    }

    char line[PATH_MAX + 1] = {0};
    
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = 0;

        char* at_pos = strrchr(line, DBG_MTX_FN_LINE_DELIMITER);
        if(at_pos)
        {
            char* acq_line_start = at_pos + sizeof(char);
            
            if(acq_line_start && acq_line_start[0] >= '0' && acq_line_start[0] <= '9')
                detail->line = atol(acq_line_start);
            else
                detail->line = 0;
            
            memset(at_pos, 0, strlen(at_pos));
            strncpy(detail->file_path, line, PATH_MAX);
        }
        else
        {
            strncpy(detail->function_name, line, DBG_MTX_ACQ_FN_NAME_LEN);
        }
    }

    pclose(fp);

    return 0;
}

static int __attribute__((unused)) PrintFileAndLineFromAddr(const void* restrict addr, char* output_buffer, const unsigned int address_index, DBG_MTX_ACQ_LOCATION_DETAIL* restrict detail)
{
    int ret = DbgMutexGetLockDetailFromAddr(addr, detail);
    
    snprintf(   output_buffer + strlen(output_buffer)                           ,
                (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(output_buffer)),
                DBG_MTX_ACQ_LOCATION_FULL_FORMAT                                ,
                address_index                                                   ,
                addr                                                            ,
                detail->relative_address                                        ,
                detail->function_name                                           ,
                detail->file_path                                               ,
                detail->line                                                    );
    
    if(detail->line == 0)
    {
        char* line_delimiter_pos = strrchr(output_buffer, DBG_MTX_FN_LINE_DELIMITER);
        if(line_delimiter_pos)
        {
            memset(line_delimiter_pos, 0, strlen(line_delimiter_pos));
            strncat(output_buffer, "\r\n", (DBG_MTX_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(output_buffer)));
        }
    }

    return ret;
}

static __attribute__((noinline)) void* TestDbgGetFuncRetAddr(void)
{
    return __builtin_return_address(0); 
}

static int DbgMutexUnlock(DBG_MTX* restrict p_dbg_mtx)
{
    if(!p_dbg_mtx)
        return -1;

    memset(&p_dbg_mtx->mutex_acq_location, 0, sizeof(DBG_MTX_ACQ_LOCATION));

    int ret = pthread_mutex_unlock(&p_dbg_mtx->mutex);
    DbgMutexRemoveLatestAddress(p_dbg_mtx);

    #ifdef __DBG_MTX_FULL_BT__ 
    if(p_dbg_mtx != &acq_info_lock && ret == 0)
        DbgMutexShowBacktrace(&p_dbg_mtx->mutex, false);
    #endif

    return ret;
}

static void DbgReleaseMutexCleanup(void* ptr)
{
    if(!ptr || !(*(DBG_MTX**)ptr))
        return;
    
    DbgMutexUnlock(*(DBG_MTX**)ptr);
    *(DBG_MTX**)ptr = NULL;
}

static inline __attribute__((unused)) int DbgMutexAttrDestroy(DBG_MTX* restrict p_dbg_mtx)
{
    return pthread_mutexattr_destroy(&p_dbg_mtx->mutex_attr);
}

static __attribute__((unused)) void DbgDestroyMutexAttrCleanup(void* ptr)
{
    if(!ptr || !(*(DBG_MTX**)ptr))
        return;

    DbgMutexAttrDestroy(*(DBG_MTX**)ptr);
}

static inline int DbgMutexDestroy(DBG_MTX* restrict p_dbg_mtx)
{
    return pthread_mutex_destroy(&p_dbg_mtx->mutex);
}

static __attribute__((unused)) void DbgDestroyMutexCleanup(void* ptr)
{
    if(!ptr)
        return;

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

    return NULL;
}

static void* TestCommonThreadRoutine(void* arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    COMMON_MTX_TEST_PAIR* p_mtx_test_pair = (COMMON_MTX_TEST_PAIR*)arg;

    int try_lock = pthread_mutex_trylock(p_mtx_test_pair->p_mtx_first);

    usleep(DBG_MTX_DEADLOCK_SLEEP_US); // Let the current thread take a nap so as to cause a deadlock.

    mtx_to_t lock_timeout = DbgMutexGenTimespec(DBG_MTX_DEFAULT_TOUT);

    int timed_lock = pthread_mutex_timedlock(p_mtx_test_pair->p_mtx_second, &lock_timeout);

    if(!timed_lock)
    {
        printf("TO failed\n");
        pthread_mutex_unlock(p_mtx_test_pair->p_mtx_second);
    }
    
    if(!try_lock)
        pthread_mutex_unlock(p_mtx_test_pair->p_mtx_first);

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
}

void TestCommonMutexes(void)
{
    pthread_t t_0, t_1;

    pthread_mutex_t common_mutex_0, common_mutex_1;

    pthread_mutex_init(&common_mutex_0, NULL);
    pthread_mutex_init(&common_mutex_1, NULL);

    COMMON_MTX_TEST_PAIR common_mutex_test_pair_0 =
    {
        .p_mtx_first    = &common_mutex_0,
        .p_mtx_second   = &common_mutex_1,
    };

    COMMON_MTX_TEST_PAIR common_mutex_test_pair_1 =
    {
        .p_mtx_first    = &common_mutex_1,
        .p_mtx_second   = &common_mutex_0,
    };

    if(pthread_create(&t_0, NULL, TestCommonThreadRoutine, &common_mutex_test_pair_0))
    {
        printf(DBG_MTX_MSG_ERR_THREAD_CREATION, 0);
        pthread_mutex_destroy(&common_mutex_0);
        pthread_mutex_destroy(&common_mutex_1);

        return;
    }

    if(pthread_create(&t_1, NULL, TestCommonThreadRoutine, &common_mutex_test_pair_1))
    {
        printf(DBG_MTX_MSG_ERR_THREAD_CREATION, 1);
        pthread_cancel(t_0);
        pthread_mutex_destroy(&common_mutex_0);
        pthread_mutex_destroy(&common_mutex_1);

        return;
    }

    pthread_join(t_0, NULL);
    pthread_join(t_1, NULL);

    pthread_mutex_destroy(&common_mutex_0);
    pthread_mutex_destroy(&common_mutex_1);
}

/*****************************************/