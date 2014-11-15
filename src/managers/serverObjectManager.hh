#pragma once

#include "../events/event.hh"
#include "../events/eventDispatcher.hh"
#include "../world/worldNode.hh"
#include "../world/entity.hh"

#include <map>
#include <memory>


class ServerObjectManager : public EventListener
{
 public:
	void HandleEvent( Event *e );

	std::vector<std::shared_ptr<WorldNode>> worldNodes;
	std::vector<std::shared_ptr<Entity>>    entities;

	std::mutex managerMutex;
};

