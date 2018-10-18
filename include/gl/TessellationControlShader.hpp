#ifndef TESSELLATIONCONTROLSHADER_H_
#define TESSELLATIONCONTROLSHADER_H_

#include "Shader.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class TessellationControlShader : public Shader<TessellationControlShader>
{
public:
	using Shader<TessellationControlShader>::Shader;

	GLenum type() const
	{
		return GL_TESS_CONTROL_SHADER;
	}
};

}
}
}
}

#endif /* TESSELLATIONCONTROLSHADER_H_ */

