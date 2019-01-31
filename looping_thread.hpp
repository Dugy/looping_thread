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
#include <condition_variable>
#include <chrono>
 
class LoopingThread {
	std::chrono::system_clock::duration m_period;
	std::function<void()> m_routine;
	std::timed_mutex m_mutex;
	std::unique_lock<std::timed_mutex> m_lock;
	std::thread m_worker;

	void work() {
		while(true) {
			std::chrono::system_clock::time_point startedAt = std::chrono::system_clock::now();

			m_routine();

			std::unique_lock<std::timed_mutex> lock(m_mutex, std::try_to_lock);
			if (lock.owns_lock())
				break;
			if (lock.try_lock_until(startedAt + m_period))
				break;
		}
	}
	public:

	/*!
	* \brief Default contructor, nothing is done if created this way
	*/

	LoopingThread() : m_routine(nullptr) {

	}

	/*!
	* \brief Constructs the thread and starts running the routine periodically
	* \param The calling period
	* \param The function that is called periodically
	*/
	LoopingThread(std::chrono::system_clock::duration period, std::function<void()> routine):
		m_period(period), m_routine(routine), m_lock(m_mutex), m_worker(&LoopingThread::work, this)
	{

	}

	/*!
	* \brief The destructor, interrupts the wait for another routine call, but waits for the routine to end if it's running
	*/
	~LoopingThread() {
		if (m_routine) {
			m_lock.unlock();
			m_worker.join();
		}
	}

	/*!
	* \brief Changes the period
	* \param The new calling period
	*/
	void setPeriod(std::chrono::system_clock::duration newPeriod) {
		m_period = newPeriod;
	}
};
