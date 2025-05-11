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
#include "MutexGuard_api.h"

/*****************************************/

/*********** Define statements ***********/

#ifndef __MTX_GRD_FULL_BT_MAX_SIZE__
#define __MTX_GRD_FULL_BT_MAX_SIZE__    100
#endif

#ifndef __MTX_GRD_LAST_LOCK_ERR_STRING_LEN__
#define __MTX_GRD_LAST_LOCK_ERR_STRING_LEN__    10000
#endif

#ifndef __MTX_GRD_STD_ERR_STRING_LEN__
#define __MTX_GRD_STD_ERR_STRING_LEN__  1000
#endif

#define MTX_GRD_TOUT_1_SEC_AS_NS    (uint64_t)1000000000

#define MTX_GRD_LAST_LOCK_ERR_DEF_MSG   "Could not lock target mutex. "
#define MTX_GRD_STD_ERR_DEF_MSG         "Standard error code. "

#define MTX_GRD_PROC_MAX_LEN        (uint16_t)256
#define MTX_GRD_PROC_MAPS_PATH      "/proc/self/maps"
#define MTX_GRD_CURRENT_PROC_PATH   "/proc/self/exe"

#define MTX_GRD_ADDR2LINE_CMD_FORMAT_LEN        (uint8_t)26
#define MTX_GRD_ADDR2LINE_CMD_FORMAT            "addr2line -f --exe=%s +%p"
#define MTX_GRD_ADDR2LINE_CMD_FORMAT_MAX_LEN    MTX_GRD_ADDR2LINE_CMD_FORMAT_LEN + PATH_MAX
#define MTX_GRD_FN_LINE_DELIMITER               ':'

#define MTX_GRD_ACQ_LOCATION_FULL_FORMAT    "#%u %p (+%p): %s defined at %s:%llu\r\n"

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

/******* Private type definitions ********/

/// @brief Helper struct to store mutex lock acquisition detail.
typedef struct C_MUTEX_GUARD_ALIGNED
{
    void*               relative_address;
    char                function_name[MTX_GRD_ACQ_FN_NAME_LEN + 1];
    unsigned long long  line;
    char                file_path[PATH_MAX + 1];
} MTX_GRD_ACQ_LOCATION_DETAIL;

/// @brief Error codes to be stored in mutex_guard_errno.
typedef enum
{
    MTX_GRD_ERR_INVALID_VERBOSITY_LEVEL             = 1000  ,
    MTX_GRD_ERR_NULL_MTX_GRD                                ,
    MTX_GRD_ERR_NULL_MTX_GRD_ACQ                            ,
    MTX_GRD_ERR_INIT_FAILED                                 ,
    MTX_GRD_ERR_NULL_TARGET_STRING                          ,
    MTX_GRD_ERR_NO_STORED_LOCK_ADDRESSES                    ,
    MTX_GRD_ERR_ATTR_SET_FAILED                             ,
    MTX_GRD_ERR_NO_ADDR_SPACE_AVAILABLE                     ,
    MTX_GRD_ERR_INVALID_LOCK_TYPE                           ,
    MTX_GRD_ERR_LOCK_ERROR                                  ,
    MTX_GRD_ERR_STD_ERROR_CODE                              ,
    MTX_GRD_ERR_NOT_LOCKED                                  ,
    MTX_GRD_ERR_INVALID_OWNER_TID                           ,
    MTX_GRD_ERR_COULD_NOT_FIND_PROCESS                      ,
    MTX_GRD_ERR_COULD_NOT_CLOSE_PROC_FILE                   ,
    MTX_GRD_ERR_COULD_NOT_FIND_EXE_PATH                     ,
    MTX_GRD_ERR_COULD_NOT_EXECUTE_ADDR2LINE                 ,
    MTX_GRD_ERR_COULD_NOT_CLOSE_ADDR2LINE                   ,
    MTX_GRD_ERR_OUT_OF_ADDR_COUNTER_BOUNDARIES              ,
    MTX_GRD_ERR_OUT_OF_BOUNDARIES_ERR                       ,

    MTX_GRD_ERR_MIN = MTX_GRD_ERR_INVALID_VERBOSITY_LEVEL   ,
    MTX_GRD_ERR_MAX = MTX_GRD_ERR_OUT_OF_BOUNDARIES_ERR     ,
} MTX_GRD_ERR_CODE;

/// @brief Alias for timespec struct. 
typedef struct timespec mtx_to_t;

