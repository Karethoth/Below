#include "gameState.hh"


class ClientStartScene : public GameState
{
 public:
	ClientStartScene();
	virtual ~ClientStartScene();

	virtual void Create()  override;
	virtual void Destroy() override;

	virtual void Tick( std::chrono::duration<std::chrono::milliseconds> ) override;
};

