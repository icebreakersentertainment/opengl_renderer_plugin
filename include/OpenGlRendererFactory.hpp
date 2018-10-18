#ifndef OPENGLRENDERERFACTORY_H_
#define OPENGLRENDERERFACTORY_H_

#include <memory>

#include "graphics/IGraphicsEngine.hpp"
#include "graphics/IGraphicsEngineFactory.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{

class OpenGlRendererFactory : public IGraphicsEngineFactory
{
public:
	OpenGlRendererFactory();
	virtual ~OpenGlRendererFactory();

	virtual std::unique_ptr<IGraphicsEngine> create(
		utilities::Properties* properties,
		fs::IFileSystem* fileSystem,
		logger::ILogger* logger
	) override;

};

}
}
}

#endif /* OPENGLRENDERERFACTORY_H_ */
