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
	std::chrono::system_clock::duration _period;
	std::function<void()> _routine;
	std::timed_mutex _mutex;
	std::unique_lock<std::timed_mutex> _lock;
	std::thread _worker;

	inline void work() {
		while(true) {
			std::chrono::system_clock::time_point startedAt = std::chrono::system_clock::now();

			_routine();

			std::unique_lock<std::timed_mutex> lock(_mutex, std::try_to_lock);
			if (lock.owns_lock())
				break;
			if (lock.try_lock_until(startedAt + _period))
				break;
		}
	}
	public:

	/*!
	* \brief Default contructor, nothing is done if created this way
	*/

	inline LoopingThread() : _routine(nullptr) {

	}

	/*!
	* \brief Constructs the thread and starts running the routine periodically
	* \param The calling period
	* \param The function that is called periodically
	*/
	inline LoopingThread(std::chrono::system_clock::duration period, std::function<void()> routine):
		_period(period), _routine(routine), _lock(_mutex), _worker(&LoopingThread::work, this)
	{

	}

	/*!
	* \brief The destructor, interrupts the wait for another routine call, but waits for the routine to end if it's running
	*/
	inline ~LoopingThread() {
		if (_routine) {
			_lock.unlock();
			_worker.join();
		}
	}

	/*!
	* \brief Changes the period
	* \param The new calling period
	*/
	inline void setPeriod(std::chrono::system_clock::duration newPeriod) {
		_period = newPeriod;
	}
};
