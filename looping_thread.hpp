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
 
class LoopingThread {
	std::chrono::steady_clock::duration period_;
	std::function<void()> routine_;
	std::timed_mutex waitMutex_;
	std::unique_lock<std::timed_mutex> waitLock_;
	std::mutex pauseMutex_;
	std::mutex resumeMutex_;
	std::unique_lock<std::mutex> resumeLock_;
	bool exiting_ = false;
	bool paused_;
	bool resetTimeOnPause_ = true;
	std::thread worker_;
	
	inline void work()
	{
		bool interrupted = false;
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
					awakenAt + period_ = std::chrono::steady_clock::now();
					resetTimeOnPause_ = false;
				}
			}
			
			if (interrupted) {
				if (exiting_) break;
				pauseLock.unlock();
				std::unique_lock<std::mutex> lock(resumeMutex_);
				pauseLock.lock();
			} else {
				routine_();
				awakenAt += period_;
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
		worker_(&LoopingThread::work, this)
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
};
#endif // LOOPING_THREAD_H
