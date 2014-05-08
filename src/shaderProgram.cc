#include "shaderProgram.hh"
#include <iostream>


ShaderProgram::ShaderProgram()
{
	program = glCreateProgram();
	linked  = 0;
}



ShaderProgram::~ShaderProgram()
{
	if( program )
	{
		glDeleteProgram( program );

	}
}



bool ShaderProgram::Load( const Shader& vertexShader,
                          const Shader& fragmentShader,
                          std::map<std::string, GLuint> attributes )
{
	if( !program )
	{
		std::cerr << "Failed to create shader program.\n";
		return false;
	}

	if( linked )
	{
		std::cerr << "Error: Trying to use single ShaderProgram object to link multiple shaders!\n";
		return false;
	}

	// Attach the shaders
	glAttachShader( program, vertexShader.Get() );
	glAttachShader( program, fragmentShader.Get() );

	// Bind attribute locations as requested
	for( auto& attr : attributes )
	{
		glBindAttribLocation( program, attr.second, attr.first.c_str() );
	}

	// Link the shader program
	glLinkProgram( program );
	glGetProgramiv( program, GL_LINK_STATUS, &linked );

	if( !linked )
	{
		GLint infoLen = 0;
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infoLen );
		if( infoLen > 1 )
		{
			char* infoLog = new char[ sizeof( char )*infoLen ];
			std::string errorMsg;

			if( infoLog )
			{
				glGetProgramInfoLog( program, infoLen, NULL, infoLog );
				errorMsg = "Error linking shader program: " + std::string( infoLog );
				delete[] infoLog;
			}
			else
			{
				errorMsg = "Error linking shader program and also failed to allocate memory for infolog!";
			}
			std::cerr << errorMsg << "\n";
		}

		glDeleteProgram( program );
		program = 0;
		return false;
	}

	// Check out how the attributes were bound actually
	for( auto& attr : attributes )
	{
		this->attributes[attr.first] = glGetAttribLocation( program, attr.first.c_str() );
	}

	return true;
}


const GLuint ShaderProgram::Get() const
{
	return program;
}


// Fetches the requested uniform location
const GLint ShaderProgram::GetUniform( const std::string& uniformName )
{
	if( !program )
	{
		return 0;
	}
	std::map<std::string, GLint>::iterator it = uniforms.find( uniformName );

	GLint uniform = -1;
	if( it != uniforms.end() )
	{
	   uniform = it->second;
	}
	else
	{
		uniform = glGetUniformLocation( program, uniformName.c_str() );
		uniforms[uniformName] = uniform;
	}

	if( uniform == -1 )
	{
		std::cerr << "Error: Tried to get location of shader uniform '"
	              << uniformName << "', which doesn't exist.\n";
	}

	return uniform;
}


// Fetches the requested attribute location
const GLuint ShaderProgram::GetAttribute( const std::string& attributeName )
{
	if( !program )
	{
		return 0;
	}
	std::map<std::string, GLuint>::iterator it = attributes.find( attributeName );

	if( it != attributes.end() )
	{
	   return it->second;
	}

	std::cerr << "Error: Tried to get location of shader attribute '"
	          << attributeName << "', which doesn't exist.\n";

	return 0;
}

