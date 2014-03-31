#pragma once
#ifndef TASK_HH
#define TASK_HH

#include <vector>
#include <atomic>


class Task
{
 public:
	virtual ~Task();

	bool HasDependenciesLeft();

	void (*f)(void);

	std::atomic_uint   dependencies;
	std::vector<Task*> dependents;
};


#endif

