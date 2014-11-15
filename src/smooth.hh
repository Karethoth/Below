#pragma once

#include <iostream>
#include <chrono>
#include <mutex>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


// Helpers
namespace
{
	// For differences between quaternions.
	struct AngleAxis
	{
		float     angle;
		glm::vec3 axis;

		AngleAxis( float _angle=0.0f, glm::vec3 _axis={} ) : angle(_angle), axis(_axis) {}
	};

	AngleAxis operator* ( const AngleAxis& lhs, float rhs )
	{
		AngleAxis result{ lhs.angle, lhs.axis };
		result.angle *= rhs;
		return result;
	}

	AngleAxis operator/ ( const AngleAxis& lhs, float rhs )
	{
		AngleAxis result{ lhs.angle, lhs.axis };
		result.angle /= rhs;
		return result;
	}


	// Difference
	template <typename T>
	inline T _minus( const T& lhs={}, const T& rhs={} )
	{
		return lhs - rhs;
	}


	// Difference between quaternions
	inline AngleAxis _minus( const glm::quat& lhs, const glm::quat& rhs )
	{
		auto quatDelta = lhs * glm::inverse(rhs);
		return AngleAxis{ glm::angle( quatDelta ), glm::axis( quatDelta ) };;
	}


	// Addition
	template <typename T, typename RHT=T>
	inline T _plus( const T& lhs={}, const RHT& rhs={} )
	{
		return lhs + rhs;
	}


	template <>
	inline glm::quat _plus( const glm::quat& lhs, const AngleAxis& rhs )
	{
		auto rot = glm::angleAxis( rhs.angle, rhs.axis );
		return lhs * rot;
	}


	// Division
	template <typename T, typename S=float>
	inline T _scalarDivide( const T& lhs={}, S rhs={1.f} )
	{
		if( rhs == 0.0 ) rhs = S{ 1.f };
		return lhs / rhs;
	}


	// Multiplying
	template <typename T, typename S=float>
	inline T _scalarMultiply( const T& lhs={}, S rhs={1.f} )
	{
		return lhs * rhs;
	}
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
		std::lock_guard<std::mutex> valueLock( mut );
		auto currentTime = HiResTimePoint::clock::now();
		auto deltaTime   = DeltaTime( currentTime );
		DeltaType deltaValue = _minus( newValue, value );

		lastUpdate = currentTime;
		speed = _scalarDivide( deltaValue, deltaTime );

		value = newValue;
		guess = newValue;
	}


	void Calculate( const float& stepMultiplier={1.f} )
	{
		std::lock_guard<std::mutex> valueLock( mut );
		auto deltaTime = DeltaTime( HiResTimePoint::clock::now() );
		auto sum = _scalarMultiply( speed, deltaTime*stepMultiplier );
		guess    = _plus<ValueType>( value, sum );
	}


	inline T Get()
	{
		std::lock_guard<std::mutex> valueLock( mut );
		return guess;
	}


 protected:
	inline float DeltaTime( const HiResTimePoint& currentTime )
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>( currentTime - lastUpdate ).count() / 1000.f;
	}

	HiResTimePoint lastUpdate = HiResTimePoint::clock::now();

	T value;
	T guess;
	DeltaType speed;
	std::mutex mut;
};

