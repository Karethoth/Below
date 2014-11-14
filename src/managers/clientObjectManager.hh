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

	std::vector<std::shared_ptr<WorldNode>> worldNodes;
	std::vector<std::shared_ptr<Entity>>    entities;

	std::mutex managerMutex;

 private:

	void AddParent( unsigned int childId, unsigned int parentId );
	void RemoveParent( unsigned int childId, unsigned int parentId );

	void AddChild( unsigned int parentId, unsigned int childId );
	void RemoveChild( unsigned int parentId, unsigned int childId );
};

