#ifndef OPENGLRENDERER_GL33_H_
#define OPENGLRENDERER_GL33_H_

#include <string>

#include <GL/glew.h>
#include <SDL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "graphics/IGraphicsEngine.hpp"
#include "graphics/Event.hpp"

#include "../gl/VertexShader.hpp"
#include "../gl/FragmentShader.hpp"
#include "../gl/TessellationControlShader.hpp"
#include "../gl/TessellationEvaluationShader.hpp"
#include "../gl/ShaderProgram.hpp"
#include "../gl/Texture2d.hpp"
#include "../gl/Texture2dArray.hpp"
#include "../gl/TextureCubeMap.hpp"
#include "../gl/FrameBuffer.hpp"

#include "handles/HandleVector.hpp"
#include "utilities/Properties.hpp"
#include "fs/IFileSystem.hpp"
#include "logger/ILogger.hpp"

using namespace ice_engine::graphics::opengl_renderer::gl;

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl33
{

struct Vbo
{
	GLuint id;
};

struct Ebo
{
	GLuint id;
	GLenum mode;
  GLsizei count;
	GLenum type;
};

struct Ubo
{
	GLuint id;
};

struct Vao
{
	GLuint id;
	Vbo vbo[4];
	Ebo ebo;
};

struct GraphicsData
{
	glm::vec3 position;
	glm::vec3 scale;
	glm::quat orientation;
};

struct Renderable
{
	Vao vao;
	Ubo ubo;
	TextureHandle textureHandle;
	MaterialHandle materialHandle;
	GraphicsData graphicsData;
	glm::ivec4 boneIds;
	glm::vec4 boneWeights;

	bool hasBones = false;
	bool hasBoneAttachment = false;
};

struct TerrainRenderable
{
	Vao vao;
	Ubo ubo;
	TerrainHandle terrainHandle;
	GraphicsData graphicsData;
};

struct Terrain
{
	Vao vao;
	uint32 width = 0;
	uint32 height = 0;
	TextureHandle textureHandle;
	TextureHandle terrainMapTextureHandle;
	TextureHandle splatMapTextureHandles[3];
	Texture2dArray splatMapTexture2dArrays[5];
};

struct SkyboxRenderable
{
	Vao vao;
	Ubo ubo;
	SkyboxHandle skyboxHandle;
	GraphicsData graphicsData;
};

struct Skybox
{
	Vao vao;
	uint32 width = 0;
	uint32 height = 0;
	TextureCubeMap textureCubeMap;
};

struct Material
{
	Texture2d albedo;
	Texture2d normal;
	Texture2d metallicRoughnessAmbientOcclusion;
};

struct RenderScene
{
	handles::HandleVector<Renderable, RenderableHandle> renderables;
	handles::HandleVector<GraphicsData, PointLightHandle> pointLights;
	handles::HandleVector<TerrainRenderable, TerrainRenderableHandle> terrain;
	handles::HandleVector<SkyboxRenderable, SkyboxRenderableHandle> skyboxes;
	ShaderProgramHandle shaderProgramHandle;
};

struct Camera
{
	glm::vec3 position;
	glm::quat orientation;
};

class OpenGlRenderer : public IGraphicsEngine
{
public:
	OpenGlRenderer(utilities::Properties* properties, fs::IFileSystem* fileSystem, logger::ILogger* logger);
	virtual ~OpenGlRenderer() override;

	OpenGlRenderer(const OpenGlRenderer& other) = delete;

	virtual void setViewport(const uint32 width, const uint32 height) override;
	virtual glm::uvec2 getViewport() const override;

	virtual glm::mat4 getModelMatrix() const override;
	virtual glm::mat4 getViewMatrix() const override;
	virtual glm::mat4 getProjectionMatrix() const override;

	virtual void beginRender() override;
	virtual void render(const RenderSceneHandle& renderSceneHandle) override;
	virtual void renderLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) override;
	virtual void renderLines(const std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3>>& lineData) override;
	virtual void endRender() override;

	virtual RenderSceneHandle createRenderScene() override;
	virtual void destroyRenderScene(const RenderSceneHandle& renderSceneHandle) override;

	virtual CameraHandle createCamera(const glm::vec3& position, const glm::vec3& lookAt = glm::vec3(0.0f, 0.0f, 0.0f)) override;

	virtual PointLightHandle createPointLight(const RenderSceneHandle& renderSceneHandle, const glm::vec3& position) override;
	virtual void destroy(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle) override;

	virtual MeshHandle createStaticMesh(const IMesh* mesh) override;
	virtual MeshHandle createDynamicMesh(const IMesh* mesh) override;

	virtual SkeletonHandle createSkeleton(const MeshHandle& meshHandle, const ISkeleton* skelton) override;
	virtual void destroy(const SkeletonHandle& skeletonHandle) override;

	virtual BonesHandle createBones(const uint32 maxNumberOfBones) override;
	virtual void destroy(const BonesHandle& bonesHandle) override;

	virtual void attach(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const BonesHandle& bonesHandle) override;
	virtual void detach(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const BonesHandle& bonesHandle) override;

	virtual void attachBoneAttachment(
		const RenderSceneHandle& renderSceneHandle,
		const RenderableHandle& renderableHandle,
		const BonesHandle& bonesHandle,
		const glm::ivec4& boneIds,
		const glm::vec4& boneWeights
	) override;
	virtual void detachBoneAttachment(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) override;

	virtual TextureHandle createTexture2d(const ITexture* texture) override;

	virtual MaterialHandle createMaterial(const IPbrMaterial* pbrMaterial) override;

	virtual TerrainHandle createStaticTerrain(
			const IHeightMap* heightMap,
			const ISplatMap* splatMap,
			const IDisplacementMap* displacementMap
		) override;
	virtual void destroy(const TerrainHandle& terrainHandle) override;

	virtual SkyboxHandle createStaticSkybox(const IImage& back, const IImage& down, const IImage& front, const IImage& left, const IImage& right, const IImage& up) override;
	virtual void destroy(const SkyboxHandle& skyboxHandle) override;

	virtual VertexShaderHandle createVertexShader(const std::string& data) override;
	virtual FragmentShaderHandle createFragmentShader(const std::string& data) override;
	virtual TessellationControlShaderHandle createTessellationControlShader(const std::string& data) override;
	virtual TessellationEvaluationShaderHandle createTessellationEvaluationShader(const std::string& data) override;
	virtual bool valid(const VertexShaderHandle& shaderHandle) const override;
	virtual bool valid(const FragmentShaderHandle& shaderHandle) const override;
	virtual bool valid(const TessellationControlShaderHandle& shaderHandle) const override;
	virtual bool valid(const TessellationEvaluationShaderHandle& shaderHandle) const override;
	virtual void destroyShader(const VertexShaderHandle& shaderHandle) override;
	virtual void destroyShader(const FragmentShaderHandle& shaderHandle) override;
	virtual void destroyShader(const TessellationControlShaderHandle& shaderHandle) override;
	virtual void destroyShader(const TessellationEvaluationShaderHandle& shaderHandle) override;
	virtual ShaderProgramHandle createShaderProgram(const VertexShaderHandle& vertexShaderHandle, const FragmentShaderHandle& fragmentShaderHandle) override;
	virtual ShaderProgramHandle createShaderProgram(
		const VertexShaderHandle& vertexShaderHandle,
		const TessellationControlShaderHandle& tessellationControlShaderHandle,
		const TessellationEvaluationShaderHandle& tessellationEvaluationShaderHandle,
		const FragmentShaderHandle& fragmentShaderHandle
	) override;
	virtual bool valid(const ShaderProgramHandle& shaderProgramHandle) const override;
	virtual void destroyShaderProgram(const ShaderProgramHandle& shaderProgramHandle) override;

	virtual RenderableHandle createRenderable(
		const RenderSceneHandle& renderSceneHandle,
		const MeshHandle& meshHandle,
		const TextureHandle& textureHandle,
		const glm::vec3& position,
		const glm::quat& orientation,
		const glm::vec3& scale = glm::vec3(1.0f),
		const ShaderProgramHandle& shaderProgramHandle = ShaderProgramHandle()
	) override;
	virtual RenderableHandle createRenderable(
		const RenderSceneHandle& renderSceneHandle,
		const MeshHandle& meshHandle,
		const MaterialHandle& materialHandle,
		const glm::vec3& position,
		const glm::quat& orientation,
		const glm::vec3& scale = glm::vec3(1.0f)
	) override;
	virtual void destroy(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) override;

	virtual TerrainRenderableHandle createTerrainRenderable(
		const RenderSceneHandle& renderSceneHandle,
		const TerrainHandle& terrainHandle
	) override;
	virtual void destroy(const RenderSceneHandle& renderSceneHandle, const TerrainRenderableHandle& terrainRenderableHandle) override;

	virtual SkyboxRenderableHandle createSkyboxRenderable(const RenderSceneHandle& renderSceneHandle, const SkyboxHandle& skyboxHandle) override;
	virtual void destroy(const RenderSceneHandle& renderSceneHandle, const SkyboxRenderableHandle& skyboxRenderableHandle) override;

	virtual void rotate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::quat& quaternion, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;
	virtual void rotate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 degrees, const glm::vec3& axis, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;
	virtual void rotate(const CameraHandle& cameraHandle, const glm::quat& quaternion, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;
	virtual void rotate(const CameraHandle& cameraHandle, const float32 degrees, const glm::vec3& axis, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;

	virtual void rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::quat& quaternion) override;
	virtual void rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 degrees, const glm::vec3& axis) override;
	virtual glm::quat rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const override;
	virtual void rotation(const CameraHandle& cameraHandle, const glm::quat& quaternion) override;
	virtual void rotation(const CameraHandle& cameraHandle, const float32 degrees, const glm::vec3& axis) override;
	virtual glm::quat rotation(const CameraHandle& cameraHandle) const override;

	virtual void translate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z) override;
	virtual void translate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& trans) override;
	virtual void translate(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const float32 x, const float32 y, const float32 z) override;
	virtual void translate(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const glm::vec3& trans) override;
	virtual void translate(const CameraHandle& cameraHandle, const float32 x, const float32 y, const float32 z) override;
	virtual void translate(const CameraHandle& cameraHandle, const glm::vec3& trans) override;

	virtual void scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z) override;
	virtual void scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& scale) override;
	virtual void scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 scale) override;
	virtual glm::vec3 scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const override;

	virtual void position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z) override;
	virtual void position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& position) override;
	virtual glm::vec3 position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const override;
	virtual void position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const float32 x, const float32 y, const float32 z) override;
	virtual void position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const glm::vec3& position) override;
	virtual glm::vec3 position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle) const override;
	virtual void position(const CameraHandle& cameraHandle, const float32 x, const float32 y, const float32 z) override;
	virtual void position(const CameraHandle& cameraHandle, const glm::vec3& position) override;
	virtual glm::vec3 position(const CameraHandle& cameraHandle) const override;

	virtual void lookAt(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& lookAt) override;
	virtual void lookAt(const CameraHandle& cameraHandle, const glm::vec3& lookAt) override;

	virtual void assign(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const SkeletonHandle& skeletonHandle) override;

	virtual void update(
		const RenderSceneHandle& renderSceneHandle,
		const RenderableHandle& renderableHandle,
		const BonesHandle& bonesHandle,
		const std::vector<glm::mat4>& transformations
	) override;

	virtual void setMouseRelativeMode(const bool enabled) override;
	virtual void setWindowGrab(const bool enabled) override;
    bool cursorVisible() const override;
    virtual void setCursorVisible(const bool visible) override;

	virtual void processEvents() override;
	virtual void addEventListener(IEventListener* eventListener) override;
	virtual void removeEventListener(IEventListener* eventListener) override;

