/********** Include statements ***********/

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <execinfo.h>
#include <stdbool.h>

/*****************************************/

/*********** Define statements ***********/

#ifndef __MTX_GRD_TOUT_SECS__
#define __MTX_GRD_TOUT_SECS__   1
#endif

#ifndef __MTX_GRD_ADDR_NUM__
#define __MTX_GRD_ADDR_NUM__    10
#endif

#ifndef __MTX_GRD_FULL_BT_MAX_SIZE__
#define __MTX_GRD_FULL_BT_MAX_SIZE__    100
#endif

#define MTX_GRD_DEADLOCK_SLEEP_US   (uint32_t)1000000
#define MTX_GRD_TOUT_1_SEC_AS_NS    (uint64_t)1000000000
#define MTX_GRD_DEFAULT_TOUT        (uint64_t)(MTX_GRD_TOUT_1_SEC_AS_NS * __MTX_GRD_TOUT_SECS__)

#define MTX_GRD_MSG_ERR_THREAD_CREATION "Could not initialize thread <%d>.\r\n"

#define MTX_GRD_PROC_MAX_LEN                (uint16_t)256
#define MTX_GRD_PROC_MAPS_PATH              "/proc/self/maps"
#define MTX_GRD_MSG_ERR_OPEN_PROC_MAPS      "Failed to open /proc/self/maps"
#define MTX_GRD_CURRENT_PROC_PATH           "/proc/self/exe"

#define MTX_GRD_ADDR2LINE_CMD_FORMAT_LEN        (uint8_t)26
#define MTX_GRD_ADDR2LINE_CMD_FORMAT            "addr2line -f --exe=%s +%p"
#define MTX_GRD_ADDR2LINE_CMD_FORMAT_MAX_LEN    MTX_GRD_ADDR2LINE_CMD_FORMAT_LEN + PATH_MAX
#define MTX_GRD_MSG_ERR_ADDR2LINE               "Could not execute addr2line command."
#define MTX_GRD_FN_LINE_DELIMITER               ':'

#define MTX_GRD_ACQ_LOCATION_FULL_FORMAT        "#%u %p (+%p): %s defined at %s:%llu\r\n"

#define MTX_GRD_MSG_ERR_MUTEX_HEADER            "*********************************\r\n"
#define MTX_GRD_MSG_ERR_MUTEX_TIMEOUT           "Timeout elapsed (%lu s, %lu ns). "
#define MTX_GRD_MSG_ERR_MUTEX_ACQ               "Thread with ID <0x%lx> cannot acquire mutex at <%p> (%s).\r\n"
#define MTX_GRD_MSG_ERR_MUTEX_ACQ_ADDR_HEADER   "Locked previously by thread with ID: <0x%lx> at the following address(es):\r\n"
#define MTX_GRD_MSG_ERR_MUTEX_FOOTER            "---------------------------------\r\n"

#define MTX_GRD_ACQ_FN_NAME_LEN 100

#define MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN  1024

#define MTX_GRD_BT_ID_LEN           100
#define MTX_GRD_BT_ID_LOCK_STR      "LOCK BT"
#define MTX_GRD_BT_ID_UNLOCK_STR    "UNLOCK BT"
#define MTX_GRD_BT_ID_FORMAT        "|TID: <0x%lx>, MUTEX ADDR: <%p>, %s| "

#define MTX_GRD_BT_HEADER   "BT START"

#define MTX_GRD_BT_REL_ADDR_BEGIN_STR   "(+0x"
#define MTX_GRD_BT_REL_ADDR_END_STR     ") ["
#define MTX_GRD_BT_REL_ADDR_HEX_BASE    16

#define MTX_GRD_BT_FRAME_INFO   "#%llu %s"

#define MTX_GRD_BT_NOT_FOUND_SYMBOL         "??"
#define MTX_GRD_BT_FRAME_TO_FILE_FORMAT     " -> %s"
#define MTX_GRD_BT_FRAME_TO_LINE_FORMAT     ":%llu"
#define MTX_GRD_BT_FRAME_TO_FUNCTION_FORMAT " (%s)"

#define MTX_GRD_BT_FOOTER   "BT END" 

/**************** Macros *****************/

#define MTX_GRD_CREATE(var_name)    MTX_GRD var_name = {0}

#define MTX_GRD_ATTR_INIT(p_mtx_grd, mutex_type, priority, proc_sharing)\
(MutexGuardAttrInit(p_mtx_grd, mutex_type, priority, proc_sharing))

#define MTX_GRD_INIT(p_mtx_grd) MutexGuardInit(p_mtx_grd)

