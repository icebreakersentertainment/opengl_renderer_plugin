#include "OpenGlRendererFactory.hpp"

#include "gl33/OpenGlRenderer.hpp"

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{

OpenGlRendererFactory::OpenGlRendererFactory()
{
}

OpenGlRendererFactory::~OpenGlRendererFactory()
{
}

std::unique_ptr<IGraphicsEngine> OpenGlRendererFactory::create(
	utilities::Properties* properties,
	fs::IFileSystem* fileSystem,
	logger::ILogger* logger
)
{
	std::unique_ptr<IGraphicsEngine> ptr = std::make_unique< gl33::OpenGlRenderer >( properties, fileSystem, logger );
	
	return std::move( ptr );
}

}
}
}