private:
	uint32 width_;
	uint32 height_;

	GLuint shaderProgram_;

	SDL_Window* sdlWindow_;
	SDL_GLContext openglContext_;

	std::vector<IEventListener*> eventListeners_;
	handles::HandleVector<VertexShader, VertexShaderHandle> vertexShaders_;
	handles::HandleVector<FragmentShader, FragmentShaderHandle> fragmentShaders_;
	handles::HandleVector<TessellationControlShader, TessellationControlShaderHandle> tessellationControlShaders_;
	handles::HandleVector<TessellationEvaluationShader, TessellationEvaluationShaderHandle> tessellationEvaluationShaders_;
	handles::HandleVector<ShaderProgram, ShaderProgramHandle> shaderPrograms_;
	handles::HandleVector<RenderScene, RenderSceneHandle> renderSceneHandles_;
	handles::HandleVector<Vao, MeshHandle> meshes_;
	handles::HandleVector<Terrain, TerrainHandle> terrains_;
	handles::HandleVector<Skybox, SkyboxHandle> skyboxes_;
	handles::HandleVector<Ubo, SkeletonHandle> skeletons_;
	handles::HandleVector<Ubo, BonesHandle> bones_;
	handles::HandleVector<Texture2d, TextureHandle> texture2ds_;
	handles::HandleVector<Material, MaterialHandle> materials_;
	Camera camera_;

	glm::mat4 model_;
	glm::mat4 view_;
	glm::mat4 projection_;

	utilities::Properties* properties_;
	fs::IFileSystem* fileSystem_;
	logger::ILogger* logger_;

	void initialize();
	void initializeOpenGlShaderPrograms();
	void initializeOpenGlBuffers();

	MeshHandle createStaticMesh(
		const std::vector<glm::vec3>& vertices,
		const std::vector<uint32>& indices,
		const std::vector<glm::vec4>& colors,
		const std::vector<glm::vec3>& normals,
		const std::vector<glm::vec2>& textureCoordinates
	);

	std::string loadShaderContents(const std::string& filename) const;
	GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader);
	GLuint compileShader(const std::string& source, const GLenum type);

	std::string getShaderErrorMessage(const GLuint shader);
	std::string getShaderProgramErrorMessage(const GLuint shaderProgram);

	void handleEvent(const Event& event);
	static Event convertSdlEvent(const SDL_Event& event);
	static WindowEventType convertSdlWindowEventId(const uint8 windowEventId);
	static KeySym convertSdlKeySym(const SDL_Keysym& keySym);
	static ScanCode convertSdlScancode(const SDL_Scancode& sdlScancode);
	static uint16 convertSdlKeymod(const uint16 sdlKeymod);
	static KeyCode convertSdlKeycode(const SDL_Keycode& sdlKeycode);
};

}
}
}
}

#endif /* OPENGLRENDERER_GL33_H_ */
