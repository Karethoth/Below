#include "events/event.hh"

#include <chrono>



class GameState
{
 public:
	virtual ~GameState();

	virtual void Create()  = 0;
	virtual void Destroy() = 0;

	virtual void Tick( std::chrono::duration<std::chrono::milliseconds> ) = 0;
};

