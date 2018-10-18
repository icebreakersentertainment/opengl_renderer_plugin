#include <boost/config.hpp> // for BOOST_SYMBOL_EXPORT

#include "OpenGlRendererPlugin.hpp"

#include "OpenGlRendererFactory.hpp"

namespace ice_engine
{

OpenGlRendererPlugin::OpenGlRendererPlugin()
{
}

OpenGlRendererPlugin::~OpenGlRendererPlugin()
{
}

std::string OpenGlRendererPlugin::getName() const
{
	return std::string("opengl_renderer");
}

std::unique_ptr<graphics::IGraphicsEngineFactory> OpenGlRendererPlugin::createFactory() const
{
	std::unique_ptr<graphics::IGraphicsEngineFactory> ptr = std::make_unique< graphics::opengl_renderer::OpenGlRendererFactory >();
	
	return std::move( ptr );
}

// Exporting `my_namespace::plugin` variable with alias name `plugin`
// (Has the same effect as `BOOST_DLL_ALIAS(my_namespace::plugin, plugin)`)
extern "C" BOOST_SYMBOL_EXPORT OpenGlRendererPlugin plugin;
OpenGlRendererPlugin plugin;

}