/*****************************************/

/****** Private function prototypes ******/

static void MutexGuardInitModule(void) __attribute__((constructor));
static void MutexGuardEndModule(void) __attribute__((destructor));

static int MutexGuardStoreNewAddress(MTX_GRD* restrict p_mutex_guard, void* address);
static int MutexGuardRemoveLatestAddress(MTX_GRD* restrict p_mutex_guard);

static mtx_to_t MutexGuardGenTimespec(const uint64_t timeout_ns);

static int MutexGuardGetLockError(  const MTX_GRD* restrict p_mutex_guard   ,
                                    const uint64_t timeout_ns               ,
                                    char* lock_error_string                 ,
                                    const size_t lock_error_str_size        );

static void MutexGuardPrintLockError(   const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                        const pthread_mutex_t* restrict target_mutex_addr               ,
                                        const uint64_t timeout_ns                                       ,
                                        const int ret_lock                                              );

static int MutexGuardCopyLockError( const MTX_GRD* restrict p_mutex_guard   ,
                                    const uint64_t timeout_ns               ,
                                    const int ret_lock                      ,
                                    char* lock_error_string                 ,
                                    const size_t lock_error_str_size        );
static int MutexGuardPrintLockErrorCause(   const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                            const pthread_mutex_t* restrict mutex_address                   ,
                                            const uint64_t timeout_ns                                       ,
                                            const int ret_lock                                              ,
                                            char* lock_error_string                                         ,
                                            const size_t lock_error_str_size                                );
static int MutexGuardPrintLockAddresses(const MTX_GRD_ACQ_LOCATION* p_mutex_guard_acq_location  ,
                                        char* lock_error_string                                 ,
                                        const size_t lock_error_str_size                        );

static size_t MutexGuardGetExecutableBaseddress(void);
static int MutexGuardGetLockDetailFromAddr( const void* restrict addr                   ,
                                            MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail);
static int MutexGuardPrintFileAndLineFromAddr(  const void* restrict addr                   ,
                                                char* output_buffer                         ,
                                                const unsigned int address_index            ,
                                                MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail,
                                                const size_t lock_error_str_size            );

static void MutexGuardShowBacktrace(const pthread_mutex_t* restrict p_locked_mutex, const bool is_lock);

/*****************************************/

/*********** Private variables ***********/

/// @brief Acquisition info lock. Used when trying to lock a mutex.
static MTX_GRD acq_info_lock = {0};
/// @brief Variable in which data belonging to a mutex that failed to be locked is meant to be copied.
static MTX_GRD last_failed_mutex_guard = {0};
/// @brief Verbosity level holding variable.
static int verbosity_level = MTX_GRD_VERBOSITY_SILENT;
/// @brief MTX_GRD_ERR_CODE holding variable.
static __thread int mutex_guard_errno = 0;    
/// @brief Variable storing values returned by POSIX thread locking/unlocking functions.
static __thread int mutex_guard_lock_error_code = 0;
/// @brief String to store lock error strings.
static __thread char last_lock_error_string[__MTX_GRD_LAST_LOCK_ERR_STRING_LEN__] = MTX_GRD_LAST_LOCK_ERR_DEF_MSG;
/// @brief String to store standard error strings.
static __thread char standard_error_string[__MTX_GRD_STD_ERR_STRING_LEN__] = MTX_GRD_STD_ERR_DEF_MSG;

/// @brief Error code strings (related to mutex_guard_errno).
static const char* error_str_table[MTX_GRD_ERR_MAX - MTX_GRD_ERR_MIN + 1] =
{
    "Invalid verbosity level"                           ,
    "MTX_GRD null pointer"                              ,
    "MTX_GRD_ACQ_LOCATION null pointer"                 ,
    "Mutex Guard initialization failed"                 ,
    "Target string to write is null"                    ,
    "No stored lock addresses have been found"          ,
    "Attribute setting failed"                          ,
    "No space available for more addresses"             ,
    "Invalid lock type provided"                        ,
    NULL                                                ,
    NULL                                                ,
    "MTX_GRD was not locked beforehand"                 ,
    "Owner thread's TID does not match current one"     ,
    "Could not find current process information"        ,
    "Could not close current process information file"  ,
    "Could not close current executable\'s file path"   ,
    "Could not execute addr2line"                       ,
    "Could not close addr2line running process"         ,
    "Address counter is out of boundaries"              ,
    "Out of boundaries error code"                      ,
};

