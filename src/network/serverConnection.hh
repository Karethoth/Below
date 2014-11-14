#pragma once

#ifdef _MSC_VER
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <mutex>
#include <list>
#include <vector>

#include <boost/asio.hpp>

#include "../events/eventDispatcher.hh"
#include "../events/eventFactory.hh"
#include "networkEvents.hh"

using boost::asio::ip::tcp;
using namespace boost;


class ServerConnection : public EventFactory , public std::enable_shared_from_this<ServerConnection>
{
public:
	ServerConnection( );
	ServerConnection( asio::io_service& ioService, std::string host, short port );
	virtual ~ServerConnection( );

	void Init( asio::io_service& ioService, std::string host, short port );

	void Connect( asio::io_service& ioService );
	void Disconnect();
	bool IsConnected();

	void Write( std::string msg );
	void SetRead();



private:
	tcp::endpoint  m_endpoint;
	tcp::socket   *m_socket;
	std::string    m_host;
	short          m_port;

	bool           connected;

	enum { maxLength = 1024 };
	char m_data[maxLength];

	std::mutex writeMutex;
};

