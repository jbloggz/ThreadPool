# ThreadPool

A C++20 thread pool implementation. Creates a pool of threads that will execute tasks added to the queue. A task can be any callable object (eg. function, lambda, function object). The queue is a FIFO (First in, First Out), so tasks will be processed in the same order that they are added. When the ThreadPool is destroyed, it will block until all remaining tasks in the queue have finished processing.

## Installation

The entire library is contained within the `ThreadPool.h` file. On \*nix systems, run `make install` from the ThreadPool directory to install the header file to `/usr/local/include`. On other systems, you can manually install the header file to the necessary directory.
Then add the following to any code that needs to use the ThreadPool:

```c++
#include <ThreadPool.h>
```

## Examples

### Basic usage

The most basic usage is to create a ThreadPool object and send it a task to run. The destructor of the ThreadPool will ensure that the task is finished before the object is destroyed.

```c++
#include <iostream>
#include <ThreadPool.h>

int main(int argc, char *argv[]) {
   // Create a thread pool of 4 threads
   threadpool::ThreadPool tp(4);

   // Add a task to the queue
   tp.addJob([]() { std::cout << "Hello!" << std::endl; });
}
```

Output:

```
Hello!
```

### Multiple tasks

You can add any number of tasks to the queue.

```c++
#include <iostream>
#include <ThreadPool.h>

int main(int argc, char *argv[]) {
   threadpool::ThreadPool tp(3);

   for (int i = 0; i < 5; ++i) {
      tp.addJob([]() {
         std::cout << "Hello from " << std::this_thread::get_id() << std::endl;
      });
   }
}
```

Output:

```
Hello from 7fa71134f700
Hello from Hello from 7fa71134f700
7fa710b4e700
Hello from 7fa711b50700
Hello from 7fa71134f700
Hello from 7fa711b50700
```

While this example does work, it's probably not what we want. Remember to use something like a mutex to synchronise threads when necessary:

```c++
#include <iostream>
#include <mutex>
#include <ThreadPool.h>

int main(int argc, char *argv[]) {
   threadpool::ThreadPool tp(3);
   std::mutex             mtx;

   for (int i = 0; i < 5; ++i) {
      tp.addJob([&mtx]() {
         std::scoped_lock lk(mtx);
         std::cout << "Hello from " << std::this_thread::get_id() << std::endl;
      });
   }
}
```

Output:

```
Hello from 140706618689280
Hello from 140706610296576
Hello from 140706627081984
Hello from 140706618689280
Hello from 140706610296576
```

### Passing arguments

For tasks that require arguments, you can pass these to `addJob` as well:

```c++
#include <iostream>
#include <mutex>
#include <ThreadPool.h>

int main(int argc, char *argv[]) {
   threadpool::ThreadPool tp(3);
   std::mutex             mtx;

   auto fn = [&mtx](int val) {
      std::scoped_lock lk(mtx);
      std::cout << "You passed " << val << std::endl;
   };

   for (int i = 0; i < 5; ++i) {
      tp.addJob(fn, i);
   }
}
```

Output:

```
You passed 0
You passed 1
You passed 2
You passed 3
You passed 4
```

### Return value

If you want to access the return value of a task (or simply check if a task has finished), `addJob` will return a `std::future` that you can use for this purpose.

```c++
#include <iostream>
#include <ThreadPool.h>

int main(int argc, char *argv[]) {
   threadpool::ThreadPool tp(4);

   auto res = tp.addJob([](int val) { return val * 2; }, 12);

   std::cout << res.get() << std::endl;
}
```

Output:

```
24
```

### Callable

The ThreadPool can work with any callable type in C++.

```c++
#include <iostream>
#include <ThreadPool.h>

int increment(int a) { return a + 1; }

class MyClass {
  private:
   int val;

  public:
   MyClass(int val) : val(val) {}
   int operator()() const { return val / 2; }
   int multiply(int v) { return v * val; }
};


int main(int argc, char *argv[]) {
   threadpool::ThreadPool tp(4);

   // A free function
   auto res1 = tp.addJob(increment, 6);
   std::cout << res1.get() << std::endl;

   // A lambda
   auto res2 = tp.addJob([](int a) { return a - 1; }, 6);
   std::cout << res2.get() << std::endl;

   // A function object
   MyClass fo(6);
   auto    res3 = tp.addJob(fo);
   std::cout << res3.get() << std::endl;

   // A member function
   auto res4 = tp.addJob(&MyClass::multiply, &fo, 2);
   std::cout << res4.get() << std::endl;
}
```

Output:

```
7
5
3
12
```
