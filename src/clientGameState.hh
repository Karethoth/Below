#include "gameState.hh"


class ClientGameState : public GameState
{
 public:
	ClientGameState();
	virtual ~ClientGameState();

	virtual void Create()  override;
	virtual void Destroy() override;

	virtual void Tick( std::chrono::duration<std::chrono::milliseconds> ) override;

 protected:
	void Render();
};

