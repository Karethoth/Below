#pragma once

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

