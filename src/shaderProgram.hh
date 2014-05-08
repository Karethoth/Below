#pragma once
#ifndef _SHADERPROGRAM_HH_
#define _SHADERPROGRAM_HH_

#include "shader.hh"
#include <map>


class ShaderProgram
{
 public:
	ShaderProgram();
	~ShaderProgram();


	bool Load( const Shader& vertexShader,
	           const Shader& fragmentShader,
			   std::map<std::string, GLuint> attributes );

	const GLuint Get() const;

	const GLint  GetUniform( const std::string& uniformName );
	const GLuint GetAttribute( const std::string& attributeName );


 private:
	GLuint program;
	GLint linked;

	std::map<std::string, GLint>  uniforms;
	std::map<std::string, GLuint> attributes;
};

#endif

