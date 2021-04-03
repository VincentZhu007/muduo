// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
/**
 * 封装条件变量
 * 作为MutexLock的友元类，可以访问 pthread_mutex_t 变量
 */
class Condition : boost::noncopyable
{
 public:
  explicit Condition(MutexLock& mutex)
    : mutex_(mutex)
  {
    MCHECK(pthread_cond_init(&pcond_, NULL));
  }

  ~Condition()
  {
    MCHECK(pthread_cond_destroy(&pcond_));
  }

  void wait()
  {
    /**
     * 这里有一个问题：pthread_cond_wait只会获取pthread_mutex原始锁，感知不到
     * MutexLock，需要从成员变量范围this对象。
     * 作者使用了一个内部类UnassignGuard来记录外部对象owner_，从而实现由成员对象访问
     * 外层this对象的目的。下面首先使用UnassignGuard来清除holder_线程信息。那条件变量
     * 能否记录锁持有者信息呢？
     */
    MutexLock::UnassignGuard ug(mutex_);
    MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
  }

  // returns true if time out, false otherwise.
  bool waitForSeconds(int seconds);

  void notify()
  {
    MCHECK(pthread_cond_signal(&pcond_));
  }

  void notifyAll()
  {
    MCHECK(pthread_cond_broadcast(&pcond_));
  }

 private:
  MutexLock& mutex_;
  pthread_cond_t pcond_;
};

}
#endif  // MUDUO_BASE_CONDITION_H
