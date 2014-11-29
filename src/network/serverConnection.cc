#include "serverConnection.hh"
#include "networkEvents.hh"
#include "serializable.hh"
#include "../logger.hh"

#include <atomic>
#include <memory>
#include <ostream>
#include <sstream>
#include <thread>

using namespace std;


ServerConnection::ServerConnection()
{
	m_port    = 22001;
	m_socket  = nullptr;
	connected = false;
}


ServerConnection::ServerConnection( asio::io_service& ioService, std::string host, short port )
{
	m_socket = nullptr;
	Init( ioService, host, port );
}


ServerConnection::~ServerConnection()
{
	if( m_socket )
	{
		m_socket->close();
		m_socket = nullptr;
	}
}



void ServerConnection::Init( asio::io_service& ioService, std::string host, short port )
{
	m_host   = host;
	m_port   = port;

	if( m_socket )
	{
		m_socket->close();
		delete m_socket;
	}

	m_socket = new asio::ip::tcp::socket( ioService );
}


static stringstream readStream(
	stringstream::in |
	stringstream::out |
	stringstream::binary
);


void ServerConnection::SetRead()
{
	auto self( shared_from_this() );
	m_socket->async_read_some(
		asio::buffer( m_data, maxLength ),
		[this, self]( boost::system::error_code ec, size_t length )
		{
			char buffer[USHRT_MAX];

			// Check for errors
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

			// Apped the received data to the readStream
			readStream.write( m_data, length );

			// Read more if we don't have enough data even
			// for the header alone
			size_t streamLength = readStream.str().length();
			if( streamLength < sizeof( unsigned short ) )
			{
				SetRead();
				return;
			}

			// Read the package byte count
			size_t packetLength = UnserializeUint16( readStream );

			// Reverse the reading point back and
			// wait for more data if we don't have enough
			if( streamLength < packetLength )
			{
				long pos = static_cast<long>( readStream.tellg() );
				readStream.seekg( pos - sizeof( unsigned short ) );
				SetRead();
				return;
			}

			// We got all the data for the package!
			readStream.read( buffer, packetLength-2 );
			stringstream packetStream(
				stringstream::in |
				stringstream::out |
				stringstream::binary
			);
			packetStream.write( buffer, packetLength-2 );

			// Create the event
			auto dataInEvent      = new DataInEvent();
			dataInEvent->type     = NETWORK_EVENT;
			dataInEvent->subType  = NETWORK_DATA_IN;
			dataInEvent->clientId = 0;
			dataInEvent->data     = packetStream.str();
			eventQueue->AddEvent( dataInEvent );


			// Set this as a callback again
			SetRead();
		}
	);
}



void ServerConnection::Write( std::string msg )
{
	if( !m_socket )
		return;

	lock_guard<mutex> writeLock( writeMutex );

	boost::asio::streambuf request;
	ostream requestStream( &request );

	unsigned short msgLength = msg.length() + 2;
	requestStream.write( reinterpret_cast<char*>( &msgLength ), 2 );

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

