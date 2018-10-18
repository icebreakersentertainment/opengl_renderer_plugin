#ifndef VERTEXSHADER_H_
#define VERTEXSHADER_H_

#include "Shader.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class VertexShader : public Shader<VertexShader>
{
public:
	using Shader<VertexShader>::Shader;
	/*
	VertexShader() : Shader<VertexShader>::Shader()
	{
	}

	explicit VertexShader(const GLuint id) : Shader<VertexShader>::Shader(id)
	{
	}
	
	explicit VertexShader(const std::string& source)
	{
		Shader<VertexShader>::Shader(source);
	}
	
	VertexShader(const VertexShader& other) = delete;
	
	VertexShader(VertexShader&& other) : Shader<VertexShader>::Shader(std::move(other))
	{
	}
	
	~VertexShader() = default;
	
	const VertexShader& operator=(const VertexShader& other) = delete;
	*/
	/*
	const VertexShader& operator=(const VertexShader& other)
	{
		
	}
	*/

	GLenum type() const
	{
		return GL_VERTEX_SHADER;
	}
};

}
}
}
}

#endif /* VERTEXSHADER_H_ */

