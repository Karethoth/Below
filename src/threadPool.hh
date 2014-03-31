#pragma once
#ifndef THREAD_POOL_HH
#define THREAD_POOL_HH

#include <vector>
#include <thread>
#include <mutex>

#include "workerContext.hh"


class ThreadPool
{
 public:
	std::mutex threadListMutex;
	std::mutex contextListMutex;

	std::vector<std::thread*>   threads;
	std::vector<WorkerContext*> contexts;

	void CleanThreads();
};

#endif

