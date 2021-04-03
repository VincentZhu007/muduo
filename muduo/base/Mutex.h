// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include <muduo/base/CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail (int errnum,
                                  const char *file,
                                  unsigned int line,
                                  const char *function)
    __THROW __attribute__ ((__noreturn__));
__END_DECLS
#endif

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       if (__builtin_expect(errnum != 0, 0))    \
                         __assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})

#else  // CHECK_PTHREAD_RETURN_VALUE

// 检查返回值是否为0
#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

#endif // CHECK_PTHREAD_RETURN_VALUE

namespace muduo
{

// Use as data member of a class, eg.
//
// class Foo
// {
//  public:
//   int size() const;
//
//  private:
//   mutable MutexLock mutex_;
//   std::vector<int> data_; // GUARDED BY mutex_
// };
/**
 * 比std::mutex多保存了锁的持有线程信息
 */
class MutexLock : boost::noncopyable
{
 public:
  MutexLock()
    : holder_(0)
  {
    MCHECK(pthread_mutex_init(&mutex_, NULL));
  }

  ~MutexLock()
  {
    assert(holder_ == 0); // 如果还有线程持有锁，则断言失败，表示代码逻辑有问题
    MCHECK(pthread_mutex_destroy(&mutex_));
  }

  /**
   * 判断当前线程是否已经持有锁。
   * 在本线程未持有锁时调用此函数没有并发问题。
   * a）调用此方法前未加锁，调用时正好先被其它线程加锁，tid被修改，但是不等于当前tid，判断正确
   * b）调用此方法前已被其它线程加锁，调用时正好其它线程释放锁，tid被改为0，不等于当前tid，判断正确。
   *
   * 是否要加volatile？不需要，volatile只能保证编译器不对单线程内代码进行乱序优化，没有多线程同步语义。
   * TODO：是否可以改为private?
   */
  // must be called when locked, i.e. for assertion
  bool isLockedByThisThread() const
  {
    return holder_ == CurrentThread::tid();
  }

  void assertLocked() const
  {
    assert(isLockedByThisThread());
  }

  // internal usage

  void lock()
  {
    MCHECK(pthread_mutex_lock(&mutex_));
    assignHolder();
  }

  void unlock()
  {
    unassignHolder();
    MCHECK(pthread_mutex_unlock(&mutex_));
  }

  /**
   * 将private对象暴露出去，没有必要，Condition作为友元类完全可以直接访问mutex_
   * @return
   */
  pthread_mutex_t* getPthreadMutex() /* non-const */
  {
    return &mutex_;
  }

 private:
  friend class Condition;

  class UnassignGuard : boost::noncopyable
  {
   public:
    UnassignGuard(MutexLock& owner)
      : owner_(owner)
    {
      owner_.unassignHolder();
    }

    ~UnassignGuard()
    {
      owner_.assignHolder();
    }

   private:
    MutexLock& owner_;
  };

  void unassignHolder()
  {
    holder_ = 0;
  }

  void assignHolder()
  {
    holder_ = CurrentThread::tid();
  }

  pthread_mutex_t mutex_;
  pid_t holder_; // 记录持有锁的线程ID
};

/**
 * RAII加锁和解锁类
 */
// Use as a stack variable, eg.
// int Foo::size() const
// {
//   MutexLockGuard lock(mutex_);
//   return data_.size();
// }
class MutexLockGuard : boost::noncopyable
{
 public:
  explicit MutexLockGuard(MutexLock& mutex)
    : mutex_(mutex)
  {
    mutex_.lock();
  }

  ~MutexLockGuard()
  {
    mutex_.unlock();
  }

 private:

  MutexLock& mutex_;
};

}

/**
 * 如果出现这种格式错误，编译器是否会告警？
 */
// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"

#endif  // MUDUO_BASE_MUTEX_H
