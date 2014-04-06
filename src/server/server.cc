#include "server.hh"
#include <atomic>


static std::atomic<unsigned int> clientIdCounter( 0 ); // Should be good enough.

Client::Client( tcp::socket socket ) : m_socket( std::move( socket ) )
{
	m_clientId = ++clientIdCounter;
	memset( m_data, 0, maxLength );
}


void Client::SetRead()
{
	auto self( shared_from_this() );
	m_socket.async_read_some(
		asio::buffer( m_data, maxLength ),
		[this, self]( boost::system::error_code ec, std::size_t length )
		{
			if( ec.value() )
			{
				if( ec.value() == boost::asio::error::eof )
				{
					std::cout << "Client disconnected" << std::endl;

					auto partEvent = new Event();
					partEvent->type    = NETWORK_EVENT;
					partEvent->subType = CLIENT_PART;
					eventQueue->AddEvent( partEvent );

					return;
				}
				std::cerr << "Client::Read() got error " << ec.value() << ": '" << ec.message() << "'" << std::endl;
			}
			m_data[length] = 0x00;
			// Should generate an event and let the dispatcher do its job.
			m_received.push_back( std::string( m_data ) );
			memset( m_data, 0, maxLength );

			// Set this as a callback again
			SetRead();
		});
}



void Client::Write( std::size_t length )
{
	auto self( shared_from_this() );
	asio::async_write(
		m_socket,
		asio::buffer( m_data, length ),
		[this, self]( boost::system::error_code ec, std::size_t )
		{
			if( ec.value() )
			{
				std::cerr << "Client::Write() got error '" << ec.message() << "'" << std::endl;
			}
		});
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
	if( m_acceptor )
	{
		m_acceptor->close();
		delete m_acceptor;
	}

	if( m_socket )
	{
		m_socket->close();
		delete m_socket;
	}
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
		std::cerr << "Server::Accept() Error: Server is uninitialized!" << std::endl;
	}

	m_acceptor->async_accept(
		*m_socket,
		[this]( boost::system::error_code ec )
		{
			if( !ec )
			{
				auto client = std::make_shared<Client>( std::move( *m_socket ) );
				client->SetEventQueue( eventQueue );
				client->SetRead();
				clientListMutex.lock();
				clientList.push_back( client );
				clientListMutex.unlock();

				auto joinEvent = new Event();
				joinEvent->type    = NETWORK_EVENT;
				joinEvent->subType = CLIENT_JOIN;
				eventQueue->AddEvent( joinEvent );
			}

			Accept();
		});
}