/*****************************************/

/********** Function definitions *********/

/// @brief Initializes module by setting default verbosity and initializing shared resources mutex.
static void __attribute__((constructor)) MutexGuardInitModule(void)
{
    MutexGuardSetPrintStatus(MTX_GRD_VERBOSITY_SILENT);

    MTX_GRD_ATTR_INIT_SC(   &acq_info_lock          ,
                            PTHREAD_MUTEX_ERRORCHECK,
                            PTHREAD_PRIO_INHERIT    ,
                            PTHREAD_PROCESS_PRIVATE ,
                            p_acq_info_lock_attr    );

    MTX_GRD_INIT(&acq_info_lock);
}


/// @brief Destroys shared resources mutex lock. 
static void __attribute__((destructor)) MutexGuardEndModule(void)
{
    MTX_GRD_DESTROY(&acq_info_lock);
}

/// @brief Returns Mutex Guard error code.
/// @return Currently stored error code.
/// @warning Stores last error code (same as errno), even if no error happened lately. Use with care.
int MutexGuardGetErrorCode(void)
{
    return mutex_guard_errno;
}

/// @brief Returns pointer to error describing string.
/// @param error_code Target error code to be described.
/// @return Pointer to error string.
/// @warning As last error code is stored each time, so a string may be returned even if no error happened lately. Use with care.
const char* MutexGuardGetErrorString(const int error_code)
{
    if( (error_code < MTX_GRD_ERR_MIN) || (error_code > MTX_GRD_ERR_MAX) )
        return error_str_table[MTX_GRD_ERR_MAX - MTX_GRD_ERR_MIN];

    if(error_code == MTX_GRD_ERR_LOCK_ERROR)
    {
        char* custom_err_code_start = last_lock_error_string + strlen(MTX_GRD_LAST_LOCK_ERR_DEF_MSG);
        memset( custom_err_code_start, 0, strlen(custom_err_code_start));
        MutexGuardGetLockError( &last_failed_mutex_guard, 0, custom_err_code_start, sizeof(last_lock_error_string));
        return last_lock_error_string;
    }
    
    if(error_code == MTX_GRD_ERR_STD_ERROR_CODE)
    {
        char* custom_std_code_start = standard_error_string + strlen(MTX_GRD_STD_ERR_DEF_MSG);
        memset(custom_std_code_start, 0, strlen(custom_std_code_start));
        strncpy(custom_std_code_start                                           ,
                strerror(mutex_guard_lock_error_code)                           ,
                (sizeof(custom_std_code_start) - strlen(custom_std_code_start)) );
        return standard_error_string;
    }

    return error_str_table[error_code - MTX_GRD_ERR_MIN];
}

/// @brief Prints error.
/// @param custom_error_msg String to be copied to.
void MutexGuardPrintError(const char* restrict custom_error_msg)
{
    bool msg_exists = true;
    if( (custom_error_msg == NULL) || (*custom_error_msg == '\0') )
        msg_exists = false;
    
    fprintf(stderr                                  ,
            "%s%s%s\r\n"                            ,
            (msg_exists ? custom_error_msg : "" )   ,
            (msg_exists ? ": " : "" )               ,
            MTX_GRD_GET_LAST_ERR_STR                );
}

/// @brief Sets verbosity level.
/// @param target_verbosity_level Target verbosity level (silent, lock errors, backtrace or both). 
/// @return 0 if succeeded, < 0 if invalid verbosity level was provided.
int MutexGuardSetPrintStatus(const MTX_GRD_VERBOSITY_LEVEL target_verbosity_level)
{
    if( (target_verbosity_level < MTX_GRD_VERBOSITY_MIN) || (target_verbosity_level > MTX_GRD_VERBOSITY_MAX) )
    {
        mutex_guard_errno = MTX_GRD_ERR_INVALID_VERBOSITY_LEVEL;
        return -1;
    }

    verbosity_level = target_verbosity_level;
    
    return 0;
}

/// @brief Gets verbosity level.
/// @return Currently assigned verbosity level.
MTX_GRD_VERBOSITY_LEVEL MutexGuardGetPrintStatus(void)
{
    return verbosity_level;
}

