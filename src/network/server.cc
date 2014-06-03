#include "server.hh"
#include "serializable.hh"
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

				m_socket.close();
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
			dataInEvent->clientId = m_clientId;
			dataInEvent->data     = packetStream.str();
			eventQueue->AddEvent( dataInEvent );

			// Set this as a callback again
			SetRead();
		});
}



void Client::Write( string msg )
{
	lock_guard<mutex> writeLock( writeMutex );

	boost::asio::streambuf request;
	ostream requestStream( &request );

	unsigned short msgLength = msg.length() + 2;
	requestStream.write( reinterpret_cast<char*>( &msgLength ), 2 );

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

