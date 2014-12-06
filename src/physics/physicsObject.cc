#include "physicsObject.hh"
#include "../logger.hh"
#include <string>
#include <sstream>

using namespace std;


#define SS_RW_BIN std::stringstream::in | std::stringstream::out | std::stringstream::binary


PhysicsObject::PhysicsObject()
{
	type = PHYSICS_OBJECT_TYPE;
	mass = 1.f;
	collisionShape = { COLLISION_SHAPE_SPHERE, {{ 1.f }} };
}



vector<string> PhysicsObject::GetDefaultFields()
{
	auto fields = Entity::GetDefaultFields();
	fields.push_back( "velocity" );
	fields.push_back( "angVelocity" );
	fields.push_back( "mass" );
	fields.push_back( "collisionShape" );
	return fields;
}



bool PhysicsObject::SerializeField( std::string &fieldName, std::stringstream &stream )
{
	stringstream  fieldStream( SS_RW_BIN );
	unsigned char fieldDataLength = 0;
	string        fieldDataString;

	// Velocity
	if( !fieldName.compare( "velocity" ) )
	{
		fieldStream << "velocity:";
		SerializeFloat( fieldStream, velocity.x );
		SerializeFloat( fieldStream, velocity.y );
		SerializeFloat( fieldStream, velocity.z );
	}

	// Angular Velocity
	else if( !fieldName.compare( "angVelocity" ) )
	{
		fieldStream << "angVelocity:";
		SerializeFloat( fieldStream, angularVelocity.x );
		SerializeFloat( fieldStream, angularVelocity.y );
		SerializeFloat( fieldStream, angularVelocity.z );
		SerializeFloat( fieldStream, angularVelocity.w );
	}

	// Mass
	else if( !fieldName.compare( "mass" ) )
	{
		fieldStream << "mass:";
		SerializeFloat( fieldStream, mass );
	}

	// Collision shape
	else if( !fieldName.compare( "collisionShape" ) )
	{
		fieldStream << "collisionShape:";
		SerializeUint8( fieldStream, collisionShape.type );
		SerializeFloat( fieldStream, collisionShape.aabb.x );
		SerializeFloat( fieldStream, collisionShape.aabb.y );
		SerializeFloat( fieldStream, collisionShape.aabb.w );
		SerializeFloat( fieldStream, collisionShape.aabb.h );
	}

	// No fitting field found?
	else
	{
		return Entity::SerializeField( fieldName, stream );
	}

	// Handle the fieldStream to get the
	// data as a string and the length of it.
	fieldDataString = fieldStream.str();
	fieldDataLength = fieldDataString.length();

	// Do we have anything to serialize?
	if( fieldDataLength <= 0 )
	{
		return false;
	}

	// Write the header / amount of data:
	SerializeUint8( stream, fieldDataLength );

	// Write the data:
	stream << fieldDataString;

	return true;
}



bool PhysicsObject::UnserializeField( std::string &fieldName, std::stringstream &stream )
{
	// Calculate the length
	stream.seekp( 0, ios::end );
	size_t streamLength = static_cast<size_t>( stream.tellp() );
	stream.seekp( 0, ios::beg );

	// Velocity
	if( !fieldName.compare( "velocity" ) )
	{
		if( streamLength != sizeof( velocity.x ) * 3 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( velocity.x ) * 3 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		velocity.x = UnserializeFloat( stream );
		velocity.y = UnserializeFloat( stream );
		velocity.z = UnserializeFloat( stream );
	}

	// Angular Velocity
	else if( !fieldName.compare( "angVelocity" ) )
	{
		if( streamLength != sizeof( angularVelocity.x ) * 4 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( angularVelocity.x ) * 3 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		angularVelocity.x = UnserializeFloat( stream );
		angularVelocity.y = UnserializeFloat( stream );
		angularVelocity.z = UnserializeFloat( stream );
		angularVelocity.w = UnserializeFloat( stream );
	}

	// Mass
	else if( !fieldName.compare( "mass" ) )
	{
		mass = UnserializeFloat( stream );
	}

	// Collision Shape
	else if( !fieldName.compare( "collisionShape" ) )
	{
		if( streamLength != sizeof( unsigned char ) + sizeof( collisionShape.aabb.x ) * 4 )
		{
			LOG_ERROR( ToString(
				"UnserializeField failed for " << fieldName <<
				", expected data to have length " << sizeof( collisionShape.aabb.x ) * 3 <<
				" instead of " << streamLength << "!"
			) );

			return false;
		}

		collisionShape.type   = static_cast<CollisionShapeType>( UnserializeUint8( stream ) );
		collisionShape.aabb.x = UnserializeFloat( stream );
		collisionShape.aabb.y = UnserializeFloat( stream );
		collisionShape.aabb.w = UnserializeFloat( stream );
		collisionShape.aabb.h = UnserializeFloat( stream );
	}

	// No such field was found
	else
	{
		return Entity::UnserializeField( fieldName, stream );
	}

	return true;
}



// Physics object collision detection
bool Collides( const PhysicsObject &a, const PhysicsObject &b )
{
	auto delta    = a.position.Raw() - b.position.Raw();
	auto distance = abs( glm::distance( glm::vec3( 0.0f ), delta ) );

	// SPHERE - SPHERE
	if( a.collisionShape.type == COLLISION_SHAPE_SPHERE &&
	    b.collisionShape.type == COLLISION_SHAPE_SPHERE )
	{
		if( distance <= a.collisionShape.sphere.size + b.collisionShape.sphere.size )
			return true;
		return false;
	}

	// UNHANDLED SITUATION
	else
	{
		std::cout << "Collision not handled between types "
		          << a.collisionShape.type << " & "
		          << b.collisionShape.type << std::endl;

		return false;
	}
}