/// @brief Initializes mutex attribute.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param mutex_type Mutex type (NORMAL, ERRORCHECK, RECURSIVE, DEFAULT).
/// @param priority Mutex priority (NONE, INHERIT, PROTECT).
/// @param proc_sharing Share mutex with other processes (PRIVATE, SHARED).
/// @return 0 if succeeded, < 0 otherwise.
int MutexGuardAttrInit( MTX_GRD* restrict p_mutex_guard ,
                        const int mutex_type            ,
                        const int priority              ,
                        const int proc_sharing          )
{
    if(!p_mutex_guard)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }
    
    int set_type        = pthread_mutexattr_settype(&p_mutex_guard->mutex_attr, mutex_type);
    int set_priority    = pthread_mutexattr_setprotocol(&p_mutex_guard->mutex_attr, priority);
    int set_proc_share  = pthread_mutexattr_setpshared(&p_mutex_guard->mutex_attr, proc_sharing);

    if( set_type | set_priority | set_proc_share )
    {
        mutex_guard_errno = MTX_GRD_ERR_ATTR_SET_FAILED;
        return -2;
    }

    return 0;
}

/// @brief Initializes mutex.
/// @param p_mutex_guard Pointer to mutex containing mutex guard structure.
/// @return 0 if succeeded, != 0 otherwise.
int MutexGuardInit(MTX_GRD* restrict p_mutex_guard)
{
    if(!p_mutex_guard)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

    int try_init = pthread_mutex_init(&p_mutex_guard->mutex, &p_mutex_guard->mutex_attr);

    if(try_init)
    {
        mutex_guard_errno = MTX_GRD_ERR_INIT_FAILED;
        return -2;
    }

    return 0;
}

/// @brief Stores a new lock address.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param address Target address to be stored.
/// @return 0 if succeeded, < 0 otherwise.
static int MutexGuardStoreNewAddress(MTX_GRD* restrict p_mutex_guard, void* address)
{
    if(!p_mutex_guard)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

    for(int address_index = 0; address_index < __MTX_GRD_ADDR_NUM__; address_index++)
    {
        if(!p_mutex_guard->mutex_acq_location.addresses[address_index])
        {
            p_mutex_guard->mutex_acq_location.addresses[address_index] = address;
            return 0;
        }
    }
    
    mutex_guard_errno = MTX_GRD_ERR_NO_ADDR_SPACE_AVAILABLE;
    return -2;
}

/// @brief Removes latest lock address.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @return 0 if succeeded, < 0 otherwise.
static int MutexGuardRemoveLatestAddress(MTX_GRD* restrict p_mutex_guard)
{
    if(!p_mutex_guard)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

    for(int address_index = (__MTX_GRD_ADDR_NUM__ - 1); address_index >= 0; address_index--)
    {
        if( p_mutex_guard->mutex_acq_location.addresses[address_index])
        {
            p_mutex_guard->mutex_acq_location.addresses[address_index] = NULL;
            return 0;
        }
    }
    
    mutex_guard_errno = MTX_GRD_ERR_NO_ADDR_SPACE_AVAILABLE;
    return -2;
}

/// @brief Returns timespec type struct to be used alongside timed locks.
/// @param timeout_ns Target timeout value (in nanoseconds).
/// @return Resulting timespec.
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

/// @brief Copies lock error to a provided buffer.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param ret_lock Value returned by pthread mutex locking function.
/// @param lock_error_string Buffer where the error is meant to eb copied to.
/// @param lock_error_str_size Buffer size.
/// @return 0 if succeeded, < 0 otherwise.
static int MutexGuardCopyLockError( const MTX_GRD* restrict p_mutex_guard   ,
                                    const uint64_t timeout_ns               ,
                                    const int ret_lock                      ,
                                    char* lock_error_string                 ,
                                    const size_t lock_error_str_size        )
{
    if(!p_mutex_guard)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

    if(!lock_error_string)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_TARGET_STRING;
        return -2;
    }

    MTX_GRD_ACQ_LOCATION mutex_guard_acq_location = {};

    if(p_mutex_guard != &acq_info_lock)
    {
        MTX_GRD_LOCK_SC(&acq_info_lock, p_acq_info_lock);
        memcpy(&mutex_guard_acq_location, &p_mutex_guard->mutex_acq_location, sizeof(MTX_GRD_ACQ_LOCATION));
    }

    MutexGuardPrintLockErrorCause(  &mutex_guard_acq_location                           ,
                                    &p_mutex_guard->mutex                               ,
                                    timeout_ns                                          ,
                                    ret_lock                                            ,
                                    (lock_error_string + strlen(lock_error_string))     ,
                                    (lock_error_str_size - strlen(lock_error_string))   );

    MutexGuardPrintLockAddresses(   &mutex_guard_acq_location                           ,
                                    (lock_error_string + strlen(lock_error_string))     ,
                                    (lock_error_str_size - strlen(lock_error_string))   );
    
    return 0;
}

