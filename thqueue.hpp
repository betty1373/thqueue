#ifndef THQUEUE_H
#define THQUEUE_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <limits>
#include <deque>
#include <queue>
#include <algorithm>

/**
 * A thread-safe queue for inter-thread communication.
 * This is a lockinq queue with blocking operations. The get() operations
 * can always block on an empty queue, but have variations for non-blocking
 * (try_get) and bounded-time blocking (try_get_for, try_get_until).
 * @par
 * The default queue has a capacity that is unbounded in the practical
 * sense, limited by available memory. In this mode the object will not
 * block when placing values into the queue. A capacity can bet set with the
 * construtor or, at any time later by calling the @ref capacity(size_type)
 * method. Using this latter method, the capacity can be set to an amount
 * smaller than the current size of the queue. In that case all put's to the
 * queue will block until the number of items are removed from the queue to
 * bring the size below the new capacity.
 * @par
 * Note that the queue uses move semantics to place items into the queue and
 * remove items from the queue. This means that the type, T, of the data
 * held by the queue only needs to follow move semantics; not copy
 * semantics. In addition, this means that copies of the value will @em not
 * be left in the queue. This is especially useful when creating queues of
 * shared pointers, as the "dead" part of the queue will not hold onto a
 * reference count after the item has been removed from the queue.
 *
 * @param T The type of the items to be held in the queue.
 * @param Container The type of the underlying container to use. It must
 * support back(), front(), push_back(), pop_front().
 */
template <typename T, class Container=std::deque<T>>
class thqueue
{
public:

/// @brief The container type
	using container_type = Container;
/// @brief Data type 
	using value_type = T;
/// @brief Number of data
	using size_type = typename Container::size_type;
/// @brief Maximum capacity 
	static constexpr size_type MAX_CAPACITY = std::numeric_limits<size_type>::max();

private:
	mutable std::mutex m_lock;
/// @brief Condition that queue is not empty
	std::condition_variable m_notEmptyCond;
/// @brief Condition that queue is not full
	std::condition_variable m_notFullCond;
/// @brief Capacity
	size_type m_capacity;
/// @brief STL container to hold data 
	std::queue<T,Container> m_queue;

	using guard = std::lock_guard<std::mutex>;
	using unique_guard = std::unique_lock<std::mutex>;

public:
	thqueue() : m_capacity(MAX_CAPACITY) {}

/// @brief 
	explicit thqueue(size_t cap) : m_capacity(std::max<size_type>(cap, 1)) {}

	bool empty() const {
		guard g(m_lock);
		return m_queue.empty();
	}

	size_type capacity() const {
		guard g(m_lock);
		return m_capacity;
	}

	void capacity(size_type cap) {
		guard g(m_lock);
		m_capacity = cap;
	}
	
	size_type size() const {
		guard g(m_lock);
		return m_queue.size();
	}
/// @brief Blocking Put data into queue
	void put(value_type val) {
		unique_guard g(m_lock);
		if (m_queue.size() >= m_capacity)
			m_notFullCond.wait(g, [this]{return m_queue.size() < m_capacity;});
        bool wasEmpty = m_queue.empty();
		m_queue.emplace(std::move(val));
		if (wasEmpty) {
			g.unlock();
			m_notEmptyCond.notify_one();
		}
	}
/// @brief Non-Blocking Put data into queue
	bool try_put(value_type val) {
		unique_guard g(m_lock);
		size_type n = m_queue.size();
		if (n >= m_capacity)
			return false;
		m_queue.emplace(std::move(val));
		if (n == 0) {
			g.unlock();
			m_notEmptyCond.notify_one();
		}
		return true;
	}
/// @brief Retrieve a value from the queue with blocking, while queue is empty by pointer
	void get(value_type* val) {
		unique_guard g(m_lock);
		if (m_queue.empty())
			m_notEmptyCond.wait(g, [this]{return !m_queue.empty();});
		*val = std::move(m_queue.front());
		m_queue.pop();
		if (m_queue.size() == m_capacity-1) {
			g.unlock();
			m_notFullCond.notify_one();
		}
	}
/// @brief Retrieve a value from the queue with blocking, while queue is empty
	value_type get() {
		unique_guard g(m_lock);
		if (m_queue.empty())
			m_notEmptyCond.wait(g, [this]{return !m_queue.empty();});
		value_type val = std::move(m_queue.front());
		m_queue.pop();
		if (m_queue.size() == m_capacity-1) {
			g.unlock();
			m_notFullCond.notify_one();
		}
		return val;
	}
/// @brief Retrieve a value from the queue without blocking
	bool try_get(value_type* val) {
		unique_guard g(m_lock);
		if (m_queue.empty())
			return false;
		*val = std::move(m_queue.front());
		m_queue.pop();
		if (m_queue.size() == m_capacity-1) {
			g.unlock();
			m_notFullCond.notify_one();
		}
		return true;
	}
};
#endif