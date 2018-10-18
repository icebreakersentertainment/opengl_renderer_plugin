#ifndef TEXTURE2D_H_
#define TEXTURE2D_H_

#include <ostream>

#include <GL/glew.h>

#include "OpenGl.hpp"
#include "Texture.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class Texture2d : public Texture<Texture2d>
{
public:
	using Texture<Texture2d>::Texture;
	
	void generate(GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data, const bool generateMipmap = false)
	{
		if (valid()) throw std::runtime_error("Cannot generate texture - texture was already created.");
		
		glGenTextures(1, &id_);
		
		if (id_ == INVALID_ID)
		{
			throw std::runtime_error("Could not create texture.");
		}
		
		bind();
		
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
		
		if (generateMipmap)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		
		glBindTexture(GL_TEXTURE_2D, 0);
		
		ASSERT_GL_ERROR();
	}
	
	void bind()
	{
		if (!valid()) throw std::runtime_error("Cannot bind texture - texture was not created.");
		
		glBindTexture(GL_TEXTURE_2D, id_);
	}
	
	static void activate(const GLuint number)
	{
		glActiveTexture(GL_TEXTURE0 + number);
	}
	
	static void texParameter(const GLenum pname, GLint param)
	{
		glTexParameteri(GL_TEXTURE_2D, pname, param);
	}
};

}
}
}
}

#endif /* TEXTURE2D_H_ */