/// @brief Gets lock error and copies it provided buffer.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param lock_error_string Buffer where the error is meant to eb copied to.
/// @param lock_error_str_size Buffer size.
/// @return 0 if succeeded, < 0 otherwise.
int MutexGuardGetLockError( const MTX_GRD* restrict p_mutex_guard   ,
                            const uint64_t timeout_ns               ,
                            char* lock_error_string                 ,
                            const size_t lock_error_str_size        )
{
    if(!p_mutex_guard)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

    if(!lock_error_string)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_TARGET_STRING;
        return -2;
    }

    int copy_lock_error_string = MutexGuardCopyLockError(   p_mutex_guard               ,
                                                            timeout_ns                  ,
                                                            mutex_guard_lock_error_code ,
                                                            lock_error_string           ,
                                                            lock_error_str_size         );
    
    if(copy_lock_error_string < 0)
    {   
        strncat(lock_error_string                                   ,
                MTX_GRD_GET_LAST_ERR_STR                            ,
                (lock_error_str_size - strlen(lock_error_string))   );
        
        return -3;
    }
    
    return 0;
}

/// @brief Stores lock error cause to a given buffer.
/// @param p_mutex_guard_acq_location Pointer to mutex acquisition location struct.
/// @param mutex_address Pointer to target mutex variable.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param ret_lock Value returned by pthread mutex locking function.
/// @param lock_error_string Buffer where the error is meant to eb copied to.
/// @param lock_error_str_size Buffer size.
/// @return 0 if succeeded, < 0 otherwise.
static int MutexGuardPrintLockErrorCause(   const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                            const pthread_mutex_t* restrict mutex_address                   ,
                                            const uint64_t timeout_ns                                       ,
                                            const int ret_lock                                              ,
                                            char* lock_error_string                                         ,
                                            const size_t lock_error_str_size                                )
{
    if(!p_mutex_guard_acq_location)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD_ACQ;
        return -1;
    }

    if(!lock_error_string)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_TARGET_STRING;
        return -2;
    }

    if(timeout_ns > 0 && (ret_lock == ETIMEDOUT))
        snprintf(   lock_error_string + strlen(lock_error_string)       ,
                    (lock_error_str_size - strlen(lock_error_string))   ,
                    MTX_GRD_MSG_ERR_MUTEX_TIMEOUT                       ,
                    timeout_ns / MTX_GRD_TOUT_1_SEC_AS_NS               ,
                    timeout_ns % MTX_GRD_TOUT_1_SEC_AS_NS               );

    snprintf(   lock_error_string + strlen(lock_error_string)       ,
                (lock_error_str_size - strlen(lock_error_string))   ,
                MTX_GRD_MSG_ERR_MUTEX_ACQ                           ,
                pthread_self()                                      ,
                mutex_address                                       ,
                strerror(ret_lock)                                  );
    
    if(p_mutex_guard_acq_location->addresses[0])
        snprintf(   lock_error_string + strlen(lock_error_string)       ,
                    (lock_error_str_size - strlen(lock_error_string))   ,
                    MTX_GRD_MSG_ERR_MUTEX_ACQ_ADDR_HEADER               ,
                    p_mutex_guard_acq_location->thread_id               );
    
    return 0;
}

