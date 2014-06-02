#include "event.hh"
#include "eventListener.hh"

#include <vector>
#include <memory>
#include <mutex>


struct EventListenerCollection
{
	std::mutex collectionMutex;
	std::vector<EventListenerPtr> collection;
};