#define MTX_GRD_ATTR_INIT_SC(p_mtx_grd, mutex_type, priority, proc_sharing, cleanup_var_name)\
MTX_GRD* cleanup_var_name __attribute__((cleanup(MutexGuardDestroyAttrCleanup))) = \
(MutexGuardAttrInitAddr(p_mtx_grd, mutex_type, priority, proc_sharing))

#define MTX_GRD_INIT_SC(p_mtx_grd, cleanup_var_name)\
MTX_GRD* cleanup_var_name __attribute__((cleanup(MutexGuardDestroyMutexCleanup))) = \
(MutexGuardInitAddr(p_mtx_grd))

#define MTX_GRD_TRY_LOCK(p_mtx_grd)                 MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_TRY)
#define MTX_GRD_LOCK(p_mtx_grd)                     MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_PERMANENT)
#define MTX_GRD_TIMED_LOCK(p_mtx_grd, tout_ns)      MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_TIMED)
#define MTX_GRD_PERIODIC_LOCK(p_mtx_grd, tout_ns)   MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_PERIODIC)

#define MTX_GRD_TRY_LOCK_SC(p_mtx_grd, cleanup_var_name)\
MTX_GRD* cleanup_var_name __attribute__((cleanup(MutexGuardReleaseMutexCleanup))) = \
(MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_TRY))

#define MTX_GRD_LOCK_SC(p_mtx_grd, cleanup_var_name)\
MTX_GRD* cleanup_var_name __attribute__((cleanup(MutexGuardReleaseMutexCleanup))) = \
(MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_PERMANENT))

#define MTX_GRD_TIMED_LOCK_SC(p_mtx_grd, tout_ns, cleanup_var_name)\
MTX_GRD* cleanup_var_name __attribute__((cleanup(MutexGuardReleaseMutexCleanup))) = \
(MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_TIMED))

#define MTX_GRD_PERIODIC_LOCK_SC(p_mtx_grd, tout_ns, cleanup_var_name)\
MTX_GRD* cleanup_var_name __attribute__((cleanup(MutexGuardReleaseMutexCleanup))) = \
(MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_PERIODIC))

#define MTX_GRD_UNLOCK(p_mtx_grd)       MutexGuardUnlock(p_mtx_grd)
#define MTX_GRD_DESTROY(p_mtx_grd)      MutexGuardDestroy(p_mtx_grd)
#define MTX_GRD_ATTR_DESTROY(p_mtx_grd) MutexGuardAttrDestroy(p_mtx_grd)

/*****************************************/

/******* Private type definitions ********/

typedef struct
{
    void*           addresses[__MTX_GRD_ADDR_NUM__];
    pthread_t       thread_id;
} MTX_GRD_ACQ_LOCATION;

typedef struct
{
    pthread_mutex_t         mutex;
    pthread_mutexattr_t     mutex_attr;
    MTX_GRD_ACQ_LOCATION    mutex_acq_location;
    unsigned long long      lock_counter;
    void*                   additional_data;
} MTX_GRD;

typedef struct
{
    MTX_GRD* p_mtx_grd_first;
    MTX_GRD* p_mtx_grd_second;
} MTX_GRD_TEST_PAIR;

typedef struct
{
    pthread_mutex_t* p_mtx_first;
    pthread_mutex_t* p_mtx_second;
} COMMON_MTX_TEST_PAIR;

typedef struct
{
    void*               relative_address;
    char                function_name[MTX_GRD_ACQ_FN_NAME_LEN + 1];
    unsigned long long  line;
    char                file_path[PATH_MAX + 1];
} MTX_GRD_ACQ_LOCATION_DETAIL;

typedef enum
{
    MTX_GRD_LOCK_TYPE_TRY       = 0                         ,
    MTX_GRD_LOCK_TYPE_PERMANENT                             ,
    MTX_GRD_LOCK_TYPE_TIMED                                 ,
    MTX_GRD_LOCK_TYPE_PERIODIC                              ,
    MTX_GRD_LOCK_TYPE_MIN       = MTX_GRD_LOCK_TYPE_TRY     ,
    MTX_GRD_LOCK_TYPE_MAX       = MTX_GRD_LOCK_TYPE_PERIODIC,
} MTX_GRD_LOCK_TYPES;

typedef enum
{
    MTX_GRD_VERBOSITY_SILENT    = 0                         ,
    MTX_GRD_VERBOSITY_LOCK_ERROR                            ,
    MTX_GRD_VERBOSITY_BT                                    ,
    MTX_GRD_VERBOSITY_ALL                                   , // Both lock errors as well as lock/unlock backtraces are printed.
    MTX_GRD_VERBOSITY_MIN       = MTX_GRD_VERBOSITY_SILENT  ,
    MTX_GRD_VERBOSITY_MAX       = MTX_GRD_VERBOSITY_ALL     ,
} MTX_GRD_VERBOSITY_LEVEL;

