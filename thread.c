#ifndef _THREADS_H
#define _THREADS_H
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <stdint.h>
#if !(defined(__linux__) || defined(_WIN32) || defined(__CYGWIN__))
  #define C11THREADS_NO_TIMED_MUTEX
#endif
#ifdef C11THREADS_NO_TIMED_MUTEX
  #include <sys/time.h>
  #define PTHREAD_MUTEX_TIMED_NP PTHREAD_MUTEX_NORMAL
#endif
#define ONCE_FLAG_INIT  PTHREAD_ONCE_INIT
#define TSS_DTOR_ITERATIONS 4
typedef pthread_t thrd_t;
typedef pthread_mutex_t mtx_t;
typedef pthread_cond_t cnd_t;
typedef pthread_key_t tss_t;
typedef pthread_once_t once_flag;
typedef int (*thrd_start_t)(void*);
typedef void (*tss_dtor_t)(void*);
enum {
  mtx_plain = 0,
  mtx_recursive = 1,
  mtx_timed = 2,
};
enum {
  thrd_success,
  thrd_timedout,
  thrd_busy,
  thrd_error,
  thrd_nomem
};
static inline int thrd_err_map(int errcode) {
  switch(errcode) {
    case 0:
      return thrd_success;
    case ENOMEM:
      return thrd_nomem;
    case ETIMEDOUT:
      return thrd_timedout;
    case EBUSY:
      return thrd_busy;
  }
  return thrd_error;
}
static inline int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
  return thrd_err_map(pthread_create(thr, 0, (void*(*)(void*))(void*)func, arg));
}
static inline int thrd_equal(thrd_t a, thrd_t b) {
  return pthread_equal(a, b);
}
static inline thrd_t thrd_current(void) {
  return pthread_self();
}
static inline int thrd_sleep(const struct timespec *ts_in, struct timespec *rem_out) {
#ifdef _POSIX_TIMERS
  return nanosleep(ts_in, rem_out) == 0 ? 0 : (errno == EINTR ? -1 : -2);
#else
  if(rem_out) {
    rem_out.tv_sec = 0;
    rem_out.tv_nsec = 0;
  }
  return usleep(ts_in.tv_sec*1000000 + ts_in.tv_nsec/1000) == 0 ? 0 : (errno == EINTR ? -1 : -2);
#endif
}
static inline void thrd_exit(int res) {
  pthread_exit((void*)(uintptr_t)res);
}
static inline int thrd_detach(thrd_t thr) {
  return thrd_err_map(pthread_detach(thr));
}
static inline int thrd_join(thrd_t thr, int *res) {
  void *retval;
  int errcode = pthread_join(thr, &retval);
  if(res) {
    *res = (int)(uintptr_t)retval;
  }
  return thrd_err_map(errcode);
}
static inline void thrd_yield(void) {
  sched_yield();
}
static inline int mtx_init(mtx_t *mtx, int type) {
  int errcode;
  pthread_mutexattr_t attr;
  errcode = pthread_mutexattr_init(&attr);
  if(type & mtx_timed) {
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);
  }
  if(type & mtx_recursive) {
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  }
  errcode = pthread_mutex_init(mtx, &attr);
  pthread_mutexattr_destroy(&attr);
  return thrd_err_map(errcode);
}
static inline int mtx_lock(mtx_t *mtx) {
  return thrd_err_map(pthread_mutex_lock(mtx));
}
static inline int mtx_timedlock(mtx_t *mtx, const struct timespec *ts) {
  int errcode;
#ifdef C11THREADS_NO_TIMED_MUTEX
  struct timeval now;
  struct timespec sleeptime;
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 5000000;
  while((errcode = pthread_mutex_trylock(mtx)) == EBUSY) {
    gettimeofday(&now, NULL);
    if(now.tv_sec > ts->tv_sec || (now.tv_sec == ts->tv_sec && (now.tv_usec * 1000) >= ts->tv_nsec)) {
      return thrd_timedout;
    }
    nanosleep(&sleeptime, NULL);
  }
#else
  errcode = pthread_mutex_timedlock(mtx, ts);
#endif
  return thrd_err_map(errcode);
}
static inline int mtx_trylock(mtx_t *mtx) {
  return thrd_err_map(pthread_mutex_trylock(mtx));
}
static inline int mtx_unlock(mtx_t *mtx) {
  return thrd_err_map(pthread_mutex_unlock(mtx));
}
static inline void mtx_destroy(mtx_t *mtx) {
  pthread_mutex_destroy(mtx);
}
static inline int cnd_init(cnd_t *cond) {
  return thrd_err_map(pthread_cond_init(cond, 0));
}
static inline int cnd_signal(cnd_t *cond) {
  return thrd_err_map(pthread_cond_signal(cond));
}
static inline int cnd_broadcast(cnd_t *cond) {
  return thrd_err_map(pthread_cond_broadcast(cond));
}
static inline int cnd_wait(cnd_t *cond, mtx_t *mtx) {
  return thrd_err_map(pthread_cond_wait(cond, mtx));
}
static inline int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts) {
  return thrd_err_map(pthread_cond_timedwait(cond, mtx, ts));
}
static inline void cnd_destroy(cnd_t *cond) {
  pthread_cond_destroy(cond);
}
static inline int tss_create(tss_t *key, tss_dtor_t dtor) {
  return thrd_err_map(pthread_key_create(key, dtor));
}
static inline void *tss_get(tss_t key) {
  return pthread_getspecific(key);
}
static inline int tss_set(tss_t key, void *val){
  return thrd_err_map(pthread_setspecific(key, val));
}
static inline void tss_delete(tss_t key) {
  pthread_key_delete(key);
}
static inline void call_once(once_flag *flag, void (*func)(void)) {
  pthread_once(flag, func);
}
#endif