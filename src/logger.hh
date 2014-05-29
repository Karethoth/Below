#pragma once
#include <string>
#include <mutex>
#include <sstream>
#include <ostream>


class Logger
{
 public:
	void Log( std::string );
	void LogError( std::string );

	static Logger& GetInstance();


 private:
	Logger();
	Logger( Logger const& );
	void operator=( Logger const& );

	std::mutex loggerMutex;
};


// Macro to ease up appending stuff together:
// - Thanks to Mr.Ree's answer to
//   http://stackoverflow.com/questions/303562/
#define ToString( MSG ) \
	((dynamic_cast<ostringstream &>(\
	ostringstream().seekp( 0, ios_base::cur ) << MSG \
	)).str())


// Quick access to the Logger.Log
#define LOG( MSG ){ \
	Logger::GetInstance().Log( ToString( MSG ) ); \
}

// Quick access to the Logger.LogError
#define LOG_ERROR( MSG ){ \
	Logger::GetInstance().LogError( ToString( MSG ) ); \
}

