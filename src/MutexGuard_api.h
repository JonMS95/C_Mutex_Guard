#ifndef MUTEX_GUARD_API_H
#define MUTEX_GUARD_API_H

#ifdef __cplusplus
extern "C" {
#endif

/********** Include statements ***********/

#include <stdint.h>
#include <pthread.h>

/*****************************************/

/*********** Define statements ***********/

#define C_MUTEX_GUARD_API       __attribute__((visibility("default")))
#define C_MUTEX_GUARD_AINLINE   inline __attribute__((always_inline))
#define C_MUTEX_GUARD_NINLINE   __attribute__((noinline))
#define C_MUTEX_GUARD_ALIGNED   __attribute__((aligned(sizeof(size_t))))

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

/*****************************************/

/**************** Macros *****************/

#define MTX_GRD_CREATE(var_name) MTX_GRD var_name = {0}

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

/******* Public function prototypes ******/

/// @brief Returns Mutex Guard error code.
/// @return Currently stored error code
C_MUTEX_GUARD_API int MutexGuardGetErrorCode(void);

/// @brief Returns pointer to error describing string.
/// @param error_code Target error code to be described.
/// @return Pointer to error string.
C_MUTEX_GUARD_API const char* MutexGuardGetErrorString(const int error_code);

/// @brief Prints error.
/// @param custom_error_msg String to be copied to.
C_MUTEX_GUARD_API void MutexGuardPrintError(const char* custom_error_msg);

/// @brief Sets verbosity level.
/// @param target_verbosity_level Target verbosity level (silent, lock errors, backtrace or both). 
/// @return 0 if succeeded, < 0 if invalid verbosity level was provided.
C_MUTEX_GUARD_API int MutexGuardSetPrintStatus(const MTX_GRD_VERBOSITY_LEVEL target_verbosity_level);

/// @brief Initializeds mutex attribute.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param mutex_type Mutex type (NORMAL, ERRORCHECK, RECURSIVE, DEFAULT).
/// @param priority Mutex priority (NONE, INHERIT, PROTECT).
/// @param proc_sharing Share mutex with other processes (PRIVATE, SHARED).
/// @return 0 if succeeded, < 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardAttrInit(   MTX_GRD* restrict p_mutex_guard ,
                                            const int mutex_type            ,
                                            const int priority              ,
                                            const int proc_sharing          );

/// @brief MutexGuardAttrInit function wrapper.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param mutex_type Mutex type (NORMAL, ERRORCHECK, RECURSIVE, DEFAULT).
/// @param priority Mutex priority (NONE, INHERIT, PROTECT).
/// @param proc_sharing Share mutex with other processes (PRIVATE, SHARED).
/// @return Pointer to mutex guard structure if succeeded, NULL otherwise.
C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE MTX_GRD* MutexGuardAttrInitAddr(MTX_GRD* restrict p_mutex_guard ,
                                                                        const int mutex_type            , 
                                                                        const int priority              ,
                                                                        const int proc_sharing          ) {
    return (MutexGuardAttrInit(p_mutex_guard, mutex_type, priority, proc_sharing) ? NULL : p_mutex_guard);
}

/// @brief Initializes mutex.
/// @param p_mutex_guard Pointer to mutex containing mutex guard structure.
/// @return 0 if succeeded, != 0 otherwise.
C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE int MutexGuardInit(MTX_GRD* restrict p_mutex_guard) {
    return (p_mutex_guard ? pthread_mutex_init(&p_mutex_guard->mutex, &p_mutex_guard->mutex_attr) : -1);
}

/// @brief MutexGuardInit function wrapper.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @return Pointer to given mutex guard structure if succeeded, NULL otherwise.
C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE MTX_GRD* MutexGuardInitAddr(MTX_GRD* restrict p_mutex_guard) {
    return (MutexGuardInit(p_mutex_guard) ? NULL : p_mutex_guard);
}

/// @brief Gets lock error and copies it provided buffer.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param lock_error_string Buffer where the error is meant to eb copied to.
/// @param lock_error_str_size Buffer size.
/// @return 0 if succeeded, < 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardGetLockError(   const MTX_GRD* restrict p_mutex_guard   ,
                                                const uint64_t timeout_ns               ,
                                                char* lock_error_string                 ,
                                                const size_t lock_error_str_size        );

/// @brief Directly prints mutex lock error to standard output.
/// @param p_mutex_guard_acq_location Pointer to given mutex acquisition location.
/// @param target_mutex_addr Pointer to target mutex variable.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param ret_lock Value returned by pthread mutex locking function.
C_MUTEX_GUARD_API void MutexGuardPrintLockError(const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                                const pthread_mutex_t* restrict target_mutex_addr               ,
                                                const uint64_t timeout_ns                                       ,
                                                const int ret_lock                                              );

/// @brief Locks target mutex.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param address Address in which the current mutex is being tried to be locked.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param lock_type Lock type (TR_LOCK, LOCK, TIMED_LOCK, PERIODIC_TIMED_LOCK).
/// @return 0 if succeeded, != 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardLock(   MTX_GRD* p_mutex_guard      ,
                                        void* restrict address      ,
                                        const uint64_t timeout_ns   ,
                                        const int lock_type         );

/// @brief MutexGuardLock function wrapper.
/// @param p_mutex_guard Pointer to mutex guard structure.
/// @param address Address in which the current mutex is being tried to be locked.
/// @param timeout_ns Target timeout value (if any, in nanoseconds).
/// @param lock_type Lock type (TR_LOCK, LOCK, TIMED_LOCK, PERIODIC_TIMED_LOCK).
/// @return Pointer to given mutex guard structure if succeeded, NULL otherwise.
C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE MTX_GRD* MutexGuardLockAddr(MTX_GRD* restrict p_mutex_guard ,
                                                                    void* restrict address          ,
                                                                    const uint64_t timeout_ns       ,
                                                                    const int lock_type             ) {
    return (MutexGuardLock(p_mutex_guard, address, timeout_ns, lock_type) ? NULL : p_mutex_guard);
}

/// @brief Returns address within the program of line in which the current function was called. Meant to be used in macros.
/// @return Current function calling address.
C_MUTEX_GUARD_API C_MUTEX_GUARD_NINLINE void* MutexGuardGetFuncRetAddr(void);

/// @brief Unlocks target mutex.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, != 0 otherwise.
C_MUTEX_GUARD_API int MutexGuardUnlock(MTX_GRD* restrict p_mtx_grd);

/// @brief Destroys mutex attribute within given mutex guard.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, > 0 otherwise.
C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE int MutexGuardAttrDestroy(MTX_GRD* restrict p_mtx_grd) {
    return pthread_mutexattr_destroy(&p_mtx_grd->mutex_attr);
}

/// @brief Destroys mutex within given mutex guard.
/// @param p_mtx_grd Pointer to mutex guard structure.
/// @return 0 if succeeded, > 0 otherwise.
C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE int MutexGuardDestroy(MTX_GRD* restrict p_mtx_grd) {
    return pthread_mutex_destroy(&p_mtx_grd->mutex);
}

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