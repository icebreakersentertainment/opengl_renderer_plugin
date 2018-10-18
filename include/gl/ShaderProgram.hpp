#ifndef SHADERPROGRAM_H_
#define SHADERPROGRAM_H_

#include "Program.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl
{

class ShaderProgram : public Program<ShaderProgram>
{
public:
	using Program<ShaderProgram>::Program;
};

}
}
}
}

#endif /* SHADERPROGRAM_H_ */

