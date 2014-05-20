#include "serverConnection.hh"
#include "networkEvents.hh"
#include <atomic>
#include <memory>

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
				std::cerr << "Client::Read() got error " << ec.value() << ": '" << ec.message() << "'" << std::endl;

				auto partEvent      = new PartEvent();
				partEvent->type     = NETWORK_EVENT;
				partEvent->subType  = NETWORK_PART;
				partEvent->clientId = 0;

				eventQueue->AddEvent( partEvent );
			}

			m_data[length] = 0x00;
			std::string data = std::string( m_data );
			memset( m_data, 0, maxLength );

			if( !(data.length() == 2 && data[0] == '\r' && data[1] == '\n') &&
			    !(data.length() == 1 && data[0] == '\n' ) )
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
	auto self( shared_from_this() );
	asio::async_write(
		*m_socket,
		asio::buffer( m_data, msg.length() ),
		[this, self]( boost::system::error_code ec, std::size_t )
		{
			if( ec.value() )
			{
				std::cerr << "Client::Write() got error '" << ec.message() << "'" << std::endl;
			}
		});
}



void ServerConnection::Connect( asio::io_service& ioService )
{
	if( !m_socket )
	{
		std::cerr << "ServerConnection::Connect() Error: m_socket is uninitialized!" << std::endl;
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


bool ServerConnection::IsConnected()
{
	return connected;
}

