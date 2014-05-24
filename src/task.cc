#include "task.hh"


Task::Task() : name( "Unnamed Task" )
{
	timer.Reset();
}



Task::~Task()
{
	for( auto dep  = dependents.begin();
	          dep != dependents.end();
	          dep++ )
	{
		(*dep)->dependencies -= 1;
	}
	timer.End();
}



bool Task::HasDependenciesLeft()
{
	return (dependencies > 0);
}

