#pragma once

#include <string>
#include <vector>
#include <atomic>

#include "statistics/executionTimer.hh"


struct Task
{
 public:
	Task();
	Task( std::string taskName,
	      void (*func)(void) = nullptr,
	      unsigned int taskDependencies = 0 );

	virtual ~Task();

	bool HasDependenciesLeft();

	std::string        name;
	std::atomic_uint   dependencies;
	std::vector<Task*> dependents;
	ExecutionTimer     timer;

	void (*f)(void);
};

