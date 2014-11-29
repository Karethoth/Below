#pragma once

#define GLM_FORCE_RADIANS

#include <iostream>
#include <chrono>
#include <mutex>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


// Helpers
namespace smoothHelpers
{
	// For differences between quaternions.
	struct AngleAxis
	{
		float     angle;
		glm::vec3 axis;

		AngleAxis( float _angle=0.0f, glm::vec3 _axis={} ) : angle(_angle), axis(_axis) {}
	};

	// Required binary operators for AngleAxis
	AngleAxis operator* ( const AngleAxis& lhs, float rhs );
	AngleAxis operator/ ( const AngleAxis& lhs, float rhs );


	// Difference
	template <typename T>
	inline T Minus( const T& lhs={}, const T& rhs={} )
	{
		return lhs - rhs;
	}


	// Difference between quaternions
	inline AngleAxis Minus( const glm::quat& lhs, const glm::quat& rhs )
	{
		auto quatDelta = lhs * glm::inverse(rhs);
		return AngleAxis{ glm::angle( quatDelta ), glm::axis( quatDelta ) };;
	}


	// Addition
	template <typename T, typename RHT=T>
	inline T Plus( const T& lhs={}, const RHT& rhs={} )
	{
		return lhs + rhs;
	}


	template <>
	inline glm::quat Plus( const glm::quat& lhs, const AngleAxis& rhs )
	{
		auto rot = glm::angleAxis( rhs.angle, rhs.axis );
		return lhs * rot;
	}


	// Division
	template <typename T, typename S=float>
	inline T ScalarDivide( const T& lhs={}, S rhs={1.f} )
	{
		if( rhs == 0.0 ) rhs = S{ 1.f };
		return lhs / rhs;
	}


	// Multiplying
	template <typename T, typename S=float>
	inline T ScalarMultiply( const T& lhs={}, S rhs={1.f} )
	{
		return lhs * rhs;
	}
}



template <
	typename T,
	typename DeltaType=decltype( smoothHelpers::Minus( T{}, T{} ) )
>
struct Smooth
{
	typedef std::chrono::high_resolution_clock::time_point HiResTimePoint;
	typedef T ValueType;

	Smooth( const T& startValue={} )
	{
		value = startValue;
		guess = startValue;
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
		DeltaType deltaValue = smoothHelpers::Minus( newValue, value );

		lastUpdate = currentTime;
		speed = smoothHelpers::ScalarDivide( deltaValue, deltaTime );

		value = newValue;
		guess = newValue;
	}


	void Calculate( const float& stepMultiplier={1.f} )
	{
		std::lock_guard<std::mutex> valueLock( mut );
		auto deltaTime = DeltaTime( HiResTimePoint::clock::now() );
		auto sum = smoothHelpers::ScalarMultiply( speed, deltaTime*stepMultiplier );
		guess    = smoothHelpers::Plus<ValueType>( value, sum );
	}


	inline T Get()
	{
		std::lock_guard<std::mutex> valueLock( mut );
		return guess;
	}


	inline T Raw() const
	{
		std::lock_guard<std::mutex> valueLock( mut );
		return value;
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
	mutable std::mutex mut;
};

// Explicit template instantiations:
// (Helps to supress clang warnings)
template struct Smooth<glm::vec3>;
template struct Smooth<glm::quat>;

