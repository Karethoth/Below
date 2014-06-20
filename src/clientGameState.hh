#include "gameState.hh"

#include "events/eventListener.hh"
#include "network/networkEvents.hh"

#include <memory>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>


class ClientGameState : public GameState, public EventListener
{
 public:
	ClientGameState();
	virtual ~ClientGameState();

	virtual void Create()  override;
	virtual void Destroy() override;

	virtual void Tick( std::chrono::milliseconds ) override;


	void HandleEvent( Event* );


 protected:
	void Connect();
	void Render();

	// Event handling
	void HandleDataInEvent( DataInEvent* );


	// State info and flags
	struct
	{
		bool connected;
		bool tryingToConnect;
	} state;
};