/// @brief Prints lock address history to a given buffer.
/// @param p_mutex_guard_acq_location Pointer to given mutex acquisition location.
/// @param lock_error_string Buffer where the error is meant to eb copied to.
/// @param lock_error_str_size Buffer size.
/// @return 0 if succeeded, < 0 otherwise.
static int MutexGuardPrintLockAddresses(const MTX_GRD_ACQ_LOCATION* p_mutex_guard_acq_location  ,
                                        char* lock_error_string                                 ,
                                        const size_t lock_error_str_size                        )
{
    if(!p_mutex_guard_acq_location)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD_ACQ;
        return -1;
    }

    if(!lock_error_string)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_TARGET_STRING;
        return -2;
    }

    MTX_GRD_ACQ_LOCATION_DETAIL detail = {0};

    if(!p_mutex_guard_acq_location->addresses[0])
    {
        mutex_guard_errno = MTX_GRD_ERR_NO_STORED_LOCK_ADDRESSES;
        return -3;
    }

    for(unsigned int adress_index = 0; adress_index < __MTX_GRD_ADDR_NUM__; adress_index++)
    {
        if(!p_mutex_guard_acq_location->addresses[adress_index])
            break;
        
        MutexGuardPrintFileAndLineFromAddr( p_mutex_guard_acq_location->addresses[adress_index] ,
                                            lock_error_string                                   ,
                                            adress_index                                        ,
                                            &detail                                             ,
                                            (lock_error_str_size - strlen(lock_error_string))   );
        
        memset(detail.function_name, 0, strlen(detail.function_name));
        memset(detail.file_path, 0, strlen(detail.file_path));
        detail.line = 0;
        detail.relative_address = NULL;
    }

    return 0;
}

/// @brief Directly prints mutex lock error to standard output.
/// @param p_mutex_guard_acq_location Pointer to given mutex acquisition location.
/// @param target_mutex_addr Pointer to target mutex variable.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param ret_lock Value returned by pthread mutex locking function.
void MutexGuardPrintLockError(  const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                const pthread_mutex_t* restrict target_mutex_addr               ,
                                const uint64_t timeout_ns                                       ,
                                const int ret_lock                                              )
{
    char lock_error_string[MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN] = {0};

    strncat((lock_error_string + strlen(lock_error_string))                     ,
            MTX_GRD_MSG_ERR_MUTEX_HEADER                                        ,
            (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    MutexGuardPrintLockErrorCause(  p_mutex_guard_acq_location                                          ,
                                    target_mutex_addr                                                   ,
                                    timeout_ns                                                          ,
                                    ret_lock                                                            ,
                                    (lock_error_string + strlen(lock_error_string))                     ,
                                    (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    MutexGuardPrintLockAddresses(   p_mutex_guard_acq_location                                          ,
                                    (lock_error_string + strlen(lock_error_string))                     ,
                                    (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    strncat((lock_error_string + strlen(lock_error_string))                     ,
            MTX_GRD_MSG_ERR_MUTEX_FOOTER                                        ,
            (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(lock_error_string)));

    printf("%s", lock_error_string);
}

/// @brief Directly prints mutex lock/unlock backtrace to standard output.
/// @param p_locked_mutex Address of mutex to be locked/unlocked.
/// @param is_lock true for locking operations, false for unlocking.
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
            
            MutexGuardGetLockDetailFromAddr(NULL, &detail);

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
                            MTX_GRD_BT_FRAME_TO_FUNCTION_FORMAT                                 ,
                            detail.function_name                                                );

            printf("%s\r\n", lock_error_string);
            memset(lock_error_string + bt_id_str_len, 0, strlen(lock_error_string + bt_id_str_len));
        }

        printf("%s%s\r\n", bt_id_str, MTX_GRD_BT_FOOTER);
    }
}

/// @brief Locks target mutex.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param address Address in which the current mutex is being tried to be locked.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param lock_type Lock type (TR_LOCK, LOCK, TIMED_LOCK, PERIODIC_TIMED_LOCK).
/// @return 0 if succeeded, != 0 otherwise.
int MutexGuardLock(MTX_GRD* p_mutex_guard, void* restrict address, const uint64_t timeout_ns, const int lock_type)
{
    if(!p_mutex_guard)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

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
    {
        mutex_guard_errno = MTX_GRD_ERR_INVALID_LOCK_TYPE;
        return -2;
    }
    
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
            mutex_guard_errno = MTX_GRD_ERR_INVALID_LOCK_TYPE;
            return -2;
        }
        break;
    }

    if(ret_lock)
    {
        if(verbosity_level & MTX_GRD_VERBOSITY_LOCK_ERROR)
            MutexGuardPrintLockError(&target_mutex_acq_location, &p_mutex_guard->mutex, timeout_ns, ret_lock);

        mutex_guard_errno           = MTX_GRD_ERR_LOCK_ERROR;
        mutex_guard_lock_error_code = ret_lock;

        MTX_GRD_LOCK_SC(&acq_info_lock, p_acq_info_lock);
        memcpy(&last_failed_mutex_guard, p_mutex_guard, sizeof(MTX_GRD));
        
        return ret_lock;
    }

    if(p_mutex_guard != &acq_info_lock)
        mutex_guard_lock_error_code = 0;

    MutexGuardStoreNewAddress(p_mutex_guard, address);

    p_mutex_guard->mutex_acq_location.thread_id = pthread_self();

    ++p_mutex_guard->lock_counter;
    
    if(p_mutex_guard->lock_counter > __MTX_GRD_ADDR_NUM__)
        mutex_guard_errno = MTX_GRD_ERR_OUT_OF_ADDR_COUNTER_BOUNDARIES;

    if(verbosity_level & MTX_GRD_VERBOSITY_BT)
        MutexGuardShowBacktrace(&p_mutex_guard->mutex, true);

    return ret_lock;
}

