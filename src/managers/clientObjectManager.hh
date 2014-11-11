#pragma once

#include "../events/event.hh"
#include "../events/eventDispatcher.hh"
#include "../world/worldNode.hh"
#include "../world/entity.hh"

#include <vector>
#include <memory>


class ClientObjectManager : public EventListener
{
 public:
	void HandleEvent( Event *e );

	std::vector<std::shared_ptr<WorldNode>> worldObjects;
	std::vector<std::shared_ptr<Entity>>    entities;

 private:
	std::mutex managerMutex;
};

