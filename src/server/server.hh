#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <mutex>

#include <boost/asio.hpp>

#include "../eventDispatcher.hh"
#include "../eventFactory.hh"


using boost::asio::ip::tcp;
using namespace boost;


struct Client
	: public EventFactory,
	  public std::enable_shared_from_this<Client>
{
 public:
	Client( tcp::socket socket ) ;

	void SetRead();
	void Write( std::size_t length );

	unsigned int m_clientId;
	tcp::socket m_socket;
	enum { maxLength = 1024 };
	char m_data[maxLength];
	std::vector<std::string> m_received;
};



class Server : public EventFactory
{
 public:
	Server();
	Server( asio::io_service& ioService, short port );
	virtual ~Server();

	void Init( asio::io_service& ioService, short port );

	void Accept();

	std::mutex clientListMutex;
	std::vector<std::shared_ptr<Client>> clientList;

	void HandleEvent( Event *e );


 private:
	tcp::acceptor *m_acceptor;
	tcp::socket   *m_socket;
	short          m_port;
};




// All network related events
struct JoinEvent : public Event
{
	Client *client;
};


struct PartEvent : public Event
{
	Client *client;
};


struct DataInEvent : public Event
{
	Client      *client;
	std::string  data;
};

