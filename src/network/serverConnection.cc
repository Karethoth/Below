#include "serverConnection.hh"
#include "networkEvents.hh"
#include "../logger.hh"

#include <atomic>
#include <memory>
#include <ostream>

using namespace std;


ServerConnection::ServerConnection()
{
	m_port    = 22001;
	m_socket  = nullptr;
	connected = false;
}


ServerConnection::ServerConnection( asio::io_service& ioService, std::string host, short port )
{
	Init( ioService, host, port );
}


ServerConnection::~ServerConnection()
{
	if( m_socket )
	{
		m_socket->close();
		delete m_socket;
	}
}



void ServerConnection::Init( asio::io_service& ioService, std::string host, short port )
{
	m_host      = host;
	m_port      = port;
	m_socket    = new asio::ip::tcp::socket( ioService );
}



void ServerConnection::SetRead()
{
	auto self( shared_from_this() );
	m_socket->async_read_some(
		asio::buffer( m_data, maxLength ),
		[this, self]( boost::system::error_code ec, std::size_t length )
		{
			if( ec.value() )
			{
				LOG_ERROR( "Client::Read() got error " << ec.value() << ": '" << ec.message() << "'" );

				auto partEvent      = new PartEvent();
				partEvent->type     = NETWORK_EVENT;
				partEvent->subType  = NETWORK_PART;
				partEvent->clientId = 0;
				eventQueue->AddEvent( partEvent );

				m_socket->close();
				return;
			}

			m_data[length] = 0x00;
			std::string data = std::string( m_data );
			memset( m_data, 0, maxLength );

			if( !(data.length() == 2 && data[0] == '\r' && data[1] == '\n') &&
			    !(data.length() == 1 && data[0] == '\n' ) &&
				  data.length() > 0 )
			{
				auto dataInEvent      = new DataInEvent();
				dataInEvent->type     = NETWORK_EVENT;
				dataInEvent->subType  = NETWORK_DATA_IN;
				dataInEvent->clientId = 0;
				dataInEvent->data     = data;
				eventQueue->AddEvent( dataInEvent );
			}

			// Set this as a callback again
			SetRead();
		});
}



void ServerConnection::Write( std::string msg )
{
	if( !m_socket )
		return;

	std::lock_guard<std::mutex> writeLock( writeMutex );

	boost::asio::streambuf request;
    std::ostream requestStream( &request );
    requestStream << msg;
	boost::asio::write( *m_socket, request );
}



void ServerConnection::Connect( asio::io_service& ioService )
{
	if( !m_socket )
	{
		LOG_ERROR( "ServerConnection::Connect() Error: m_socket is uninitialized!" );
	}

	std::stringstream sstream;
	sstream << m_port;
	std::string port;
	sstream >> port;

	tcp::resolver resolver( ioService );
    tcp::resolver::query query( m_host, port );
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;


    boost::system::error_code error = boost::asio::error::host_not_found;
    while( error && endpoint_iterator != end )
    {
      m_socket->close();
      m_socket->connect( *endpoint_iterator++, error );
    }
	if( error )
	{
		connected = false;
		throw boost::system::system_error( error );
	}

	// We managed to connect!
	connected = true;

	// Create a join event
	auto joinEvent      = new JoinEvent();
	joinEvent->type     = NETWORK_EVENT;
	joinEvent->subType  = NETWORK_JOIN;
	joinEvent->clientId = 0;
	eventQueue->AddEvent( joinEvent );

	// Create the asynchronous reader
	SetRead();
}



void ServerConnection::Disconnect()
{
	if( m_socket && connected )
	{
		m_socket->close();
	}
}



bool ServerConnection::IsConnected()
{
	return connected;
}

