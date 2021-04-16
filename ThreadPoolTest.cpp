#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include "ThreadPool.h"

int
gen_random_int()
{
   std::random_device              rd;
   std::default_random_engine      eng(rd());
   std::uniform_int_distribution<> rnum(1, 10);
   return rnum(eng);
}

int
myFunc(int a)
{
   return a + 1;
}

class MyFunctObj {
  private:
   int val;

  public:
   MyFunctObj(int val) : val(val) {}

   int
   operator()() const
   {
      return val + 1;
   }

   int
   getmultiple(int v)
   {
      return v * val;
   }
};

TEST(ThreadPool, CreateThreadpool)
{
   int                    nthreads = gen_random_int();
   threadpool::ThreadPool tp(nthreads);

   EXPECT_EQ(nthreads, tp.threadCount());
   EXPECT_EQ(0, tp.activeCount());
   EXPECT_EQ(0, tp.queuedCount());
}

TEST(ThreadPool, AddSimpleJob)
{
   threadpool::ThreadPool tp(6);

   EXPECT_EQ(6, tp.threadCount());
   EXPECT_EQ(0, tp.activeCount());

   tp.addJob([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });

   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   EXPECT_EQ(0, tp.queuedCount());
   EXPECT_EQ(1, tp.activeCount());
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   EXPECT_EQ(0, tp.activeCount());
}

TEST(ThreadPool, AddMultipleJobs)
{
   int                    nthreads = 3;
   threadpool::ThreadPool tp(nthreads);

   EXPECT_EQ(nthreads, tp.threadCount());
   EXPECT_EQ(0, tp.activeCount());

   for (int i = 0; i < nthreads; i++) {
      tp.addJob([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   EXPECT_EQ(0, tp.queuedCount());
   EXPECT_EQ(3, tp.activeCount());
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   EXPECT_EQ(0, tp.activeCount());
}

TEST(ThreadPool, AddQueuedJobs)
{
   int                    nthreads = 3;
   std::atomic<int>       count    = 0;
   threadpool::ThreadPool tp(nthreads);

   EXPECT_EQ(nthreads, tp.threadCount());
   EXPECT_EQ(0, tp.activeCount());

   for (int i = 0; i < 7; i++) {
      tp.addJob([&]() {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         count++;
      });
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   EXPECT_EQ(0, count);
   EXPECT_EQ(4, tp.queuedCount());
   EXPECT_EQ(3, tp.activeCount());
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   EXPECT_EQ(3, count);
   EXPECT_EQ(1, tp.queuedCount());
   EXPECT_EQ(3, tp.activeCount());
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   EXPECT_EQ(6, count);
   EXPECT_EQ(0, tp.queuedCount());
   EXPECT_EQ(1, tp.activeCount());
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   EXPECT_EQ(7, count);
   EXPECT_EQ(0, tp.queuedCount());
   EXPECT_EQ(0, tp.activeCount());
}

TEST(ThreadPool, FinishQueuedJobs)
{
   int              nthreads = 3;
   std::atomic<int> count    = 0;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      for (int i = 0; i < 7; i++) {
         tp.addJob([&count]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            count++;
         });
      }
   }

   EXPECT_EQ(7, count);
}

TEST(ThreadPool, TerminateRemainingJobs)
{
   int              nthreads = 3;
   std::atomic<int> count    = 0;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      for (int i = 0; i < 20; i++) {
         tp.addJob([&count]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            count++;
         });
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(250));
      tp.clearQueue();
   }

   EXPECT_EQ(9, count);
}

TEST(ThreadPool, JobWithArgs)
{
   int              nthreads = 8;
   std::atomic<int> count    = 0;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      tp.addJob([&count](int a, int b) { count = a + b; }, 7, 5);
   }

   EXPECT_EQ(count, 12);
}

TEST(ThreadPool, JobWithRefArgs)
{
   int nthreads = 8;
   int count    = 0;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      tp.addJob([](int& a) { a = 15; }, std::ref(count));
   }

   EXPECT_EQ(count, 15);
}

TEST(ThreadPool, JobWithReturnValue)
{
   int              nthreads = 8;
   int              count    = 16;
   std::future<int> ft;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      ft = tp.addJob([](int& a) { return a + 1; }, std::ref(count));
   }

   int val = ft.get();

   EXPECT_EQ(val, count + 1);
}

TEST(ThreadPool, CallStandardFunction)
{
   int nthreads = 8;
   int val      = gen_random_int();

   std::future<int> ft;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      ft = tp.addJob(myFunc, val);
   }

   int res = ft.get();

   EXPECT_EQ(res, val + 1);
}

TEST(ThreadPool, CallFunctionPointer)
{
   int nthreads   = 8;
   int (*fn)(int) = myFunc;
   int val        = gen_random_int();

   std::future<int> ft;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      ft = tp.addJob(fn, val);
   }

   int res = ft.get();

   EXPECT_EQ(res, val + 1);
}

TEST(ThreadPool, CallFunctionObject)
{
   int        nthreads = 8;
   int        val      = gen_random_int();
   MyFunctObj obj(val);

   std::future<int> ft;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      ft = tp.addJob(obj);
   }

   int res = ft.get();

   EXPECT_EQ(res, val + 1);
}

TEST(ThreadPool, CallObjectMember)
{
   int        nthreads = 8;
   int        val      = gen_random_int();
   MyFunctObj obj(val);

   std::future<int> ft;
   {
      threadpool::ThreadPool tp(nthreads);

      EXPECT_EQ(nthreads, tp.threadCount());
      EXPECT_EQ(0, tp.activeCount());

      ft = tp.addJob(&MyFunctObj::getmultiple, &obj, 3);
   }

   int res = ft.get();

   EXPECT_EQ(res, val * 3);
}
