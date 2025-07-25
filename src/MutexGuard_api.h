#ifndef MUTEX_GUARD_API_H
#define MUTEX_GUARD_API_H

#ifdef __cplusplus
extern "C" {
#endif

/********** Include statements ***********/

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

/*****************************************/

/*********** Define statements ***********/

#define C_MUTEX_GUARD_API                   __attribute__((visibility("default")))
#define C_MUTEX_GUARD_NOINLINE              __attribute__((noinline))
#define C_MUTEX_GUARD_ALIGNED               __attribute__((aligned(sizeof(size_t))))
#define C_MUTEX_GUARD_DESTROY_ATTR_CLEANUP  __attribute__((cleanup(MutexGuardDestroyAttrCleanup)))
#define C_MUTEX_GUARD_DESTROY_CLEANUP       __attribute__((cleanup(MutexGuardDestroyMutexCleanup)))
#define C_MUTEX_GUARD_UNLOCK_CLEANUP        __attribute__((cleanup(MutexGuardReleaseMutexCleanup)))

#ifdef __cplusplus
#define C_MUTEX_GUARD_RESTRICT
#else
#define C_MUTEX_GUARD_RESTRICT  restrict
#endif

#ifndef __MTX_GRD_ADDR_NUM__
#define __MTX_GRD_ADDR_NUM__    10
#endif

/******* Private type definitions ********/

/// @brief Structure holding addresses in which target mutex was locked as well as locking thread's ID.
typedef struct C_MUTEX_GUARD_ALIGNED
{
    void*           addresses[__MTX_GRD_ADDR_NUM__];
    pthread_t       thread_id;
} MTX_GRD_ACQ_LOCATION;

/// @brief Mutex guard (module's main struct). Holds mutex to be locked/unlocked as well as attributes, locking data, and a free-use pointer.
typedef struct C_MUTEX_GUARD_ALIGNED
{
    pthread_mutex_t         mutex;
    pthread_mutexattr_t     mutex_attr;
    MTX_GRD_ACQ_LOCATION    mutex_acq_location;
    unsigned long long      lock_counter;
    pthread_mutex_t         ctrl_mutex;
    void*                   additional_data;
} MTX_GRD;

/// @brief Lock types (to be used with MutexGuardLock).
typedef enum
{
    MTX_GRD_LOCK_TYPE_TRY       = 0                         ,
    MTX_GRD_LOCK_TYPE_PERMANENT                             ,
    MTX_GRD_LOCK_TYPE_TIMED                                 ,
    MTX_GRD_LOCK_TYPE_PERIODIC                              ,
    MTX_GRD_LOCK_TYPE_MIN       = MTX_GRD_LOCK_TYPE_TRY     ,
    MTX_GRD_LOCK_TYPE_MAX       = MTX_GRD_LOCK_TYPE_PERIODIC,
} MTX_GRD_LOCK_TYPES;

/// @brief Available verbosity levels (silent, print lock errors, display whole backtrace or both).
typedef enum
{
    MTX_GRD_VERBOSITY_SILENT    = 0                         ,
    MTX_GRD_VERBOSITY_LOCK_ERROR                            ,
    MTX_GRD_VERBOSITY_BT                                    ,
    MTX_GRD_VERBOSITY_ALL                                   , // Both lock errors as well as lock/unlock backtraces are printed.
    MTX_GRD_VERBOSITY_MIN       = MTX_GRD_VERBOSITY_SILENT  ,
    MTX_GRD_VERBOSITY_MAX       = MTX_GRD_VERBOSITY_ALL     ,
} MTX_GRD_VERBOSITY_LEVEL;

/// @brief Available internal error management modes.
typedef enum
{
    MTX_GRD_INT_ERR_MGMT_KEEP_TRYING    = 0                                     ,
    MTX_GRD_INT_ERR_MGMT_ABORT_ON_ERROR                                         ,
    MTX_GRD_INT_ERR_MGMT_FORCE_ONE_SHOT                                         ,
    MTX_GRD_INT_ERR_MGMT_MIN            = MTX_GRD_INT_ERR_MGMT_KEEP_TRYING      ,
    MTX_GRD_INT_ERR_MGMT_MAX            = MTX_GRD_INT_ERR_MGMT_FORCE_ONE_SHOT   ,
} MTX_GRD_INT_ERR_MGMT;

/*****************************************/

/**************** Macros *****************/

/************* Init Macros ***************/

/// @brief Creates empty MTX_GRD variable.
#define MTX_GRD_CREATE(var_name) MTX_GRD var_name = {0}

/// @brief Initializes Mutex Guard attribute with given parameters (MTX_GRD pointer, mutex type, mutex priority and mutex process sharing).
#define MTX_GRD_ATTR_INIT(p_mtx_grd, mutex_type, priority, proc_sharing) (MutexGuardAttrInit(p_mtx_grd, mutex_type, priority, proc_sharing))

/// @brief Initializes Mutex Guard for a given MTX_GRD pointer.
#define MTX_GRD_INIT(p_mtx_grd) MutexGuardInit(p_mtx_grd)

/// @brief Initializes Mutex Guard attributes for a given MTX_GRD pointer constraining its lifetime to the current scope.
#define MTX_GRD_ATTR_INIT_SC(p_mtx_grd, mutex_type, priority, proc_sharing, cleanup_var_name) MTX_GRD* cleanup_var_name C_MUTEX_GUARD_DESTROY_ATTR_CLEANUP = (MutexGuardAttrInitAddr(p_mtx_grd, mutex_type, priority, proc_sharing))

/// @brief Initializes Mutex Guard for a given MTX_GRD pointer constraining its lifetime to the current scope.
#define MTX_GRD_INIT_SC(p_mtx_grd, cleanup_var_name) MTX_GRD* cleanup_var_name C_MUTEX_GUARD_DESTROY_CLEANUP = (MutexGuardInitAddr(p_mtx_grd))

/************* Lock macros ***************/

/// @brief Tries to lock mutex pointed by given MTX_GRD pointer and provides lock address automatically.
#define MTX_GRD_TRY_LOCK(p_mtx_grd)                 MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_TRY)

/// @brief Locks mutex (not try, but normal mutex lock attempt instead) pointed by given MTX_GRD pointer and provides lock address automatically.
#define MTX_GRD_LOCK(p_mtx_grd)                     MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_PERMANENT)

/// @brief Tries to lock mutex pointed by given MTX_GRD pointer within a given time span (in nanoseoconds) and provides lock address automatically.
#define MTX_GRD_TIMED_LOCK(p_mtx_grd, tout_ns)      MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_TIMED)

/// @brief Tries to lock periodically mutex pointed by given MTX_GRD pointer with a given period (in nanoseoconds) and provides lock address automatically.
#define MTX_GRD_PERIODIC_LOCK(p_mtx_grd, tout_ns)   MutexGuardLock((p_mtx_grd), MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_PERIODIC)

/********** Scoped lock macros ***********/

/// @brief Tries to lock mutex pointed by given MTX_GRD pointer and provides lock address automatically. It ensures mutex unlock just before the current scope is exited.
#define MTX_GRD_TRY_LOCK_SC(p_mtx_grd, cleanup_var_name)                MTX_GRD* cleanup_var_name C_MUTEX_GUARD_UNLOCK_CLEANUP = (MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_TRY))

/// @brief Locks mutex (not try, but normal mutex lock attempt instead) pointed by given MTX_GRD pointer and provides lock address automatically. It ensures mutex unlock just before the current scope is exited.
#define MTX_GRD_LOCK_SC(p_mtx_grd, cleanup_var_name)                    MTX_GRD* cleanup_var_name C_MUTEX_GUARD_UNLOCK_CLEANUP = (MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), 0, MTX_GRD_LOCK_TYPE_PERMANENT))

/// @brief Tries to lock mutex pointed by given MTX_GRD pointer within a given time span (in nanoseoconds) and provides lock address automatically. It ensures mutex unlock just before the current scope is exited.
#define MTX_GRD_TIMED_LOCK_SC(p_mtx_grd, tout_ns, cleanup_var_name)     MTX_GRD* cleanup_var_name C_MUTEX_GUARD_UNLOCK_CLEANUP = (MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_TIMED))

/// @brief Tries to lock periodically mutex pointed by given MTX_GRD pointer with a given period (in nanoseoconds) and provides lock address automatically. It ensures mutex unlock just before the current scope is exited.
#define MTX_GRD_PERIODIC_LOCK_SC(p_mtx_grd, tout_ns, cleanup_var_name)  MTX_GRD* cleanup_var_name C_MUTEX_GUARD_UNLOCK_CLEANUP = (MutexGuardLockAddr(p_mtx_grd, MutexGuardGetFuncRetAddr(), tout_ns, MTX_GRD_LOCK_TYPE_PERIODIC))

/************ Unlock macros **************/

/// @brief Unlocks mutex pointed by given MTX_GRD pointer.
#define MTX_GRD_UNLOCK(p_mtx_grd)       MutexGuardUnlock(p_mtx_grd)

/************ Destroy macros *************/

/// @brief Destroys mutex pointed by given MTX_GRD pointer.
#define MTX_GRD_DESTROY(p_mtx_grd)      MutexGuardDestroy(p_mtx_grd)

/// @brief Destroys mutex attributes pointed by given MTX_GRD pointer.
#define MTX_GRD_ATTR_DESTROY(p_mtx_grd) MutexGuardAttrDestroy(p_mtx_grd)

/********* Error message macros **********/

/// @brief Retrieves string associated to latest error code.
#define MTX_GRD_GET_LAST_ERR_STR    MutexGuardGetErrorString(MutexGuardGetErrorCode())

/*****************************************/

/******* Public function prototypes ******/

/// @brief Returns Mutex Guard error code.
/// @return Currently stored error code.
/// @warning Stores last error code (same as errno), even if no error happened lately. Use with care.
C_MUTEX_GUARD_API int MutexGuardGetErrorCode(void);

/// @brief Returns pointer to error describing string.
/// @param error_code Target error code to be described.
/// @return Pointer to error string.
/// @warning As last error code is stored each time, so a string may be returned even if no error happened lately. Use with care.
C_MUTEX_GUARD_API const char* MutexGuardGetErrorString(const int error_code);

/// @brief Prints error.
/// @param custom_error_msg String to be copied to.
C_MUTEX_GUARD_API void MutexGuardPrintError(const char* C_MUTEX_GUARD_RESTRICT custom_error_msg);

/// @brief Establishes how should the program behave on internal mutex management error.
/// @param mode Target mode (check available values on MTX_GRD_INT_ERR_MGMT).
C_MUTEX_GUARD_API int MutexGuardSetInternalErrMode(const MTX_GRD_INT_ERR_MGMT mgmt_mode);

/// @brief Retrieves current internal error management mode.
/// @return Currently assigned internal error management mode.
C_MUTEX_GUARD_API MTX_GRD_INT_ERR_MGMT MutexGuardGetInternalErrMode(void);

/// @brief Sets verbosity level.
/// @param target_verbosity_level Target verbosity level (silent, lock errors, backtrace or both). 
/// @return 0 if succeeded, < 0 if invalid verbosity level was provided.
C_MUTEX_GUARD_API int MutexGuardSetPrintStatus(const MTX_GRD_VERBOSITY_LEVEL target_verbosity_level);

/// @brief Gets verbosity level.
/// @return Currently assigned verbosity level.
C_MUTEX_GUARD_API MTX_GRD_VERBOSITY_LEVEL MutexGuardGetPrintStatus(void);

/// @brief Initializeds mutex attribute.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param mutex_type Mutex type (NORMAL, ERRORCHECK, RECURSIVE, DEFAULT).
/// @param priority Mutex priority (NONE, INHERIT, PROTECT).
/// @param proc_sharing Share mutex with other processes (PRIVATE, SHARED).
/// @return 0 if succeeded, < 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardAttrInit(   MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mutex_guard   ,
                                            const int mutex_type                            ,
                                            const int priority                              ,
                                            const int proc_sharing                          );

