#pragma once

#include <iostream>
#include <chrono>
#include <mutex>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


// Helpers

template <typename T>
inline T _minus( const T& lhs={}, const T& rhs={} )
{
	return lhs - rhs;
}


template <>
inline glm::quat _minus( const glm::quat& lhs, const glm::quat& rhs )
{
	return rhs * glm::inverse( lhs );
}



template <typename T>
inline T _plus( const T& lhs={}, const T& rhs={} )
{
	return lhs + rhs;
}


template <>
inline glm::quat _plus( const glm::quat& lhs, const glm::quat& rhs )
{
	return rhs * lhs;
}



template <typename T, typename S=float>
inline T _scalarDivide( const T& lhs={}, S rhs={1.f} )
{
	if( rhs == 0.0 ) rhs = S{ 1.f };
	return lhs / rhs;
}


template <typename S=float>
inline glm::quat _scalarDivide( const glm::quat& lhs, S rhs )
{
	if( rhs == 0.0 ) rhs = S{ 1.f };
	return lhs;
}



template <typename T, typename S=float>
inline T _scalarMultiply( const T& lhs={}, S rhs={1.f} )
{
	return lhs * rhs;
}


template <typename S=float>
inline glm::quat _scalarMultiply( const glm::quat& lhs, S rhs )
{
	return lhs;
}




template <typename T, typename SpeedType=decltype( _minus( T{}, T{} ) )>
struct Smooth
{
	typedef std::chrono::high_resolution_clock::time_point HiResTimePoint;
	typedef T valueType;

	Smooth( T startValue={} )
	{
		value = startValue;
		guess = startValue;
	}


	Smooth( T&& startValue )
	{
		value = T{ std::forward( startValue ) };
		guess = value;
	}


	Smooth& operator=( const T& startValue )
	{
		value = startValue;
		guess = value;
		return *this;
	}


	void Update( const T& newValue )
	{
		auto currentTime = HiResTimePoint::clock::now();
		auto deltaTime   = DeltaTime( currentTime );
		SpeedType deltaValue = _minus( newValue, value );

		lastUpdate = currentTime;
		std::cout << deltaTime << std::endl;
		speed = _scalarDivide( deltaValue, deltaTime );

		value = newValue;
		guess = newValue;
	}


	void Calculate()
	{
		auto deltaTime = DeltaTime( HiResTimePoint::clock::now() );
		if( deltaTime < 0.002 )
		{
			return;
		}

		guess = _plus( value, _scalarMultiply( speed, deltaTime ) );
	}


	inline T Get()
	{
		return guess;
	}


 protected:
	HiResTimePoint lastUpdate = HiResTimePoint::clock::now();


	inline float DeltaTime( const HiResTimePoint& currentTime )
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>( currentTime - lastUpdate ).count() / 10000.f;
	}

	T value;
	T guess;
	SpeedType speed;
	std::mutex mut;
};

