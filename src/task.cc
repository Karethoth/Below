#include "task.hh"


Task::Task()
{
	Task( "Unnamed Task", nullptr, 0 );
}


Task::Task( std::string taskName,
            void( *func )(void),
            unsigned int taskDependencies )
{
	name         = taskName;
	f            = func;
	dependencies = taskDependencies;
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