typedef struct timespec mtx_to_t;

/*****************************************/

/****** Private function prototypes ******/

static          void        MutexGuardInitModule(void) __attribute__((constructor));
static          void        MutexGuardEndModule(void) __attribute__((destructor));
static void                 MutexGuardSetPrintStatus(const MTX_GRD_VERBOSITY_LEVEL target_verbosity_level) __attribute__((unused));
static          int         MutexGuardAttrInit( MTX_GRD* restrict p_mutex_guard ,
                                                const int mutex_type            ,
                                                const int priority              ,
                                                const int proc_sharing          );
static          MTX_GRD*    MutexGuardAttrInitAddr( MTX_GRD* restrict p_mutex_guard ,
                                                    const int mutex_type            , 
                                                    const int priority              ,
                                                    const int proc_sharing          );
static  inline  int         MutexGuardInit(MTX_GRD* restrict p_mutex_guard);
static  inline  MTX_GRD*    MutexGuardInitAddr(MTX_GRD* restrict p_mutex_guard);
static          int         MutexGuardStoreNewAddress(MTX_GRD* restrict p_mutex_guard, void* address);
static          int         MutexGuardRemoveLatestAddress(MTX_GRD* restrict p_mutex_guard);
static          mtx_to_t    MutexGuardGenTimespec(const uint64_t timeout_ns);
static          void        MutexGuardPrintLockErrorCause(  const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                                            const pthread_mutex_t* restrict mutex_address                   ,
                                                            const uint64_t timeout_ns                                       ,
                                                            const int ret_lock                                              ,
                                                            char* lock_error_string                                         );
static          void        MutexGuardPrintLockAddresses(const MTX_GRD_ACQ_LOCATION* p_mutex_guard_acq_location, char* lock_error_string);
static          void        MutexGuardPrintLockError(   const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                                        const pthread_mutex_t* restrict target_mutex_addr               ,
                                                        const uint64_t timeout_ns                                       ,
                                                        const int ret_lock                                              );
static          int         MutexGuardLock( MTX_GRD* p_mutex_guard      ,
                                            void* restrict address      ,
                                            const uint64_t timeout_ns   ,
                                            const int lock_type         );
static  inline  MTX_GRD*    MutexGuardLockAddr(   MTX_GRD* restrict p_mutex_guard ,
                                                void* restrict address          ,
                                                const uint64_t timeout_ns       ,
                                                const int lock_type             );
static          size_t      MutexGuardGetExecutableBaseddress(void);
static          int         MutexGuardGetLockDetailFromAddr(const void* restrict addr                   ,
                                                            MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail)
                                                            __attribute__((unused));
static          int         MutexGuardPrintFileAndLineFromAddr( const void* restrict addr                   ,
                                                                char* output_buffer                         ,
                                                                const unsigned int address_index            ,
                                                                MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail)
                                                                __attribute__((unused));
static          void*       MutexGuardGetFuncRetAddr(void) __attribute__((noinline)) ;
static          int         MutexGuardUnlock(MTX_GRD* restrict p_mtx_grd);
static          void        MutexGuardReleaseMutexCleanup(void* ptr);
static  inline  int         MutexGuardAttrDestroy(MTX_GRD* restrict p_mtx_grd) __attribute__((unused));
static          void        MutexGuardDestroyAttrCleanup(void* ptr) __attribute__((unused));
static  inline  int         MutexGuardDestroy(MTX_GRD* restrict p_mtx_grd);
static          void        MutexGuardDestroyMutexCleanup(void* ptr) __attribute__((unused));
static          void        MutexGuardShowBacktrace(const pthread_mutex_t* restrict p_locked_mutex, const bool is_lock);
static          void*       TestDbgThreadRoutine(void* arg);

/*****************************************/

/*********** Private variables ***********/

static MTX_GRD acq_info_lock    = {0};
static int verbosity_level      = MTX_GRD_VERBOSITY_SILENT;

/*****************************************/

/********** Function definitions *********/

static void __attribute__((constructor)) MutexGuardInitModule(void)
{
    MTX_GRD_ATTR_INIT_SC(   &acq_info_lock          ,
                            PTHREAD_MUTEX_ERRORCHECK,
                            PTHREAD_PRIO_INHERIT    ,
                            PTHREAD_PROCESS_PRIVATE ,
                            p_acq_info_lock_attr    );

    MTX_GRD_INIT(&acq_info_lock);
}

