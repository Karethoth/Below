#include "../gameState.hh"

#include "../events/eventListener.hh"
#include "../network/networkEvents.hh"
#include "../network/server.hh"

#include "../world/camera.hh"
#include "../world/entity.hh"

#include "../managers/serverObjectManager.hh"

#include <vector>
#include <memory>


class ServerGameState : public GameState, public EventListener
{
 public:
	ServerGameState();
	virtual ~ServerGameState();

	virtual void Create()  override;
	virtual void Destroy() override;

	virtual void Tick( std::chrono::milliseconds ) override;


	void HandleEvent( Event* );

	std::shared_ptr<ServerObjectManager> objectManager;

	std::vector<std::shared_ptr<Entity>>    entities;
	std::vector<std::shared_ptr<WorldNode>> worldNodes;

	Server server;

	bool StartServer();


 protected:
	// Event handling
	void HandleDataInEvent( DataInEvent* );
	void SendScene( unsigned int clientId );
};

