#include "events/event.hh"

#include <chrono>



class Scene
{
 public:
	virtual ~Scene();

	virtual void Create()  = 0;
	virtual void Destroy() = 0;

	virtual void Tick( std::chrono::duration<std::chrono::milliseconds> ) = 0;
};