static void __attribute__((destructor)) MutexGuardEndModule(void)
{
    MTX_GRD_DESTROY(&acq_info_lock);
}

static void __attribute__((unused)) MutexGuardSetPrintStatus(const MTX_GRD_VERBOSITY_LEVEL target_verbosity_level)
{
    if( (target_verbosity_level >= MTX_GRD_VERBOSITY_MIN) && (target_verbosity_level <= MTX_GRD_VERBOSITY_MAX) )
        verbosity_level = target_verbosity_level;
}

static int MutexGuardAttrInit(MTX_GRD* restrict p_mutex_guard ,
                            const int mutex_type            ,
                            const int priority              ,
                            const int proc_sharing          )
{
    if(!p_mutex_guard)
        return -1;
    
    int set_type        = pthread_mutexattr_settype(&p_mutex_guard->mutex_attr, mutex_type);
    int set_priority    = pthread_mutexattr_setprotocol(&p_mutex_guard->mutex_attr, priority);
    int set_proc_share  = pthread_mutexattr_setpshared(&p_mutex_guard->mutex_attr, proc_sharing);

    if( (set_type < 0) || (set_priority < 0) || (set_proc_share < 0))
        return -2;

    return 0;
}

static inline MTX_GRD* MutexGuardAttrInitAddr(  MTX_GRD* restrict p_mutex_guard ,
                                                const int mutex_type            ,
                                                const int priority              ,
                                                const int proc_sharing          )
{
    return (MutexGuardAttrInit(p_mutex_guard, mutex_type, priority, proc_sharing) ? NULL : p_mutex_guard);
}

static inline int MutexGuardInit(MTX_GRD* restrict p_mutex_guard)
{
    return (p_mutex_guard ? pthread_mutex_init(&p_mutex_guard->mutex, &p_mutex_guard->mutex_attr) : -1);
}

static inline MTX_GRD* MutexGuardInitAddr(MTX_GRD* restrict p_mutex_guard)
{
    return (MutexGuardInit(p_mutex_guard) ? NULL : p_mutex_guard);
}

static int MutexGuardStoreNewAddress(MTX_GRD* restrict p_mutex_guard, void* address)
{
    for(int address_index = 0; address_index < __MTX_GRD_ADDR_NUM__; address_index++)
    {
        if(!p_mutex_guard->mutex_acq_location.addresses[address_index])
            p_mutex_guard->mutex_acq_location.addresses[address_index] = address;
        return 0;
    }
    
    return -1;
}

static int MutexGuardRemoveLatestAddress(MTX_GRD* restrict p_mutex_guard)
{
    for(int address_index = (__MTX_GRD_ADDR_NUM__ - 1); address_index >= 0; address_index--)
    {
        if( p_mutex_guard->mutex_acq_location.addresses[address_index])
            p_mutex_guard->mutex_acq_location.addresses[address_index] = NULL;
        return 0;
    }
    
    return -1;
}

static mtx_to_t MutexGuardGenTimespec(const uint64_t timeout_ns)
{
    mtx_to_t lock_timeout;
    clock_gettime(CLOCK_REALTIME, &lock_timeout);
    
    // Add the timeout (in nanoseconds) to the current time
    lock_timeout.tv_sec += timeout_ns / MTX_GRD_TOUT_1_SEC_AS_NS;
    lock_timeout.tv_nsec += timeout_ns % MTX_GRD_TOUT_1_SEC_AS_NS;
    
    // Handle possible overflow in nanoseconds (if it's >= 1 second)
    if (lock_timeout.tv_nsec >= MTX_GRD_TOUT_1_SEC_AS_NS)
    {
        lock_timeout.tv_nsec -= MTX_GRD_TOUT_1_SEC_AS_NS;
        lock_timeout.tv_sec += 1;
    }

    return lock_timeout;
}

