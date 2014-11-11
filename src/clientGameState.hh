#include "gameState.hh"

#include "events/eventListener.hh"
#include "network/networkEvents.hh"

#include "graphics/mesh.hh"
#include "world/camera.hh"
#include "world/entity.hh"

#include "managers/clientObjectManager.hh"

#include <vector>
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

	std::shared_ptr<ClientObjectManager> objectManager;

	std::vector<std::shared_ptr<Mesh>>      meshes;
	std::vector<std::shared_ptr<Entity>>    entities;
	std::vector<std::shared_ptr<WorldNode>> worldNodes;


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

	// Uniforms for rendering
	GLint colorUniform;
	GLint scaleUniform;
	GLint viewMatrixUniform;
	GLint projectionMatrixUniform;
	GLint modelMatrixUniform;

	Camera cam;
};

