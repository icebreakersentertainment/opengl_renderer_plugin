#ifndef TEXTURE2DARRAY_H_
#define TEXTURE2DARRAY_H_

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

class Texture2dArray : public Texture<Texture2dArray>
{
public:
	using Texture<Texture2dArray>::Texture;
	
	void generate(GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data = nullptr, const bool generateMipmap = false)
	{
		if (valid()) throw std::runtime_error("Cannot generate texture - texture was already created.");
		
		glGenTextures(1, &id_);
		
		if (id_ == INVALID_ID)
		{
			throw std::runtime_error("Could not create texture.");
		}
		
		bind();
		
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, depth, 0, format, type, data);
		
		if (generateMipmap)
		{
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		}
		
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		
		ASSERT_GL_ERROR();
	}
	
	void bind()
	{
		if (!valid()) throw std::runtime_error("Cannot bind texture - texture was not created.");
		
		glBindTexture(GL_TEXTURE_2D_ARRAY, id_);
	}
	
	static void activate(const GLuint number)
	{
		glActiveTexture(GL_TEXTURE0 + number);
	}
	
	static void texSubImage3D(const GLsizei width, const GLsizei height, const GLsizei depth, const GLenum format, const GLenum type, const GLvoid* data)
	{
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, depth, width, height, 1, format, type, data);
	}
	
	static void texParameter(const GLenum pname, GLint param)
	{
		glTexParameteri(GL_TEXTURE_2D_ARRAY, pname, param);
	}
};

}
}
}
}

#endif /* TEXTURE2DARRAY_H_ */