/// @brief MutexGuardAttrInit function wrapper.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param mutex_type Mutex type (NORMAL, ERRORCHECK, RECURSIVE, DEFAULT).
/// @param priority Mutex priority (NONE, INHERIT, PROTECT).
/// @param proc_sharing Share mutex with other processes (PRIVATE, SHARED).
/// @return Pointer to mutex guard structure if succeeded, NULL otherwise.
C_MUTEX_GUARD_API MTX_GRD* MutexGuardAttrInitAddr(  MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mutex_guard   ,
                                                    const int mutex_type                            , 
                                                    const int priority                              ,
                                                    const int proc_sharing                          );

/// @brief Initializes mutex.
/// @param p_mutex_guard Pointer to mutex containing mutex guard structure.
/// @return 0 if succeeded, != 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardInit(MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mutex_guard);

/// @brief MutexGuardInit function wrapper.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @return Pointer to given mutex guard structure if succeeded, NULL otherwise.
C_MUTEX_GUARD_API MTX_GRD* MutexGuardInitAddr(MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mutex_guard);

/// @brief Locks target mutex.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param address Address in which the current mutex is being tried to be locked.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param lock_type Lock type (TR_LOCK, LOCK, TIMED_LOCK, PERIODIC_TIMED_LOCK).
/// @return 0 if succeeded, != 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardLock(   MTX_GRD* p_mutex_guard              ,
                                        void* C_MUTEX_GUARD_RESTRICT address,
                                        const uint64_t timeout_ns           ,
                                        const int lock_type                 );

