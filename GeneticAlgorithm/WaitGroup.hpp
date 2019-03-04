#include <mutex>
#include <vector>
#include <random>

/// Class representing thread safe counter with the ability to wait for all tasks to be done
/// Implemented using mutex + cond var
class WaitGroup {
public:
	/// Initialize with number of tasks
	WaitGroup(int tasks = 0)
		: m_remaining(tasks)
	{
	}

	WaitGroup(const WaitGroup &) = delete;
	WaitGroup & operator=(const WaitGroup &) = delete;

	/// Mark one task as done (substract 1 from counter)
	void done()
	{
		bool notify = false;
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			--m_remaining;
			notify = m_remaining <= 0;
		}
		if (notify)
		{
			m_condVar.notify_all();
		}
	}

	/// Get number of tasks remaining at call time, could be less when function returns
	int remaining() const
	{
		return m_remaining;
	}

	void reset(int tasks)
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		if (m_remaining != tasks && m_remaining != 0)
		{
			throw "Mnogo losho";
		}

		m_remaining = tasks;
	}

	/// Block until all tasks are done
	void wait()
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		if (m_remaining > 0)
		{
			m_condVar.wait(lock, [this]() { return this->m_remaining == 0; });
		}
	}


private:
	int                     m_remaining; ///< number of remaining tasks
	std::mutex              m_mtx;       ///< lock protecting m_remaining
	std::condition_variable m_condVar;   ///< cond var to wait on m_remaining
};