/// @brief Gets base address of the executable in which a mutex lock error has happened.
/// @return 0 if succeeded, < 0 otherwise.
static size_t MutexGuardGetExecutableBaseddress(void)
{
    FILE *maps = fopen(MTX_GRD_PROC_MAPS_PATH, "r");
    
    if (!maps)
    {
        mutex_guard_errno = MTX_GRD_ERR_COULD_NOT_FIND_PROCESS;
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

    int close_maps = fclose(maps);

    if(close_maps)
        mutex_guard_errno = MTX_GRD_ERR_COULD_NOT_CLOSE_PROC_FILE;

    return start;
}

/// @brief Gets mutex lock acquisition details from the address in which a mutex was meant to be stored.
/// @param addr Mutex lock address.
/// @param detail Pointer to lock acquisition detail struct.
/// @return 0 if succeeded, < 0 otherwise.
static int MutexGuardGetLockDetailFromAddr(const void* restrict addr, MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail)
{
    char cmd[MTX_GRD_ADDR2LINE_CMD_FORMAT_MAX_LEN + 1] = {0};
    char full_path[PATH_MAX + 1] = {0};
    
    if(readlink(MTX_GRD_CURRENT_PROC_PATH, full_path, sizeof(full_path) - 1) < 0)
    {
        mutex_guard_errno = MTX_GRD_ERR_COULD_NOT_FIND_EXE_PATH;
        return -1;
    }
    
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
        mutex_guard_errno = MTX_GRD_ERR_COULD_NOT_EXECUTE_ADDR2LINE;
        return -2;
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

    int close_addr2line = pclose(fp);
    
    if(close_addr2line)
        mutex_guard_errno = MTX_GRD_ERR_COULD_NOT_CLOSE_ADDR2LINE;

    return 0;
}

/// @brief Prints file and line to a buffer given an address.
/// @param addr Target address to be detailed.
/// @param output_buffer Output buffer.
/// @param address_index Address index within mutex guard.
/// @param detail Pointer to mutex acquisition detail struct. 
/// @param lock_error_str_size Output buffer size.
/// @return 0 if succeeded, < 0 otherwise.
static int MutexGuardPrintFileAndLineFromAddr(  const void* restrict addr                   ,
                                                char* output_buffer                         ,
                                                const unsigned int address_index            ,
                                                MTX_GRD_ACQ_LOCATION_DETAIL* restrict detail,
                                                const size_t lock_error_str_size            )
{
    int ret = MutexGuardGetLockDetailFromAddr(addr, detail);
    if(ret < 0)
        return ret;
    
    snprintf(   output_buffer + strlen(output_buffer)           ,
                (lock_error_str_size - strlen(output_buffer))   ,
                MTX_GRD_ACQ_LOCATION_FULL_FORMAT                ,
                address_index                                   ,
                addr                                            ,
                detail->relative_address                        ,
                detail->function_name                           ,
                detail->file_path                               ,
                detail->line                                    );
    
    if(detail->line == 0)
    {
        char* line_delimiter_pos = strrchr(output_buffer, MTX_GRD_FN_LINE_DELIMITER);
        if(line_delimiter_pos)
        {
            memset(line_delimiter_pos, 0, strlen(line_delimiter_pos));
            strncat(output_buffer, "\r\n", (MTX_GRD_MSG_ERR_MUTEX_LOCK_ERR_STR_LEN - strlen(output_buffer)));
        }
    }

    return 0;
}

/// @brief Returns address within the program of line in which the current function was called. Meant to be used in macros.
/// @return Current function calling address.
C_MUTEX_GUARD_NINLINE void* MutexGuardGetFuncRetAddr(void)
{
    return __builtin_return_address(0); 
}

/// @brief Unlocks target mutex.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, != 0 otherwise.
int MutexGuardUnlock(MTX_GRD* restrict p_mtx_grd)
{
    if(!p_mtx_grd)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

    if(p_mtx_grd->mutex_acq_location.thread_id == 0)
    {
        mutex_guard_errno = MTX_GRD_ERR_NOT_LOCKED;
        return -2;
    }

    if(pthread_self() != p_mtx_grd->mutex_acq_location.thread_id)
    {
        mutex_guard_errno = MTX_GRD_ERR_INVALID_OWNER_TID;
        return -3;
    }

    int ret_unlock = pthread_mutex_unlock(&p_mtx_grd->mutex);
    
    if(ret_unlock)
    {
        mutex_guard_lock_error_code = ret_unlock;
        mutex_guard_errno           = MTX_GRD_ERR_STD_ERROR_CODE;
        
        return ret_unlock;
    }

    if(p_mtx_grd != &acq_info_lock)
        mutex_guard_lock_error_code = 0;

    MutexGuardRemoveLatestAddress(p_mtx_grd);

    if(p_mtx_grd->lock_counter > 0)
        --p_mtx_grd->lock_counter;
    else
        mutex_guard_errno = MTX_GRD_ERR_OUT_OF_ADDR_COUNTER_BOUNDARIES;

    if(verbosity_level & MTX_GRD_VERBOSITY_BT)
        MutexGuardShowBacktrace(&p_mtx_grd->mutex, false);

    if( (p_mtx_grd->lock_counter == 0) && (p_mtx_grd != &acq_info_lock) )
    {
        MTX_GRD_LOCK_SC(&acq_info_lock, p_acq_info_lock);
        memset(&p_mtx_grd->mutex_acq_location, 0, sizeof(MTX_GRD_ACQ_LOCATION));
    }

    return ret_unlock;
}

/// @brief Cleanup function to release a mutex (meant to be used alongside scoped mutex lock macros).
/// @param ptr Pointer to mutex guard structure. 
void MutexGuardReleaseMutexCleanup(void* ptr)
{
    if(!ptr || !(*(MTX_GRD**)ptr))
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return;
    }
    
    MutexGuardUnlock(*(MTX_GRD**)ptr);
    *(MTX_GRD**)ptr = NULL;
}

