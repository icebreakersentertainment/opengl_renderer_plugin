#ifndef TEXTURECUBEMAP_H_
#define TEXTURECUBEMAP_H_

#include <ostream>

#include <GL/glew.h>
#include "graphics/exceptions/GraphicsException.hpp"

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

class TextureCubeMap : public Texture<TextureCubeMap>
{
public:
	using Texture<TextureCubeMap>::Texture;

	void generate(GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* backData, const GLvoid* downData, const GLvoid* frontData, const GLvoid* leftData, const GLvoid* rightData, const GLvoid* upData)
	{
		if (valid()) throw std::runtime_error("Cannot generate texture cube map - texture cube map was already created.");

		glGenTextures(1, &id_);

		if (id_ == INVALID_ID)
		{
            const auto e = getGlError();
			throw GraphicsException(std::string("Could not create texture cube map: ") + e.value());
		}

		bind();

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, width, height, 0, format, type, backData);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, width, height, 0, format, type, downData);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, width, height, 0, format, type, frontData);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, width, height, 0, format, type, rightData);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, width, height, 0, format, type, leftData);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, width, height, 0, format, type, upData);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		ASSERT_GL_ERROR();
	}

	void bind()
	{
		if (!valid()) throw std::runtime_error("Cannot bind texture cube map - texture cube map was not created.");

		glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
	}

	static void activate(const GLuint number)
	{
		glActiveTexture(GL_TEXTURE0 + number);
	}
};

}
}
}
}

#endif /* TEXTURECUBEMAP_H_ */