/// @brief MutexGuardLock function wrapper.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param address Address in which the current mutex is being tried to be locked.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param lock_type Lock type (TR_LOCK, LOCK, TIMED_LOCK, PERIODIC_TIMED_LOCK).
/// @return Pointer to given mutex guard structure if succeeded, NULL otherwise.
C_MUTEX_GUARD_API MTX_GRD* MutexGuardLockAddr(  MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mutex_guard   ,
                                                void* C_MUTEX_GUARD_RESTRICT address            ,
                                                const uint64_t timeout_ns                       ,
                                                const int lock_type                             );

/// @brief Returns address within the program of line in which the current function was called. Meant to be used in macros.
/// @return Current function calling address.
C_MUTEX_GUARD_API C_MUTEX_GUARD_NOINLINE void* MutexGuardGetFuncRetAddr(void);

/// @brief Unlocks target mutex.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, != 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardUnlock(MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mtx_grd);

/// @brief Destroys mutex attribute within given mutex guard.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, > 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardAttrDestroy(MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mtx_grd);

/// @brief Destroys mutex within given mutex guard.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, > 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardDestroy(MTX_GRD* C_MUTEX_GUARD_RESTRICT p_mtx_grd);

/// @brief Cleanup function to release a mutex (meant to be used alongside scoped mutex lock macros).
/// @param ptr Pointer to mutex guard structure. 
C_MUTEX_GUARD_API void MutexGuardReleaseMutexCleanup(void* ptr);

/// @brief Cleanup function to destroy a mutex attribute (meant to be used alongside scoped attribute init macros).
/// @param ptr Pointer to mutex attribute containing mutex guard structure. 
C_MUTEX_GUARD_API void MutexGuardDestroyAttrCleanup(void* ptr);

/// @brief Cleanup function to destroy a mutex (meant to be used alongside scoped mutex init macros)
/// @param ptr Pointer to mutex guard structure.
C_MUTEX_GUARD_API void MutexGuardDestroyMutexCleanup(void* ptr);

/*****************************************/

#ifdef __cplusplus
}
#endif

#endif