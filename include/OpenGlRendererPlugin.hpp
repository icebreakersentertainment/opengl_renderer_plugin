#ifndef OPENGLRENDERERPLUGIN_H_
#define OPENGLRENDERERPLUGIN_H_

#include <memory>

#include "IGraphicsPlugin.hpp"

namespace ice_engine
{

class OpenGlRendererPlugin : public IGraphicsPlugin
{
public:
	OpenGlRendererPlugin();
	virtual ~OpenGlRendererPlugin();

	virtual std::string getName() const override;

	virtual std::unique_ptr<graphics::IGraphicsEngineFactory> createFactory() const override;

};

}

#endif /* OPENGLRENDERERPLUGIN_H_ */
