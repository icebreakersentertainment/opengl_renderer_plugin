#ifndef OPENGL_H_
#define OPENGL_H_

#include <string>

#include <GL/glew.h>

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

#define ASSERT_GL_ERROR() checkGlError(__FILE__, __LINE__);

inline void checkGlError(const std::string& filename, const int line)
{
	GLenum err(glGetError());

	if (err != GL_NO_ERROR)
	{
		std::string error;

		switch (err)
		{
			case GL_INVALID_OPERATION:	  			error = "INVALID_OPERATION";				break;
			case GL_INVALID_ENUM:		  			error = "INVALID_ENUM";		  				break;
			case GL_INVALID_VALUE:		  			error = "INVALID_VALUE";		  			break;
			case GL_OUT_OF_MEMORY:		 			error = "OUT_OF_MEMORY";		  			break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";	break;
		}

		throw std::runtime_error(filename + " (" + std::to_string(line) + "): " + std::string("GL_") + error);
	}
}

inline void checkGlError()
{
	GLenum err(glGetError());

	if (err != GL_NO_ERROR)
	{
		std::string error;

		switch (err)
		{
			case GL_INVALID_OPERATION:	  			error = "INVALID_OPERATION";				break;
			case GL_INVALID_ENUM:		  			error = "INVALID_ENUM";		  				break;
			case GL_INVALID_VALUE:		  			error = "INVALID_VALUE";		  			break;
			case GL_OUT_OF_MEMORY:		 			error = "OUT_OF_MEMORY";		  			break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";	break;
		}

		throw std::runtime_error(std::string("GL_") + error);
	}
}

}
}
}
}

#endif /* OPENGL_H_ */
