#pragma once

#include "../events/event.hh"
#include "../events/eventDispatcher.hh"
#include "../world/worldNode.hh"

#include <map>
#include <memory>


class ServerObjectManager : public EventListener
{
 public:
	void HandleEvent( Event *e );

	std::map<unsigned int, std::shared_ptr<WorldNode>> worldObjects;

 private:
	std::mutex managerMutex;
};

