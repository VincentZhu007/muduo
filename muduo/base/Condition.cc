// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Condition.h>

#include <errno.h>
#include <sys/time.h>

/**
 * wait增加超时唤醒功能
 * 唤醒之后线程是已经获取了吗？
 * 时间变量翻转如何处理？ gettimeofday?
 * @param seconds
 * @return
 */
// returns true if time out, false otherwise.
bool muduo::Condition::waitForSeconds(int seconds)
{
  struct timespec abstime;
#ifdef CLOCK_REALTIME
  // FIXME: use CLOCK_MONOTONIC or CLOCK_MONOTONIC_RAW to prevent time rewind.
  clock_gettime(CLOCK_REALTIME, &abstime);
#else  // Mac OS X
  struct timeval tv;
  gettimeofday(&tv, NULL);
  abstime.tv_sec = tv.tv_sec;
  abstime.tv_nsec = tv.tv_usec * 1000;
#endif
  abstime.tv_sec += seconds;
  MutexLock::UnassignGuard ug(mutex_);
  return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}