/// @brief Destroys mutex attribute within given mutex guard.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, > 0 otherwise.
int MutexGuardAttrDestroy(MTX_GRD* restrict p_mtx_grd)
{
    if(!p_mtx_grd)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }
    
    int mutex_attr_destroy = pthread_mutexattr_destroy(&p_mtx_grd->mutex_attr);
    
    if(mutex_attr_destroy)
        mutex_guard_errno = MTX_GRD_ERR_STD_ERROR_CODE;
    
    return mutex_attr_destroy;
}

/// @brief Destroys mutex within given mutex guard.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, > 0 otherwise.
int MutexGuardDestroy(MTX_GRD* restrict p_mtx_grd)
{
    if(!p_mtx_grd)
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return -1;
    }

    int original_lock_counter = p_mtx_grd->lock_counter;

    int ret_unlock = 0;
    for(int i = 0; i < original_lock_counter; i++)
    {
        ret_unlock = MutexGuardUnlock(p_mtx_grd);
        
        if(ret_unlock < 0)
            return -2;
    }
    
    int mutex_destroy = pthread_mutex_destroy(&p_mtx_grd->mutex);
    
    if(mutex_destroy)
        mutex_guard_errno = MTX_GRD_ERR_STD_ERROR_CODE;
    
    return mutex_destroy;
}

/// @brief Cleanup function to destroy a mutex attribute (meant to be used alongside scoped attribute init macros).
/// @param ptr Pointer to mutex attribute containing mutex guard structure. 
void MutexGuardDestroyAttrCleanup(void* ptr)
{
    if(!ptr || !(*(MTX_GRD**)ptr))
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return;
    }

    MutexGuardAttrDestroy(*(MTX_GRD**)ptr);
}

/// @brief Cleanup function to destroy a mutex (meant to be used alongside scoped mutex init macros)
/// @param ptr Pointer to mutex guard structure.
void MutexGuardDestroyMutexCleanup(void* ptr)
{
    if(!ptr || !(*(MTX_GRD**)ptr))
    {
        mutex_guard_errno = MTX_GRD_ERR_NULL_MTX_GRD;
        return;
    }

    MutexGuardDestroy(*(MTX_GRD**)ptr);
}

/*****************************************/