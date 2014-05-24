#pragma once

#include <chrono>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> StatisticsTimePoint;
typedef std::chrono::duration<double> StatisticsDuration;


struct ExecutionTimer
{
	void Reset();
	void Start();
	void End();


	// The time points
	StatisticsTimePoint creation;
	StatisticsTimePoint start;
	StatisticsTimePoint end;

	// How long creation and the start of execution
	StatisticsDuration  waitDuration;

	// How long the execution took
	StatisticsDuration  executionDuration;
};

