#ifndef OPENGL_H_
#define OPENGL_H_

#include <string>

#include <boost/optional.hpp>

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

struct GlError
{
    GLenum code = GL_NO_ERROR;
    std::string codeString = "GL_NO_ERROR";
};

inline boost::optional<GlError> getGlError()
{
    const GLenum error = glGetError();

    if (error != GL_NO_ERROR)
    {
        GlError e;
        e.code = error;

        switch (error)
        {
            case GL_INVALID_OPERATION:	  			e.codeString = "INVALID_OPERATION";				break;
            case GL_INVALID_ENUM:		  			e.codeString = "INVALID_ENUM";		  			break;
            case GL_INVALID_VALUE:		  			e.codeString = "INVALID_VALUE";		  			break;
            case GL_OUT_OF_MEMORY:		 			e.codeString = "OUT_OF_MEMORY";		  			break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:  e.codeString = "INVALID_FRAMEBUFFER_OPERATION";	break;

            default:                                e.codeString = "Unknown";                       break;
        }

        return e;
    }

    return boost::none;
}

inline void checkGlError(const std::string& filename, const int line)
{
    const auto e = getGlError();

    if (e) throw std::runtime_error(filename + " (" + std::to_string(line) + "): " + e->codeString);
}

inline void checkGlError()
{
    const auto e = getGlError();

    if (e) throw std::runtime_error(e->codeString);
}

}
}
}
}

#endif /* OPENGL_H_ */
