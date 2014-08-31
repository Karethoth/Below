#include "shaderProgramManager.hh"
#include "../logger.hh"
#include <iostream>

using namespace std;


ShaderProgramManager::~ShaderProgramManager()
{
}



bool ShaderProgramManager::Load( const string& filepath )
{
	return Load( filepath, filepath );
}



bool ShaderProgramManager::Load( const std::string& filepath, const std::string& key )
{
	map<string, GLuint> attr;
	return Load( filepath, filepath, attr );
}



bool ShaderProgramManager::Load( const string& filepath,
                                 const string& key,
                                 const map<string, GLuint>& attr )
{
	Shader vertexShader( GL_VERTEX_SHADER );
	Shader fragmentShader( GL_FRAGMENT_SHADER );

	string vertexShaderPath = filepath;
	vertexShaderPath.append( ".vertex" );

	string fragmentShaderPath = filepath;
	fragmentShaderPath.append( ".fragment" );

	auto shaderProgram = make_shared<ShaderProgram>();

	if( !vertexShader.LoadFromFile( vertexShaderPath ) ||
	    !fragmentShader.LoadFromFile( fragmentShaderPath ) )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": Compiling '" << key << "' shader failed." );
		return false;
	}

	if( !shaderProgram->Load( vertexShader, fragmentShader, attr ) )
	{
		LOG_ERROR( __FILE__ << ":" << __LINE__-2 << ": Creating '" << key << "' shader failed." );
		return false;
	}

	this->Add( key, shaderProgram );

	cout << "Shader '" << key << "' created." << endl;

	return true;
};

