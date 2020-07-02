/*
* \brief Class for wrapping a thread that calls a routine periodically until the instance of this object is destroyed
*
* The period is the period of calling the routine, not the wait between the calls. It doesn't wait for this period when it's being destroyed, but it waits
* while the routine is running.
*
* \note The waiting is implemented using std::timed_mutex
*/

#ifndef LOOPING_THREAD_H
#define LOOPING_THREAD_H

#include <thread>
#include <functional>
#include <mutex>
#include <chrono>
#include <iostream>

class LoopingThread {
	std::chrono::steady_clock::duration period_;
	std::function<void()> routine_;
	std::function<void(const std::exception&)> errorCallback_;
	std::timed_mutex waitMutex_;
	std::unique_lock<std::timed_mutex> waitLock_;
	std::mutex pauseMutex_;
	std::mutex resumeMutex_;
	std::unique_lock<std::mutex> resumeLock_;
	bool exiting_ = false;
	bool paused_;
	bool resetTimeOnPause_ = true;
	bool catchUp_ = true;
	std::thread worker_;
	
	inline void work()
	{
		std::chrono::steady_clock::time_point awakenAt = std::chrono::steady_clock::now();
		std::unique_lock<std::mutex> pauseLock(pauseMutex_);
		while (true) {
			bool interrupted = false;
			{
				std::unique_lock<std::timed_mutex> lock(waitMutex_, std::try_to_lock);
				if (lock.owns_lock())
					interrupted = true;
				else {
					if (awakenAt >= std::chrono::steady_clock::now() && lock.try_lock_until(awakenAt))
						interrupted = true;
				}
				if (resetTimeOnPause_) {
					awakenAt = std::chrono::steady_clock::now() + period_;
					resetTimeOnPause_ = false;
				}
			}
			
			if (interrupted) {
				if (exiting_) break;
				pauseLock.unlock();
				std::unique_lock<std::mutex> lock(resumeMutex_);
				pauseLock.lock();
			} else {
				try {
					routine_();
				} catch(std::exception& e) {
					errorCallback_(e);
				} catch(...) {
					errorCallback_(std::runtime_error("An unknown error has been thrown in a looping thread"));
				}
				if (catchUp_)
					awakenAt += period_;
				else
					awakenAt = std::chrono::steady_clock::now() + period_;
			}
		}
	}
public:

	/*!
	* \brief Default contructor, nothing is done if created this way
	*/
	
	inline LoopingThread() : routine_(nullptr)
	{
	
	}
	
	/*!
	* \brief Constructs the thread and starts running the routine periodically
	* \param The calling period
	* \param The function that is called periodically
	* \param If the thread starts running or is paused until resume() is called
	*/
	inline LoopingThread(std::chrono::steady_clock::duration period, std::function<void()> routine, bool run = true) :
		period_(period),
		routine_(routine),
		waitLock_(waitMutex_, std::defer_lock),
		resumeLock_(resumeMutex_),
		paused_(true),
		resetTimeOnPause_(false),
		worker_(&LoopingThread::work, this),
		errorCallback_([](const std::exception& e) { std::cout << e.what() << std::endl; })
	{
		if (run)
			resume();
	}
	
	/*!
	* \brief The destructor, interrupts the wait for another routine call, but waits for the routine to end if it's running
	*/
	inline ~LoopingThread()
	{
		if (routine_) {
			exiting_ = true;
			if (paused_)
				resumeLock_.unlock();
			else
				waitLock_.unlock();
			worker_.join();
		}
	}
	
	/*!
	* \brief Pause the execution, will wait until the end of routine
	*/
	inline void pause(bool resetTime = true)
	{
		if (routine_) {
			if (paused_) throw std::logic_error("Pausing a looping thread that is already paused");
			paused_ = true;
			waitLock_.unlock();
			resumeLock_.lock();
			std::unique_lock<std::mutex> pauseLock(pauseMutex_);
			resetTimeOnPause_ = resetTime;
		}
	}
	
	/*!
	* \brief Resume paused execution
	*/
	inline void resume()
	{
		if (routine_) {
			if (!paused_) throw std::logic_error("Resuming a looping thread that is not paused");
			paused_ = false;
			waitLock_.lock();
			resumeLock_.unlock();
		}
	}
	
	/*!
	* \brief Changes the period
	* \param The new calling period
	*/
	inline void setPeriod(std::chrono::steady_clock::duration newPeriod)
	{
		period_ = newPeriod;
	}


	/*!
	* \brief Enables or disables the catch up feature
	* \param If it should be enabled
	*
	* \note Catch up enabled causes the program call the routine only the proper number of times, so if the thread is delayed, the routine
	* will still be called as many times as expected over a long time
	*/
	inline void setCatchUp(bool catchUp)
	{
		catchUp_ = catchUp;
	}

	/*!
	* \brief Changes error callback
	* \param errorCallback function which is called when exception is thrown in routine
	*/
	inline void setErrorCallback(std::function<void(const std::exception&)> errorCallback)
	{
		errorCallback_ = errorCallback;
	}
};
#endif // LOOPING_THREAD_H
