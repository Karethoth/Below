#include "server.hh"
#include "networkEvents.hh"
#include "../logger.hh"
#include <atomic>
#include <memory>
#include <sstream>

using namespace std;


static atomic<unsigned int> clientIdCounter( 0 ); // Should be good enough.


Client::Client( tcp::socket socket ) : m_socket( move( socket ) )
{
	m_clientId = ++clientIdCounter;
	memset( m_data, 0, maxLength );
}


void Client::SetRead()
{
	auto self( shared_from_this() );
	m_socket.async_read_some(
		asio::buffer( m_data, maxLength ),
		[this, self]( boost::system::error_code ec, size_t length )
		{
			if( ec.value() )
			{
				LOG_ERROR( "Client::Read() got error " << ec.value() << ": '" << ec.message() << "'" );

				auto partEvent      = new PartEvent();
				partEvent->type     = NETWORK_EVENT;
				partEvent->subType  = NETWORK_PART;
				partEvent->clientId = m_clientId;
				eventQueue->AddEvent( partEvent );
				return;
			}

			stringstream stream(
				stringstream::in |
				stringstream::out |
				stringstream::binary
			);

			stream.write( m_data, length );
			memset( m_data, 0, length );

			std::string data = stream.str();

			if( !(data.length() == 2 && data[0] == '\r' && data[1] == '\n') &&
			    !(data.length() == 1 && data[0] == '\n' ) )
			{
				auto dataInEvent      = new DataInEvent();
				dataInEvent->type     = NETWORK_EVENT;
				dataInEvent->subType  = NETWORK_DATA_IN;
				dataInEvent->clientId = m_clientId;
				dataInEvent->data     = data;
				eventQueue->AddEvent( dataInEvent );
			}

			// Set this as a callback again
			SetRead();
		});
}



void Client::Write( string msg )
{
	lock_guard<mutex> writeLock( writeMutex );

	boost::asio::streambuf request;
    ostream requestStream( &request );
    requestStream << msg;

	boost::asio::write( m_socket, request );
}


Server::Server()
{
	m_port     = 22001;
	m_socket   = nullptr;
	m_acceptor = nullptr;
}


Server::Server( asio::io_service& ioService, short port )
{
	Init( ioService, port );
}


Server::~Server()
{
}



void Server::Init( asio::io_service& ioService, short port )
{
	m_port     = port;
	m_socket   = new asio::ip::tcp::socket( ioService );
	m_acceptor = new asio::ip::tcp::acceptor(
		ioService,
		tcp::endpoint( tcp::v4(), port )
	);
}


void Server::Accept()
{
	if( !m_acceptor || !m_socket )
	{
		LOG_ERROR( "Server::Accept() Error: Server is uninitialized!" );
	}

	m_acceptor->async_accept(
		*m_socket,
		[this]( boost::system::error_code ec )
		{
			if( !ec )
			{
				auto client = make_shared<Client>( move( *m_socket ) );
				client->SetEventQueue( eventQueue );
				client->SetRead();
				clientListMutex.lock();
				clientList[client->m_clientId] = client;
				clientListMutex.unlock();

				auto joinEvent      = new JoinEvent();
				joinEvent->type     = NETWORK_EVENT;
				joinEvent->subType  = NETWORK_JOIN;
				joinEvent->clientId = client->m_clientId;
				eventQueue->AddEvent( joinEvent );
			}

			Accept();
		});
}

std::shared_ptr<Client> Server::GetClient( unsigned int id )
{
	lock_guard<mutex> clientListLock( clientListMutex );
	return clientList[id];
}

