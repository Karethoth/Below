#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <mutex>

#include <boost/asio.hpp>

#include "../events/eventDispatcher.hh"
#include "../events/eventFactory.hh"
#include "networkEvents.hh"

using boost::asio::ip::tcp;
using namespace boost;


struct Client
	: public EventFactory,
	  public std::enable_shared_from_this<Client>
{
 public:
	Client( tcp::socket socket ) ;

	void SetRead();
	void Write( std::string );

	unsigned int m_clientId;
	tcp::socket m_socket;
	enum { maxLength = 1024 };
	char m_data[maxLength];
	std::vector<std::string> m_received;


private:
	std::mutex writeMutex;
};



class Server : public EventFactory
{
 public:
	Server();
	Server( asio::io_service& ioService, short port );
	virtual ~Server();

	void Init( asio::io_service& ioService, short port );

	void Accept();

	std::shared_ptr<Client> GetClient( unsigned int id );


	std::mutex clientListMutex;
	std::map<unsigned int, std::shared_ptr<Client>> clientList;


 private:
	tcp::acceptor *m_acceptor;
	tcp::socket   *m_socket;
	short          m_port;
};

