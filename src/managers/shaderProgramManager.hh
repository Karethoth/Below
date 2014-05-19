#pragma once
#ifndef _SHADERPROGRAMMANAGER_HH_
#define _SHADERPROGRAMMANAGER_HH_

#include <string>

#include "templateManager.hh"
#include "../graphics/shaderProgram.hh"


class ShaderProgramManager : public TemplateManager<ShaderProgram>
{
 public:
	virtual ~ShaderProgramManager();

	virtual bool Load( const std::string& filepath );
	virtual bool Load( const std::string& filepath, const std::string& key );
};


#endif

