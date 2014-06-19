#include "scene.hh"


class ClientStartScene : public Scene
{
 public:
	ClientStartScene();
	virtual ~ClientStartScene();

	virtual void Create();
	virtual void Destroy();

	virtual void Tick( std::chrono::duration<std::chrono::milliseconds> );
};

