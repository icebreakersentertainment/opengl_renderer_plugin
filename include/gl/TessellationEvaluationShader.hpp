#ifndef TESSELLATIONEVALUATIONSHADER_H_
#define TESSELLATIONEVALUATIONSHADER_H_

#include "Shader.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class TessellationEvaluationShader : public Shader<TessellationEvaluationShader>
{
public:
	using Shader<TessellationEvaluationShader>::Shader;

	GLenum type() const
	{
		return GL_TESS_EVALUATION_SHADER;
	}
};

}
}
}
}

#endif /* TESSELLATIONEVALUATIONSHADER_H_ */

