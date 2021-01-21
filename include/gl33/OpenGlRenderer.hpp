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
	glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
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
	glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

class OpenGlRenderer : public IGraphicsEngine
{
public:
	OpenGlRenderer(utilities::Properties* properties, fs::IFileSystem* fileSystem, logger::ILogger* logger);
	~OpenGlRenderer() override;

    OpenGlRenderer(const OpenGlRenderer& other) = delete;
    OpenGlRenderer& operator=(const OpenGlRenderer& other) = delete;

	void setViewport(const uint32 width, const uint32 height) override;
	glm::uvec2 getViewport() const override;

	glm::mat4 getModelMatrix() const override;
	glm::mat4 getViewMatrix() const override;
	glm::mat4 getProjectionMatrix() const override;

	void beginRender() override;
	void render(const RenderSceneHandle& renderSceneHandle) override;
	void renderLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) override;
	void renderLines(const std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3>>& lineData) override;
	void endRender() override;

	RenderSceneHandle createRenderScene() override;
    bool valid(const RenderSceneHandle& renderSceneHandle) const override;
	void destroy(const RenderSceneHandle& renderSceneHandle) override;

	CameraHandle createCamera(const glm::vec3& position, const glm::vec3& lookAt = glm::vec3(0.0f, 0.0f, 0.0f)) override;
    bool valid(const CameraHandle& cameraHandle) const override;
    void destroy(const CameraHandle& cameraHandle) override;

	PointLightHandle createPointLight(const RenderSceneHandle& renderSceneHandle, const glm::vec3& position) override;
    bool valid(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle) const override;
	void destroy(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle) override;

	MeshHandle createStaticMesh(const IMesh& mesh) override;
	MeshHandle createDynamicMesh(const IMesh& mesh) override;
    bool valid(const MeshHandle& meshHandle) const override;
    void destroy(const MeshHandle& meshHandle) override;

	SkeletonHandle createSkeleton(const MeshHandle& meshHandle, const ISkeleton& skelton) override;
    bool valid(const SkeletonHandle& skeletonHandle) const override;
	void destroy(const SkeletonHandle& skeletonHandle) override;

	BonesHandle createBones(const uint32 maxNumberOfBones) override;
    bool valid(const BonesHandle& bonesHandle) const override;
	void destroy(const BonesHandle& bonesHandle) override;

	void attach(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const BonesHandle& bonesHandle) override;
	void detach(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const BonesHandle& bonesHandle) override;

	void attachBoneAttachment(
		const RenderSceneHandle& renderSceneHandle,
		const RenderableHandle& renderableHandle,
		const BonesHandle& bonesHandle,
		const glm::ivec4& boneIds,
		const glm::vec4& boneWeights
	) override;
	void detachBoneAttachment(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) override;

	TextureHandle createTexture2d(const ITexture& texture) override;
    bool valid(const TextureHandle& textureHandle) const override;
    void destroy(const TextureHandle& textureHandle) override;

	MaterialHandle createMaterial(const IPbrMaterial& pbrMaterial) override;
    bool valid(const MaterialHandle& materialHandle) const override;
    void destroy(const MaterialHandle& materialHandle) override;

	TerrainHandle createStaticTerrain(
            const IHeightMap& heightMap,
            const ISplatMap& splatMap,
            const IDisplacementMap& displacementMap
		) override;
    bool valid(const TerrainHandle& terrainHandle) const override;
	void destroy(const TerrainHandle& terrainHandle) override;

	SkyboxHandle createStaticSkybox(const IImage& back, const IImage& down, const IImage& front, const IImage& left, const IImage& right, const IImage& up) override;
    bool valid(const SkyboxHandle& skyboxHandle) const override;
	void destroy(const SkyboxHandle& skyboxHandle) override;

	VertexShaderHandle createVertexShader(const std::string& data) override;
	FragmentShaderHandle createFragmentShader(const std::string& data) override;
	TessellationControlShaderHandle createTessellationControlShader(const std::string& data) override;
	TessellationEvaluationShaderHandle createTessellationEvaluationShader(const std::string& data) override;
	bool valid(const VertexShaderHandle& shaderHandle) const override;
	bool valid(const FragmentShaderHandle& shaderHandle) const override;
	bool valid(const TessellationControlShaderHandle& shaderHandle) const override;
	bool valid(const TessellationEvaluationShaderHandle& shaderHandle) const override;
	void destroy(const VertexShaderHandle& shaderHandle) override;
	void destroy(const FragmentShaderHandle& shaderHandle) override;
	void destroy(const TessellationControlShaderHandle& shaderHandle) override;
	void destroy(const TessellationEvaluationShaderHandle& shaderHandle) override;
	ShaderProgramHandle createShaderProgram(const VertexShaderHandle& vertexShaderHandle, const FragmentShaderHandle& fragmentShaderHandle) override;
	ShaderProgramHandle createShaderProgram(
		const VertexShaderHandle& vertexShaderHandle,
		const TessellationControlShaderHandle& tessellationControlShaderHandle,
		const TessellationEvaluationShaderHandle& tessellationEvaluationShaderHandle,
		const FragmentShaderHandle& fragmentShaderHandle
	) override;
	bool valid(const ShaderProgramHandle& shaderProgramHandle) const override;
	void destroy(const ShaderProgramHandle& shaderProgramHandle) override;

	RenderableHandle createRenderable(
		const RenderSceneHandle& renderSceneHandle,
		const MeshHandle& meshHandle,
		const TextureHandle& textureHandle,
		const glm::vec3& position,
		const glm::quat& orientation,
		const glm::vec3& scale = glm::vec3(1.0f),
		const ShaderProgramHandle& shaderProgramHandle = ShaderProgramHandle()
	) override;
	RenderableHandle createRenderable(
		const RenderSceneHandle& renderSceneHandle,
		const MeshHandle& meshHandle,
		const MaterialHandle& materialHandle,
		const glm::vec3& position,
		const glm::quat& orientation,
		const glm::vec3& scale = glm::vec3(1.0f)
	) override;
    bool valid(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const override;
	void destroy(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) override;

	TerrainRenderableHandle createTerrainRenderable(
		const RenderSceneHandle& renderSceneHandle,
		const TerrainHandle& terrainHandle
	) override;
    bool valid(const RenderSceneHandle& renderSceneHandle,
               const TerrainRenderableHandle& terrainRenderableHandle) const override;
	void destroy(const RenderSceneHandle& renderSceneHandle, const TerrainRenderableHandle& terrainRenderableHandle) override;

	SkyboxRenderableHandle createSkyboxRenderable(const RenderSceneHandle& renderSceneHandle, const SkyboxHandle& skyboxHandle) override;
    bool valid(const RenderSceneHandle& renderSceneHandle,
               const SkyboxRenderableHandle& skyboxRenderableHandle) const override;
	void destroy(const RenderSceneHandle& renderSceneHandle, const SkyboxRenderableHandle& skyboxRenderableHandle) override;

	void rotate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::quat& quaternion, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;
	void rotate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 degrees, const glm::vec3& axis, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;
	void rotate(const CameraHandle& cameraHandle, const glm::quat& quaternion, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;
	void rotate(const CameraHandle& cameraHandle, const float32 degrees, const glm::vec3& axis, const TransformSpace& relativeTo = TransformSpace::TS_LOCAL) override;

	void rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::quat& quaternion) override;
	void rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 degrees, const glm::vec3& axis) override;
	glm::quat rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const override;
	void rotation(const CameraHandle& cameraHandle, const glm::quat& quaternion) override;
	void rotation(const CameraHandle& cameraHandle, const float32 degrees, const glm::vec3& axis) override;
	glm::quat rotation(const CameraHandle& cameraHandle) const override;

	void translate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z) override;
	void translate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& trans) override;
	void translate(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const float32 x, const float32 y, const float32 z) override;
	void translate(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const glm::vec3& trans) override;
	void translate(const CameraHandle& cameraHandle, const float32 x, const float32 y, const float32 z) override;
	void translate(const CameraHandle& cameraHandle, const glm::vec3& trans) override;

	void scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z) override;
	void scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& scale) override;
	void scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 scale) override;
	glm::vec3 scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const override;

	void position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z) override;
	void position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& position) override;
	glm::vec3 position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const override;
	void position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const float32 x, const float32 y, const float32 z) override;
	void position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const glm::vec3& position) override;
	glm::vec3 position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle) const override;
	void position(const CameraHandle& cameraHandle, const float32 x, const float32 y, const float32 z) override;
	void position(const CameraHandle& cameraHandle, const glm::vec3& position) override;
	glm::vec3 position(const CameraHandle& cameraHandle) const override;

	void lookAt(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& lookAt) override;
	void lookAt(const CameraHandle& cameraHandle, const glm::vec3& lookAt) override;

	void assign(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const SkeletonHandle& skeletonHandle) override;

	void update(
		const RenderSceneHandle& renderSceneHandle,
		const RenderableHandle& renderableHandle,
		const BonesHandle& bonesHandle,
		const std::vector<glm::mat4>& transformations
	) override;

	void setMouseRelativeMode(const bool enabled) override;
	void setWindowGrab(const bool enabled) override;
    bool cursorVisible() const override;
    void setCursorVisible(const bool visible) override;

	void processEvents() override;
	void addEventListener(IEventListener* eventListener) override;
	void removeEventListener(IEventListener* eventListener) override;

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

	glm::mat4 model_ = glm::mat4(1.0f);
	glm::mat4 view_ = glm::mat4(1.0f);
	glm::mat4 projection_ = glm::mat4(1.0f);

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
