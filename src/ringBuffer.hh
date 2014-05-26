#include <vector>

template <class T>
class RingBuffer
{
 public:
	RingBuffer( size_t bufferSize ): ring( bufferSize ),
	                                 bufferSize( bufferSize ),
	                                 head( 0 ),
	                                 tail( 0 )
	{
	}


	T* Front()
	{
		if( tail < head )
		{
			return &( ring[tail % bufferSize] );
		}

		return nullptr;
	}


	T* Back()
	{
		if( Available( head, tail ) )
		{
			return &( ring[head % bufferSize] );
		}

		return nullptr;
	}


	void Push()
	{
		++head;
	}


	void Pop()
	{
		++tail;
	}


	size_t Size() const
	{
		if( tail < head )
			return bufferSize - (tail + bufferSize - head);
		else if( tail > head )
			return bufferSize - (tail - head);

		return 0;
	}


	bool Available()
	{
		return available( head, tail );
	}



 private:
	std::vector<T>        ring;
	const size_t          bufferSize;
	std::atomic<uint64_t> head;
	std::atomic<uint64_t> tail;


	bool Available( uint64_t h, uint64_t t ) const
	{
		if( h == t )
		{
			return true;
		}
		else if( t > h )
		{
			return (t - h) > bufferSize;
		}

		return (t + bufferSize) - h > 0;
	}
};

