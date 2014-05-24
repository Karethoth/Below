#include "executionTimer.hh"


void ExecutionTimer::Reset()
{
	// Clear start and end
	start = StatisticsTimePoint();
	end   = StatisticsTimePoint();

	// Set this as the creation time
	creation = StatisticsTimePoint::clock::now();
}



void ExecutionTimer::Start()
{
	start = StatisticsTimePoint::clock::now();

	waitDuration = start - creation;
}



void ExecutionTimer::End()
{
	end = StatisticsTimePoint::clock::now();

	executionDuration = end - start;
}

