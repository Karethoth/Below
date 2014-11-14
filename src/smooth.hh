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


inline glm::vec3 _minus( const glm::quat& lhs, const glm::quat& rhs )
{
	return glm::eulerAngles( lhs ) - glm::eulerAngles( rhs );
}



template <typename T, typename RHT=T>
inline T _plus( const T& lhs={}, const RHT& rhs={} )
{
	return lhs + rhs;
}


template <>
inline glm::quat _plus( const glm::quat& lhs, const glm::vec3& rhs )
{
	auto rot = glm::quat{ rhs };
	return lhs * rot;
}



template <typename T, typename S=float>
inline T _scalarDivide( const T& lhs={}, S rhs={1.f} )
{
	if( rhs == 0.0 ) rhs = S{ 1.f };
	return lhs / rhs;
}



template <typename T, typename S=float>
inline T _scalarMultiply( const T& lhs={}, S rhs={1.f} )
{
	return lhs * rhs;
}




template <typename T, typename DeltaType=decltype( _minus( T{}, T{} ) )>
struct Smooth
{
	typedef std::chrono::high_resolution_clock::time_point HiResTimePoint;
	typedef T ValueType;

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
		std::lock_guard<mutex> valueLock( mut );
		auto currentTime = HiResTimePoint::clock::now();
		auto deltaTime   = DeltaTime( currentTime );
		DeltaType deltaValue = _minus( newValue, value );

		lastUpdate = currentTime;
		speed = _scalarDivide( deltaValue, deltaTime );

		value = newValue;
		guess = newValue;
	}


	void Calculate()
	{
		std::lock_guard<mutex> valueLock( mut );
		auto deltaTime = DeltaTime( HiResTimePoint::clock::now() );
		if( deltaTime < 0.002 )
		{
			return;
		}

		auto sum = _scalarMultiply( speed, deltaTime );
		guess    = _plus<ValueType>( value, sum );
	}


	inline T Get()
	{
		std::lock_guard<mutex> valueLock( mut );
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
	DeltaType speed;
	std::mutex mut;
};

