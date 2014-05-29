#include "logger.hh"
#include <iostream>

using namespace std;

Logger::Logger()
{
}



Logger& Logger::GetInstance()
{
	static Logger loggerInstance;
	return loggerInstance;
}



void Logger::Log( string message )
{
	loggerMutex.lock();

	cout << message << endl;

	loggerMutex.unlock();
}



void Logger::LogError( string message )
{
	loggerMutex.lock();

	cerr << message << endl;

	loggerMutex.unlock();
}

