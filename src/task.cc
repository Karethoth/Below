#include "task.hh"


Task::~Task()
{
	for( auto dep  = dependents.begin();
	          dep != dependents.end();
	          dep++ )
	{
		(*dep)->dependencies -= 1;
	}
}



bool Task::HasDependenciesLeft()
{
	return (dependencies > 0);
}

