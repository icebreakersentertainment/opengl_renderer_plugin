#ifndef FRAGMENTSHADER_H_
#define FRAGMENTSHADER_H_

#include "Shader.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class FragmentShader : public Shader<FragmentShader>
{
public:
	using Shader<FragmentShader>::Shader;

	GLenum type() const
	{
		return GL_FRAGMENT_SHADER;
	}
};

}
}
}
}

#endif /* FRAGMENTSHADER_H_ */

