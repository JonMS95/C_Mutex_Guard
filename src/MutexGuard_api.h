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

#ifndef __MTX_GRD_ADDR_NUM__
#define __MTX_GRD_ADDR_NUM__    10
#endif

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

C_MUTEX_GUARD_API int MutexGuardGetErrorCode(void);

C_MUTEX_GUARD_API const char* MutexGuardGetErrorString(const int error_code);

C_MUTEX_GUARD_API void MutexGuardPrintError(const char* custom_error_msg);

C_MUTEX_GUARD_API int MutexGuardSetPrintStatus(const MTX_GRD_VERBOSITY_LEVEL target_verbosity_level);

C_MUTEX_GUARD_API int MutexGuardAttrInit(   MTX_GRD* restrict p_mutex_guard ,
                                            const int mutex_type            ,
                                            const int priority              ,
                                            const int proc_sharing          );

C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE MTX_GRD* MutexGuardAttrInitAddr(MTX_GRD* restrict p_mutex_guard ,
                                                                        const int mutex_type            , 
                                                                        const int priority              ,
                                                                        const int proc_sharing          ) {
    return (MutexGuardAttrInit(p_mutex_guard, mutex_type, priority, proc_sharing) ? NULL : p_mutex_guard);
}

C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE int MutexGuardInit(MTX_GRD* restrict p_mutex_guard) {
    return (p_mutex_guard ? pthread_mutex_init(&p_mutex_guard->mutex, &p_mutex_guard->mutex_attr) : -1);
}

C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE MTX_GRD* MutexGuardInitAddr(MTX_GRD* restrict p_mutex_guard) {
    return (MutexGuardInit(p_mutex_guard) ? NULL : p_mutex_guard);
}

C_MUTEX_GUARD_API void MutexGuardPrintLockError(const MTX_GRD_ACQ_LOCATION* restrict p_mutex_guard_acq_location ,
                                                const pthread_mutex_t* restrict target_mutex_addr               ,
                                                const uint64_t timeout_ns                                       ,
                                                const int ret_lock                                              );

C_MUTEX_GUARD_API int MutexGuardLock(   MTX_GRD* p_mutex_guard      ,
                                        void* restrict address      ,
                                        const uint64_t timeout_ns   ,
                                        const int lock_type         );

C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE MTX_GRD* MutexGuardLockAddr(MTX_GRD* restrict p_mutex_guard ,
                                                            void* restrict address          ,
                                                            const uint64_t timeout_ns       ,
                                                            const int lock_type             ) {
    return (MutexGuardLock(p_mutex_guard, address, timeout_ns, lock_type) ? NULL : p_mutex_guard);
}

C_MUTEX_GUARD_API C_MUTEX_GUARD_NINLINE void* MutexGuardGetFuncRetAddr(void);

C_MUTEX_GUARD_API int MutexGuardUnlock(MTX_GRD* restrict p_mtx_grd);

C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE int MutexGuardAttrDestroy(MTX_GRD* restrict p_mtx_grd) {
    return pthread_mutexattr_destroy(&p_mtx_grd->mutex_attr);
}

C_MUTEX_GUARD_API C_MUTEX_GUARD_AINLINE int MutexGuardDestroy(MTX_GRD* restrict p_mtx_grd) {
    return pthread_mutex_destroy(&p_mtx_grd->mutex);
}

C_MUTEX_GUARD_API void MutexGuardReleaseMutexCleanup(void* ptr);
C_MUTEX_GUARD_API void MutexGuardDestroyAttrCleanup(void* ptr);
C_MUTEX_GUARD_API void MutexGuardDestroyMutexCleanup(void* ptr);

/*****************************************/

#ifdef __cplusplus
}
#endif

#endif