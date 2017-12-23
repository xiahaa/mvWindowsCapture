/*
 * safe_queue.h
*	Thread safe Async Queue
 *  Created on: Apr X, 2017
 *      Source: http://stackoverflow.com/questions/15278343/c11-thread-safe-queue, user "ChewOnThis_Trident"
 *      with minor changes
 */

#ifndef SAFE_QUEUE_H_
#define SAFE_QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>

 // A threadsafe-queue.
template <class DataType>
class SafeQueue
{
public:
	SafeQueue(void)
		: queue()
		, mtex()
		, condVar()
	{}

	~SafeQueue(void)
	{}

	// Add an element to the queue.
	void enqueue(DataType dat)
	{
		std::lock_guard<std::mutex> lock(mtex);
		queue.push(dat);
		condVar.notify_one();
	}

	// Wait until an element is available in the queue
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtex);
		while (queue.empty())
		{
			// release lock as long as the wait and re-aquire it afterwards.
			condVar.wait(lock);
		}
		return;
	}

	// Get the "front"-element.
	// If the queue is empty, wait until an element is available.
	DataType dequeue()
	{
		std::unique_lock<std::mutex> lock(mtex);
		while (queue.empty())
		{
			// release lock as long as the wait and re-aquire it afterwards.
			condVar.wait(lock);
		}
		DataType val = queue.front();
		queue.pop();
		lock.unlock();
		return val;
	}

	// Get the queue state (full/empty)
	bool empty(void) const
	{
		std::unique_lock<std::mutex> lock(mtex);
		return queue.empty();
	}

	// Get the queue state (size)
	int size(void) const
	{
		std::unique_lock<std::mutex> lock(mtex);
		return queue.size();
	}

private:
	std::queue<DataType> queue;
	mutable std::mutex mtex;
	std::condition_variable condVar;
};
#endif /* SAFE_QUEUE_H_ */

