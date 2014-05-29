#include "shader.hh"
#include "../logger.hh"

#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

using std::string;


Shader::Shader( GLenum type ) : type(type)
{
	compiled = 0;
	shader = 0;
}



Shader::~Shader()
{
	glDeleteShader( shader );
}



bool Shader::Load( const string& source )
{
	if( compiled || shader )
	{
		LOG_ERROR( "Error: Trying to use single Shader object to load multiple shaders!" );
		return false;
	}

	shader = glCreateShader( type );
	if( !shader )
		return false;

	const GLchar *sourcePtr = (GLchar*)source.c_str();
	glShaderSource( shader, 1, &sourcePtr, NULL );

	glCompileShader( shader );
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );

	if( !compiled )
	{
		GLint infoLen = 0;
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLen );
		if( infoLen > 1 )
		{
			char* infoLog = new char[ sizeof( char )*infoLen ];
			string errorMsg;

			if( infoLog )
			{
				glGetShaderInfoLog( shader, infoLen, NULL, infoLog );
				errorMsg = "Error compiling shader: " + string( infoLog );
				delete[] infoLog;
			}
			else
			{
				errorMsg = "Error compiling shader and also failed to allocate memory for infolog!";
			}
			LOG_ERROR( errorMsg );
		}

		glDeleteShader( shader );
		shader = 0;
		return false;
	}

	return true;
}



bool Shader::LoadFromFile( const string& filepath )
{
	std::ifstream file( filepath );
	string str;
	string shaderSource;

	if( !file.is_open() )
	{
		LOG_ERROR( "Error: Failed to load shader from file '" << filepath << "'" );
		return false;
	}

	while( std::getline( file, str ) )
	{
	  shaderSource += str;
	  shaderSource.push_back('\n');
	}

	return Load( shaderSource );
}



const GLuint Shader::Get() const
{
	return shader;
}

