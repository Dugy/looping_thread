# LoopingThread

Looping thread is a header-only library using only standard C++11 libraries providing a utility class that periodically calls a function and cleanly exits when destroyed without waiting for timeouts.

The typical implementation of a periodically called routine is a thread that executes the routine and sleeps for a period of time, then either exits if its parent thread set a variable to tell it to exit or runs the routine again. This implementation has a problem that there's no way to make it immediately and cleanly exit while it's sleeping, so longer waiting periods make the program take time to exit and shorter ones can be a waste of resources.

This library solves the problem by using thread synchronisation mechanisms to signal it to exit. Also, its clean interface saves programs from the boilerplate code required to implement this.

The period takes the execution time into consideration and tries to ensure that the period remains the same regardless of the time the routine takes. The routine can change the waiting period during its execution to adjust its waiting time dynamically.

I have tested it on MSVC and GCC to make sure there are no busy waits or weird situations.

## How does it work?

The parent thread holds a mutex that signals that the worker thread cannot exit yet. After every iteration, the worker thread waits for the mutex to unlock for a given period of time and exits if it succeeds at obtaining the mutex. If the mutex is unlocked during the execution of the routine, it's grabbed before the wait starts and the thread exits. Because the regular mutex doesn't allow waiting on it with a deadline, `std::timed_mutex` is used instead.

## Example

```C++
LoopingThread loop(std::chrono::seconds(2), [] {
	std::cout << "(worker) Waiting for 1s" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "(worker) Waited for 1s" << std::endl;
});
std::cout << "(main) Waiting for 5s" << std::endl;
std::this_thread::sleep_for(std::chrono::milliseconds(7000));
std::cout << "(main) Waited for 5s" << std::endl;

```

## Troubleshooting

Error message `undefined reference to 'pthread_create'` means that you need to add `-lpthread` to the compiler options.