static void MutexGuardPrintLockErrorCause(  const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                            const pthread_mutex_t* restrict mutex_address                   ,
                                            const uint64_t timeout_ns                                       ,
                                            const int ret_lock                                              ,
                                            char* lock_error_string                                         )
{
    if(timeout_ns > 0 && (ret_lock == ETIMEDOUT))
        snprintf(   lock_error_string + strlen(lock_error_string)                       ,
                    (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                    MTX_GRD_MSG_ERR_MUTEX_TIMEOUT                                       ,
                    timeout_ns / MTX_GRD_TOUT_1_SEC_AS_NS                               ,
                    timeout_ns % MTX_GRD_TOUT_1_SEC_AS_NS                               );

    snprintf(   lock_error_string + strlen(lock_error_string)                       ,
                (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                MTX_GRD_MSG_ERR_MUTEX_ACQ                                           ,
                pthread_self()                                                      ,
                mutex_address                                                       ,
                strerror(ret_lock)                                                  );
    
    if(p_mutex_guard_acq_location->addresses[0])
        snprintf(   lock_error_string + strlen(lock_error_string)                       ,
                    (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                    MTX_GRD_MSG_ERR_MUTEX_ACQ_ADDR_HEADER                               ,
                    p_mutex_guard_acq_location->thread_id                               );
}

static void MutexGuardPrintLockAddresses(const MTX_GRD_ACQ_LOCATION* p_mutex_guard_acq_location, char* lock_error_string)
{
    MTX_GRD_ACQ_LOCATION_DETAIL detail = {0};

    for(unsigned int adress_index = 0; adress_index < __MTX_GRD_ADDR_NUM__; adress_index++)
    {
        if(!p_mutex_guard_acq_location->addresses[adress_index])
            return;
        
        MutexGuardPrintFileAndLineFromAddr( p_mutex_guard_acq_location->addresses[adress_index] ,
                                            lock_error_string                                   ,
                                            adress_index                                        ,
                                            &detail                                             );
        
        memset(detail.function_name, 0, strlen(detail.function_name));
        memset(detail.file_path, 0, strlen(detail.file_path));
        detail.line = 0;
        detail.relative_address = NULL;
    }
}

static void MutexGuardPrintLockError(   const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                        const pthread_mutex_t* restrict target_mutex_addr               ,
                                        const uint64_t timeout_ns                                       ,
                                        const int ret_lock                                              )
{
    char lock_error_string[MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN] = {0};

    strncat((lock_error_string + strlen(lock_error_string))                     ,
            MTX_GRD_MSG_ERR_MUTEX_HEADER                                        ,
            (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    MutexGuardPrintLockErrorCause(  p_mutex_guard_acq_location                      ,
                                    target_mutex_addr                               ,
                                    timeout_ns                                      ,
                                    ret_lock                                        ,
                                    (lock_error_string + strlen(lock_error_string)) );

    MutexGuardPrintLockAddresses(   p_mutex_guard_acq_location                      ,
                                    (lock_error_string + strlen(lock_error_string)) );

    strncat((lock_error_string + strlen(lock_error_string))                     ,
            MTX_GRD_MSG_ERR_MUTEX_FOOTER                                        ,
            (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    printf("%s", lock_error_string);
}

static void MutexGuardShowBacktrace(const pthread_mutex_t* restrict p_locked_mutex, const bool is_lock)
{
    if(p_locked_mutex == &acq_info_lock.mutex)
        return;
    
    void* call_stack[__MTX_GRD_FULL_BT_MAX_SIZE__ + 1] = {0};
    int call_stack_size = backtrace(call_stack, __MTX_GRD_FULL_BT_MAX_SIZE__ + 1);
    char** call_stack_symbols = backtrace_symbols(call_stack, call_stack_size);

    if(call_stack_symbols && call_stack_symbols[1])
    {
        char bt_id_str[MTX_GRD_BT_ID_LEN] = {0};

        snprintf(   bt_id_str                                                   ,
                    MTX_GRD_BT_ID_LEN                                           ,
                    MTX_GRD_BT_ID_FORMAT                                        ,
                    pthread_self()                                              ,
                    p_locked_mutex                                              ,
                    is_lock ? MTX_GRD_BT_ID_LOCK_STR : MTX_GRD_BT_ID_UNLOCK_STR );

        printf("%s%s\r\n", bt_id_str, MTX_GRD_BT_HEADER);

        unsigned bt_id_str_len = strlen(bt_id_str);

        char lock_error_string[MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN] = {0};
        strncpy(lock_error_string, bt_id_str, MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string));
        MTX_GRD_ACQ_LOCATION_DETAIL detail = {0};

        for(unsigned long long call_stack_index = 1; call_stack_index < call_stack_size; call_stack_index++)
        {
            snprintf((lock_error_string + strlen(lock_error_string))                    ,
                    (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                    MTX_GRD_BT_FRAME_INFO                                               ,
                    (call_stack_index - 1)                                              ,
                    call_stack_symbols[call_stack_index]                                );

            char* rel_addr_begin = strstr(call_stack_symbols[call_stack_index], MTX_GRD_BT_REL_ADDR_BEGIN_STR);
            char* rel_addr_end = strstr(call_stack_symbols[call_stack_index], MTX_GRD_BT_REL_ADDR_END_STR);
            
            if(!rel_addr_begin || !rel_addr_end)
            {
                printf("\r\n");
                continue;
            }

            char* end_ptr;

            *rel_addr_end = 0;
            unsigned long long rel_addr = strtoull( rel_addr_begin + strlen(MTX_GRD_BT_REL_ADDR_BEGIN_STR),
                                                    &end_ptr,
                                                    MTX_GRD_BT_REL_ADDR_HEX_BASE);
            
            if(*end_ptr)
            {
                printf("\r\n");
                continue;
            }

            detail.relative_address = (void*)rel_addr;
            
            int __attribute__((unused)) ret = MutexGuardGetLockDetailFromAddr(NULL, &detail);

            if(strcmp(detail.file_path, MTX_GRD_BT_NOT_FOUND_SYMBOL))
                snprintf(   (lock_error_string + strlen(lock_error_string))                     ,
                            (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                            MTX_GRD_BT_FRAME_TO_FILE_FORMAT                                     ,
                            detail.file_path                                                    );

            if(detail.line)
                snprintf(   (lock_error_string + strlen(lock_error_string))                     ,
                            (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                             MTX_GRD_BT_FRAME_TO_LINE_FORMAT                                    ,
                            detail.line                                                         );
            
            if(strcmp(detail.function_name, MTX_GRD_BT_NOT_FOUND_SYMBOL))
                snprintf(   (lock_error_string + strlen(lock_error_string))                     ,
                            (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)),
                            MTX_GRD_BT_FRAME_TO_FUNCTION_FORMAT                                       ,
                            detail.function_name                                                );

            printf("%s\r\n", lock_error_string);
            memset(lock_error_string + bt_id_str_len, 0, strlen(lock_error_string + bt_id_str_len));
        }

        printf("%s%s\r\n", bt_id_str, MTX_GRD_BT_FOOTER);
    }
}

static int MutexGuardLock(MTX_GRD* p_mutex_guard, void* restrict address, const uint64_t timeout_ns, const int lock_type)
{
    if(!p_mutex_guard)
        return -1;

    MTX_GRD_ACQ_LOCATION target_mutex_acq_location;

    // The mutex lock below is performed for other threads not to coincidentally modify the content within the provided MTX_GRD variable.
    // Executing the lock attempt below is not allowed if target mutex is acq_info_lock so as to avoid endless recursion. 
    if(p_mutex_guard != &acq_info_lock)
    {
        MTX_GRD_LOCK_SC(&acq_info_lock, p_acq_info_lock);
        memcpy(&target_mutex_acq_location, &p_mutex_guard->mutex_acq_location, sizeof(MTX_GRD_ACQ_LOCATION));
    }

    int ret_lock;

    if( (lock_type < MTX_GRD_LOCK_TYPE_MIN) || (lock_type > MTX_GRD_LOCK_TYPE_MAX) )
        return -1;
    
    mtx_to_t timed_lock_timeout;

    if( (lock_type == MTX_GRD_LOCK_TYPE_TIMED) || (lock_type == MTX_GRD_LOCK_TYPE_PERIODIC) )
        timed_lock_timeout = MutexGuardGenTimespec(timeout_ns);
    
    switch (lock_type)
    {
        case MTX_GRD_LOCK_TYPE_TRY:
        {
            ret_lock = pthread_mutex_trylock(&p_mutex_guard->mutex);
        }
        break;

        case MTX_GRD_LOCK_TYPE_PERMANENT:
        {
            ret_lock = pthread_mutex_lock(&p_mutex_guard->mutex);
        }
        break;
        
        case MTX_GRD_LOCK_TYPE_TIMED:
        {            
            ret_lock = pthread_mutex_timedlock(&p_mutex_guard->mutex, &timed_lock_timeout);
        }
        break;

        case MTX_GRD_LOCK_TYPE_PERIODIC:
        {
                do
                {
                    ret_lock = pthread_mutex_timedlock(&p_mutex_guard->mutex, &timed_lock_timeout);
                    
                    if(ret_lock == ETIMEDOUT)
                        if(verbosity_level & MTX_GRD_VERBOSITY_LOCK_ERROR)
                            MutexGuardPrintLockError(&target_mutex_acq_location, &p_mutex_guard->mutex, timeout_ns, ret_lock);
                }
                while(ret_lock == ETIMEDOUT);
        }   
        break;

        default:
        {
            return -1;
        }
        break;
    }

    if(ret_lock)
    {
        if(verbosity_level & MTX_GRD_VERBOSITY_LOCK_ERROR)
            MutexGuardPrintLockError(&target_mutex_acq_location, &p_mutex_guard->mutex, timeout_ns, ret_lock);

        return ret_lock;
    }

    int __attribute__((unused)) store_addr = MutexGuardStoreNewAddress(p_mutex_guard, address);

    p_mutex_guard->mutex_acq_location.thread_id = pthread_self();
    ++p_mutex_guard->lock_counter;

    if(verbosity_level & MTX_GRD_VERBOSITY_BT)
        MutexGuardShowBacktrace(&p_mutex_guard->mutex, true);

    return ret_lock;
}

static inline MTX_GRD* MutexGuardLockAddr(MTX_GRD* restrict p_mutex_guard, void* restrict address, const uint64_t timeout_ns, const int lock_type)
{
    return (MutexGuardLock(p_mutex_guard, address, timeout_ns, lock_type) ? NULL : p_mutex_guard);
}

static size_t MutexGuardGetExecutableBaseddress(void)
{
    FILE *maps = fopen(MTX_GRD_PROC_MAPS_PATH, "r");
    
    if (!maps)
    {
        perror(MTX_GRD_MSG_ERR_OPEN_PROC_MAPS);
        return 0;
    }

    char line[MTX_GRD_PROC_MAX_LEN];
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

static int __attribute__((unused)) MutexGuardGetLockDetailFromAddr(const void* restrict addr, MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail)
{
    char cmd[MTX_GRD_ADDR2LINE_CMD_FORMAT_MAX_LEN + 1] = {0};
    char full_path[PATH_MAX + 1] = {0};
    
    if(readlink(MTX_GRD_CURRENT_PROC_PATH, full_path, sizeof(full_path) - 1) < 0)
        return -1;
    
    if(!detail->relative_address)
        detail->relative_address = (void*)((size_t)addr - MutexGuardGetExecutableBaseddress());

    snprintf(   cmd                                 ,
                MTX_GRD_ADDR2LINE_CMD_FORMAT_MAX_LEN,
                MTX_GRD_ADDR2LINE_CMD_FORMAT        ,
                full_path                           ,
                detail->relative_address            );

    FILE *fp = popen(cmd, "r");
    
    if (fp == NULL)
    {
        perror(MTX_GRD_MSG_ERR_ADDR2LINE);
        return -1;
    }

    char line[PATH_MAX + 1] = {0};
    
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = 0;

        char* at_pos = strrchr(line, MTX_GRD_FN_LINE_DELIMITER);
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
            strncpy(detail->function_name, line, MTX_GRD_ACQ_FN_NAME_LEN);
        }
    }

    pclose(fp);

    return 0;
}

static int __attribute__((unused)) MutexGuardPrintFileAndLineFromAddr(  const void* restrict addr                   ,
                                                                        char* output_buffer                         ,
                                                                        const unsigned int address_index            ,
                                                                        MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail)
{
    int ret = MutexGuardGetLockDetailFromAddr(addr, detail);
    
    snprintf(   output_buffer + strlen(output_buffer)                           ,
                (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(output_buffer)),
                MTX_GRD_ACQ_LOCATION_FULL_FORMAT                                ,
                address_index                                                   ,
                addr                                                            ,
                detail->relative_address                                        ,
                detail->function_name                                           ,
                detail->file_path                                               ,
                detail->line                                                    );
    
    if(detail->line == 0)
    {
        char* line_delimiter_pos = strrchr(output_buffer, MTX_GRD_FN_LINE_DELIMITER);
        if(line_delimiter_pos)
        {
            memset(line_delimiter_pos, 0, strlen(line_delimiter_pos));
            strncat(output_buffer, "\r\n", (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(output_buffer)));
        }
    }

    return ret;
}

static __attribute__((noinline)) void* MutexGuardGetFuncRetAddr(void)
{
    return __builtin_return_address(0); 
}

static int MutexGuardUnlock(MTX_GRD* restrict p_mtx_grd)
{
    if(!p_mtx_grd)
        return -1;

    int ret_unlock = pthread_mutex_unlock(&p_mtx_grd->mutex);
    
    if(ret_unlock)
        return ret_unlock;

    int remove_addr = MutexGuardRemoveLatestAddress(p_mtx_grd);

    if(remove_addr < 0)
        return remove_addr;

    if(p_mtx_grd->lock_counter > 0)
        --p_mtx_grd->lock_counter;

    if(verbosity_level & MTX_GRD_VERBOSITY_BT)
        MutexGuardShowBacktrace(&p_mtx_grd->mutex, false);

    if( (p_mtx_grd->lock_counter == 0) && (p_mtx_grd != &acq_info_lock) )
    {
        MTX_GRD_LOCK_SC(&acq_info_lock, p_acq_info_lock);
        memset(&p_mtx_grd->mutex_acq_location, 0, sizeof(MTX_GRD_ACQ_LOCATION));
    }

    return ret_unlock;
}

static void MutexGuardReleaseMutexCleanup(void* ptr)
{
    if(!ptr || !(*(MTX_GRD**)ptr))
        return;
    
    MutexGuardUnlock(*(MTX_GRD**)ptr);
    *(MTX_GRD**)ptr = NULL;
}

static inline __attribute__((unused)) int MutexGuardAttrDestroy(MTX_GRD* restrict p_mtx_grd)
{
    return pthread_mutexattr_destroy(&p_mtx_grd->mutex_attr);
}

static __attribute__((unused)) void MutexGuardDestroyAttrCleanup(void* ptr)
{
    if(!ptr || !(*(MTX_GRD**)ptr))
        return;

    MutexGuardAttrDestroy(*(MTX_GRD**)ptr);
}

static inline int MutexGuardDestroy(MTX_GRD* restrict p_mtx_grd)
{
    return pthread_mutex_destroy(&p_mtx_grd->mutex);
}

static __attribute__((unused)) void MutexGuardDestroyMutexCleanup(void* ptr)
{
    if(!ptr)
        return;

    MutexGuardDestroy(*(MTX_GRD**)ptr);
}

static void* TestDbgThreadRoutine(void* arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    MTX_GRD_TEST_PAIR* p_mtx_grd_test_pair = (MTX_GRD_TEST_PAIR*)arg;

    MTX_GRD_TRY_LOCK_SC(p_mtx_grd_test_pair->p_mtx_grd_first, cleanup_var_first);

    usleep(MTX_GRD_DEADLOCK_SLEEP_US); // Let the current thread take a nap so as to cause a deadlock.

    MTX_GRD_TIMED_LOCK_SC(p_mtx_grd_test_pair->p_mtx_grd_second, MTX_GRD_DEFAULT_TOUT, cleanup_var_second);

    return NULL;
}

static void* TestCommonThreadRoutine(void* arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    COMMON_MTX_TEST_PAIR* p_mtx_test_pair = (COMMON_MTX_TEST_PAIR*)arg;

    int ret_lock = pthread_mutex_trylock(p_mtx_test_pair->p_mtx_first);

    usleep(MTX_GRD_DEADLOCK_SLEEP_US); // Let the current thread take a nap so as to cause a deadlock.

    mtx_to_t lock_timeout = MutexGuardGenTimespec(MTX_GRD_DEFAULT_TOUT);

    int timed_lock = pthread_mutex_timedlock(p_mtx_test_pair->p_mtx_second, &lock_timeout);

    if(!timed_lock)
    {
        printf("TO failed\n");
        pthread_mutex_unlock(p_mtx_test_pair->p_mtx_second);
    }
    
    if(!ret_lock)
        pthread_mutex_unlock(p_mtx_test_pair->p_mtx_first);

    return NULL;
}

void TestMutexGuard(void)
{
    pthread_t t_0, t_1;
    
    MTX_GRD_CREATE(mutex_guard_0);
    MTX_GRD_CREATE(mutex_guard_1);

    MTX_GRD_INIT_SC(&mutex_guard_0, cleanup_var_0);
    MTX_GRD_INIT_SC(&mutex_guard_1, cleanup_var_1);

    MTX_GRD_TEST_PAIR mutex_guard_test_pair_0 =
    {
        .p_mtx_grd_first    = &mutex_guard_0,
        .p_mtx_grd_second   = &mutex_guard_1,
    };

    MTX_GRD_TEST_PAIR mutex_guard_test_pair_1 =
    {
        .p_mtx_grd_first    = &mutex_guard_1,
        .p_mtx_grd_second   = &mutex_guard_0,
    };

    if(pthread_create(&t_0, NULL, TestDbgThreadRoutine, &mutex_guard_test_pair_0))
    {
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 0);

        return;
    }

    if(pthread_create(&t_1, NULL, TestDbgThreadRoutine, &mutex_guard_test_pair_1))
    {
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 1);
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
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 0);
        pthread_mutex_destroy(&common_mutex_0);
        pthread_mutex_destroy(&common_mutex_1);

        return;
    }

    if(pthread_create(&t_1, NULL, TestCommonThreadRoutine, &common_mutex_test_pair_1))
    {
        printf(MTX_GRD_MSG_ERR_THREAD_CREATION, 1);
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