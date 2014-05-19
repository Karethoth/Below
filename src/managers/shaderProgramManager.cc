#include "shaderProgramManager.hh"


ShaderProgramManager::~ShaderProgramManager()
{
}



bool ShaderProgramManager::Load( const std::string& filepath )
{
	return Load( filepath, filepath );
}


bool ShaderProgramManager::Load( const std::string& filepath,
                                 const std::string& key )
{
	return false;
};

