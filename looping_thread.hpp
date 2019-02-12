/*
* \brief Class for wrapping a thread that calls a routine periodically until the instance of this object is destroyed
*
* The period is the period of calling the routine, not the wait between the calls. It doesn't wait for this period when it's being destroyed, but it waits
* while the routine is running.
*
* \note The waiting is implemented using std::timed_mutex
*/

#include <thread>
#include <functional>
#include <mutex>
#include <chrono>
 
class LoopingThread {
	std::chrono::system_clock::duration period_;
	std::function<void()> routine_;
	std::timed_mutex pauseMutex_;
	std::unique_lock<std::timed_mutex> pauseLock_;
	std::mutex resumeMutex_;
	std::unique_lock<std::mutex> resumeLock_;
	std::thread worker_;
	bool exiting_ = false;
	bool paused_;
	bool resetTimeOnPause_ = true;
 
	inline void work() {
		bool interrupted = false;
		std::chrono::system_clock::time_point awakenAt = std::chrono::system_clock::now();
		while(true) {
			bool interrupted = false;
			{
				std::unique_lock<std::timed_mutex> lock(pauseMutex_, std::try_to_lock);
				if (lock.owns_lock())
					interrupted = true;
				else {
					if (awakenAt >= std::chrono::system_clock::now() && lock.try_lock_until(awakenAt))
						interrupted = true;
				}
				if (resetTimeOnPause_) {
					awakenAt + period_ = std::chrono::system_clock::now();
					resetTimeOnPause_ = false;
				}
			}
 
			if (interrupted) {
				if (exiting_) break;
				paused_ = true;
				std::unique_lock<std::mutex> lock(resumeMutex_);
				paused_ = false;
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
 
	inline LoopingThread() : routine_(nullptr) {
 
	}
 
	/*!
	* \brief Constructs the thread and starts running the routine periodically
	* \param The calling period
	* \param The function that is called periodically
	* \param If the thread starts running or is paused until resume() is called
	*/
	inline LoopingThread(std::chrono::system_clock::duration period, std::function<void()> routine, bool run = true):
		period_(period),
		routine_(routine),
		pauseLock_(pauseMutex_, std::defer_lock),
		resumeLock_(resumeMutex_),
		worker_(&LoopingThread::work, this),
		paused_(true),
		resetTimeOnPause_(true)
	{
		if (run)
			resume();
	}
 
	/*!
	* \brief The destructor, interrupts the wait for another routine call, but waits for the routine to end if it's running
	*/
	inline ~LoopingThread() {
		if (routine_) {
			exiting_ = true;
			pauseLock_.unlock();
			worker_.join();
		}
	}
 
	/*!
	* \brief Pause the execution, will wait until the end of routine
	*/
	inline void pause(bool resetTime = true) {
		if (routine_) {
			resumeLock_.lock();
			pauseLock_.unlock();
			resetTimeOnPause_ = resetTime;
		}
	}
 
	/*!
	* \brief Resume paused execution
	*/
	inline void resume() {
		if (routine_ && paused_) {
			pauseLock_.lock();
			resumeLock_.unlock();
		}
	}
 
	/*!
	* \brief Changes the period
	* \param The new calling period
	*/
	inline void setPeriod(std::chrono::system_clock::duration newPeriod) {
		period_ = newPeriod;
	}
};
