#include <algorithm>
#include <sstream>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <system_error>

#include "gl33/OpenGlRenderer.hpp"

#include "detail/Assert.hpp"
#include "detail/GenerateVertices.hpp"
#include "detail/GenerateCube.hpp"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtx/quaternion.hpp"
#include <glm/gtx/string_cast.hpp>

namespace ice_engine
{
namespace graphics
{
namespace opengl_renderer
{
namespace gl33
{

/**
 * Will return the OpenGL compatible format of the given image Format 'format'.
 *
 * If no known compatible OpenGL format is found, FORMAT_UNKNOWN is returned.
 *
 * @param format
 *
 * @return The OpenGL compatible format of the given image Format 'format', or FORMAT_UNKNOWN if no known compatible OpenGL format is found.
 */
inline GLint getOpenGlImageFormat( int32 format )
{
	switch (format)
	{
		case IImage::Format::FORMAT_RGB:
			return GL_RGB;

		case IImage::Format::FORMAT_RGBA:
			return GL_RGBA;

		case IImage::Format::FORMAT_UNKNOWN:
			return (GLint)IImage::Format::FORMAT_UNKNOWN;
	}

	return (GLint)IImage::Format::FORMAT_UNKNOWN;
}

ShaderProgramHandle lineShaderProgramHandle_;
ShaderProgramHandle lightingShaderProgramHandle_;
ShaderProgramHandle skyboxShaderProgramHandle_;
ShaderProgramHandle deferredLightingGeometryPassProgramHandle_;
ShaderProgramHandle deferredLightingTerrainGeometryPassProgramHandle_;
FrameBuffer frameBuffer_;
Texture2d positionTexture_;
Texture2d normalTexture_;
Texture2d albedoTexture_;
Texture2d metallicRoughnessAmbientOcclusionTexture_;
RenderBuffer renderBuffer_;

ShaderProgramHandle shadowMappingShaderProgramHandle_;
FrameBuffer shadowMappingFrameBuffer_;
Texture2d shadowMappingDepthMapTexture_;

ShaderProgramHandle depthDebugShaderProgramHandle_;
uint depthBufferWidth = 1024;
uint depthBufferHeight = 1024;

const unsigned int NR_LIGHTS = 6;
std::vector<glm::vec3> lightPositions_;
std::vector<glm::vec3> lightColors_;

OpenGlRenderer::OpenGlRenderer(utilities::Properties* properties, fs::IFileSystem* fileSystem, logger::ILogger* logger)
	:
	properties_(properties),
	fileSystem_(fileSystem),
	logger_(logger)
{
	initialize();
}

OpenGlRenderer::~OpenGlRenderer()
{
	if (openglContext_)
	{
		SDL_GL_DeleteContext(openglContext_);
		openglContext_ = nullptr;
	}

	if (sdlWindow_)
	{
		SDL_SetWindowFullscreen( sdlWindow_, 0 );
		SDL_DestroyWindow( sdlWindow_ );
		sdlWindow_ = nullptr;
	}

	SDL_Quit();
}

void OpenGlRenderer::initialize()
{
	width_ = properties_->getIntValue(std::string("window.width"), 1024);
	height_ = properties_->getIntValue(std::string("window.height"), 768);

	LOG_INFO(logger_, std::string("Width and height set to ") + std::to_string(width_) + " x " + std::to_string(height_));

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		auto message = std::string("Unable to initialize SDL: ") + SDL_GetError();
		throw std::runtime_error(message);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	auto windowTitle = properties_->getStringValue("window.title", "Ice Engine");

	uint32 flags = SDL_WINDOW_OPENGL;

	if (properties_->getBoolValue("window.fullscreen", false)) flags |= SDL_WINDOW_FULLSCREEN;
	if (properties_->getBoolValue("window.resizable", false)) flags |= SDL_WINDOW_RESIZABLE;
	if (properties_->getBoolValue("window.maximized", false)) flags |= SDL_WINDOW_MAXIMIZED;

	sdlWindow_ = SDL_CreateWindow(windowTitle.c_str(), 50, 50, width_, height_, flags);

	if (sdlWindow_ == nullptr) throw std::runtime_error(std::string("Unable to create window: ") + SDL_GetError());

	openglContext_ = SDL_GL_CreateContext(sdlWindow_);

	if (openglContext_ == nullptr) throw std::runtime_error(std::string("Unable to create OpenGL context: ") + SDL_GetError());

	const bool vsync = properties_->getBoolValue("window.vsync", false);
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);

	glewExperimental = GL_TRUE; // Needed in core profile

	if (glewInit() != GLEW_OK) throw std::runtime_error("Failed to initialize GLEW.");

	SDL_GL_GetDrawableSize(sdlWindow_, reinterpret_cast<int*>(&width_), reinterpret_cast<int*>(&height_));

	// Set up the model, view, and projection matrices
	//model_ = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
	model_ = glm::mat4(1.0f);
	view_ = glm::mat4(1.0f);
	setViewport(width_, height_);

	initializeOpenGlShaderPrograms();

	// TEST lights
	// Source: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/8.1.deferred_shading/deferred_shading.cpp
	srand(13);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = ((rand() % 100) / 100.0f) * 6.0f - 3.0f;
		float yPos = ((rand() % 100) / 100.0f) * 6.0f - 4.0f;
		float zPos = ((rand() % 100) / 100.0f) * 6.0f - 3.0f;
		lightPositions_.push_back(glm::vec3(xPos, yPos, zPos));
		// also calculate random color
		float rColor = ((rand() % 100) / 200.0f) + 0.5f; // between 0.5 and 1.0
		float gColor = ((rand() % 100) / 200.0f) + 0.5f; // between 0.5 and 1.0
		float bColor = ((rand() % 100) / 200.0f) + 0.5f; // between 0.5 and 1.0
		lightColors_.push_back(glm::vec3(rColor, gColor, bColor));
	}

	initializeOpenGlBuffers();
}

void OpenGlRenderer::initializeOpenGlShaderPrograms()
{
	auto lineVertexShaderHandle = createVertexShader(loadShaderContents("line.vert"));
	auto lineFragmentShaderHandle = createFragmentShader(loadShaderContents("line.frag"));

	lineShaderProgramHandle_ = createShaderProgram(lineVertexShaderHandle, lineFragmentShaderHandle);

	// Shadow mapping shader program
	auto shadowMappingVertexShaderHandle = createVertexShader(loadShaderContents("shadow_mapping.vert"));
	auto shadowMappingFragmentShaderHandle = createFragmentShader(loadShaderContents("shadow_mapping.frag"));

	shadowMappingShaderProgramHandle_ = createShaderProgram(shadowMappingVertexShaderHandle, shadowMappingFragmentShaderHandle);

	// deferred lighting geometry pass shader program
	auto deferredLightingGeometryPassVertexShaderHandle = createVertexShader(loadShaderContents("deferred_lighting_geometry_pass.vert"));
	auto deferredLightingGeometryPassFragmentShaderHandle = createFragmentShader(loadShaderContents("deferred_lighting_geometry_pass.frag"));

	deferredLightingGeometryPassProgramHandle_ = createShaderProgram(deferredLightingGeometryPassVertexShaderHandle, deferredLightingGeometryPassFragmentShaderHandle);

	// deferred lighting terrain geometry pass shader program
	auto deferredLightingTerrainGeometryPassVertexShaderHandle = createVertexShader(loadShaderContents("deferred_lighting_terrain_geometry_pass.vert"));
	auto deferredLightingTerrainGeometryPassFragmentShaderHandle = createFragmentShader(loadShaderContents("deferred_lighting_terrain_geometry_pass.frag"));

	deferredLightingTerrainGeometryPassProgramHandle_ = createShaderProgram(deferredLightingTerrainGeometryPassVertexShaderHandle, deferredLightingTerrainGeometryPassFragmentShaderHandle);

	// Lighting shader program
	auto lightingVertexShaderHandle = createVertexShader(loadShaderContents("lighting.vert"));
	auto lightingFragmentShaderHandle = createFragmentShader(loadShaderContents("lighting.frag"));

	lightingShaderProgramHandle_ = createShaderProgram(lightingVertexShaderHandle, lightingFragmentShaderHandle);

	// Skybox shader program
	auto skyboxVertexShaderHandle = createVertexShader(loadShaderContents("skybox.vert"));
	auto skyboxFragmentShaderHandle = createFragmentShader(loadShaderContents("skybox.frag"));

	skyboxShaderProgramHandle_ = createShaderProgram(skyboxVertexShaderHandle, skyboxFragmentShaderHandle);

	std::string depthDebugVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
)";

	// Source: https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/3.1.2.shadow_mapping_base/3.1.2.shadow_mapping_depth.fs
	std::string depthDebugFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;
uniform float near_plane;
uniform float far_plane;

// required when using a perspective projection matrix
float LinearizeDepth(const float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main()
{
    float depthValue = texture(depthMap, TexCoords).r;
    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}
)";

	auto depthDebugVertexShaderHandle = createVertexShader(depthDebugVertexShader);
	auto depthDebugFragmentShaderHandle = createFragmentShader(depthDebugFragmentShader);

	depthDebugShaderProgramHandle_ = createShaderProgram(depthDebugVertexShaderHandle, depthDebugFragmentShaderHandle);
}

void OpenGlRenderer::initializeOpenGlBuffers()
{
	// G-buffer
	positionTexture_ = Texture2d();
	positionTexture_.generate(GL_RGB16F, width_, height_, GL_RGB, GL_FLOAT, nullptr);
	positionTexture_.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    normalTexture_ = Texture2d();
	normalTexture_.generate(GL_RGB16F, width_, height_, GL_RGB, GL_FLOAT, nullptr);
	normalTexture_.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    albedoTexture_ = Texture2d();
	albedoTexture_.generate(GL_RGBA, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	albedoTexture_.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    metallicRoughnessAmbientOcclusionTexture_ = Texture2d();
	metallicRoughnessAmbientOcclusionTexture_.generate(GL_RGB, width_, height_, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	metallicRoughnessAmbientOcclusionTexture_.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	frameBuffer_ = FrameBuffer();
	frameBuffer_.generate();
	frameBuffer_.attach(positionTexture_);
	frameBuffer_.attach(normalTexture_);
	frameBuffer_.attach(albedoTexture_);
	frameBuffer_.attach(metallicRoughnessAmbientOcclusionTexture_);

	// Shadow mapping
	shadowMappingDepthMapTexture_ = Texture2d();
	shadowMappingDepthMapTexture_.generate(GL_DEPTH_COMPONENT, depthBufferWidth, depthBufferHeight, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	shadowMappingDepthMapTexture_.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	shadowMappingFrameBuffer_ = FrameBuffer();
	shadowMappingFrameBuffer_.generate();
	shadowMappingFrameBuffer_.attach(shadowMappingDepthMapTexture_, GL_DEPTH_ATTACHMENT);
	FrameBuffer::drawBuffer(GL_NONE);
	FrameBuffer::readBuffer(GL_NONE);
	FrameBuffer::unbind();

	renderBuffer_ = RenderBuffer();
	renderBuffer_.generate();
	renderBuffer_.setStorage(GL_DEPTH_COMPONENT, width_, height_);

	frameBuffer_.attach(renderBuffer_, GL_DEPTH_ATTACHMENT);
}

void OpenGlRenderer::setViewport(const uint32 width, const uint32 height)
{
	width_ = width;
	height_ = height;

	projection_ = glm::perspective(glm::radians(60.0f), (float32)width / (float32)height, 0.1f, 500.f);

	glViewport(0, 0, width_, height_);

	if (renderBuffer_)
	{
		initializeOpenGlBuffers();
	}
}

glm::uvec2 OpenGlRenderer::getViewport() const
{
	return glm::uvec2(width_, height_);
}

glm::mat4 OpenGlRenderer::getModelMatrix() const
{
	return model_;
}

glm::mat4 OpenGlRenderer::getViewMatrix() const
{
	return view_;
}

glm::mat4 OpenGlRenderer::getProjectionMatrix() const
{
	return projection_;
}

void OpenGlRenderer::beginRender()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);

	// Setup camera
	const glm::quat temp = glm::conjugate(camera_.orientation);

	view_ = glm::mat4_cast(temp);
	view_ = glm::translate(view_, glm::vec3(-camera_.position.x, -camera_.position.y, -camera_.position.z));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
//glm::vec3 lPos = glm::vec3(1.0f, 4.0f, 1.0f);
glm::vec3 ambient = glm::vec3(0.2f, 0.2f, 0.2f);
glm::vec3 diffuse = glm::vec3(0.2f, 0.2f, 0.2f);
glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
void OpenGlRenderer::render(const RenderSceneHandle& renderSceneHandle)
{
	int modelMatrixLocation = 0;
	int pvmMatrixLocation = 0;
	int normalMatrixLocation = 0;

	//assert( modelMatrixLocation >= 0);
	//assert( pvmMatrixLocation >= 0);
	//assert( normalMatrixLocation >= 0);

	auto& renderScene = renderSceneHandles_[renderSceneHandle];

	// Rendered depth from lights perspective
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plane = -10.0f, far_plane = 100.0f;
	const float lightProjectionSize = 20.0f;
	lightProjection = glm::ortho(-lightProjectionSize, lightProjectionSize, -lightProjectionSize, lightProjectionSize, near_plane, far_plane);
	const glm::vec3 lightPos = (direction * -1.0f) + camera_.position;
	const glm::vec3 lightLookAt = camera_.position;
	//lightView = glm::lookAt(lightPos, glm::vec3(9.0f, 0.0f, -2.8f), glm::vec3(0.0, 1.0, 0.0));
	lightView = glm::lookAt(lightPos, lightLookAt, glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;

	// render scene from light's point of view
	auto& shadowMappingShaderProgram = shaderPrograms_[shadowMappingShaderProgramHandle_];
	shadowMappingShaderProgram.use();
	glUniformMatrix4fv(glGetUniformLocation(shadowMappingShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

	glViewport(0, 0, depthBufferWidth, depthBufferHeight);

	shadowMappingFrameBuffer_.bind();
	Texture2d::activate(0);

	{
		modelMatrixLocation = glGetUniformLocation(shadowMappingShaderProgram, "modelMatrix");

		ASSERT_GL_ERROR();

		glClear(GL_DEPTH_BUFFER_BIT);

		for (const auto& r : renderScene.renderables)
		{
			glm::mat4 newModel = glm::translate(model_, r.graphicsData.position);
			newModel = newModel * glm::mat4_cast( r.graphicsData.orientation );
			newModel = glm::scale(newModel, r.graphicsData.scale);

			// Send uniform variable values to the shader
			glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &newModel[0][0]);

			if (r.ubo.id > 0)
			{
//				const int bonesLocation = glGetUniformLocation(shadowMappingShaderProgram, "bones");
//				assert( bonesLocation >= 0);
//				glBindBufferBase(GL_UNIFORM_BUFFER, bonesLocation, r.ubo.id);
//				glBindBufferBase(GL_UNIFORM_BUFFER, 0, r.ubo.id);

				ASSERT_GL_ERROR();
			}

			auto& texture = texture2ds_[r.textureHandle];
			texture.bind();

			glBindVertexArray(r.vao.id);
			glDrawElements(r.vao.ebo.mode, r.vao.ebo.count, r.vao.ebo.type, 0);
			glBindVertexArray(0);

			ASSERT_GL_ERROR();
		}
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, woodTexture);
		//renderScene(simpleDepthShader);
	}
	FrameBuffer::unbind();

	// reset viewport
	glViewport(0, 0, width_, height_);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ASSERT_GL_ERROR();

	// Geometry pass
	frameBuffer_.bind();

	Texture2d::activate(0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//const auto& renderScene = renderSceneHandles_[renderSceneHandle];

	//auto& shaderProgram = shaderPrograms_[renderScene.shaderProgramHandle];
	auto& deferredLightingGeometryPassShaderProgram = shaderPrograms_[deferredLightingGeometryPassProgramHandle_];
	deferredLightingGeometryPassShaderProgram.use();
	modelMatrixLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "modelMatrix");
	pvmMatrixLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "pvmMatrix");
	normalMatrixLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "normalMatrix");
	auto hasBonesLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "hasBones");
	auto hasBoneAttachmentLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "hasBoneAttachment");
	auto boneAttachmentIdsLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "boneAttachmentIds");
	auto boneAttachmentWeightsLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "boneAttachmentWeights");

	glUniform1i(glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "texture_diffuse1"), 0);
	//glUniform1i(glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "albedoTextures"), 1);
	glUniform1i(glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "normalTextures"), 1);
	glUniform1i(glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "metallicRoughnessAmbientOcclusionTextures"), 2);

	ASSERT_GL_ERROR();

	for (const auto& r : renderScene.renderables)
	{
		glm::mat4 newModel = glm::translate(model_, r.graphicsData.position);
		newModel = newModel * glm::mat4_cast( r.graphicsData.orientation );
		newModel = glm::scale(newModel, r.graphicsData.scale);

		// Send uniform variable values to the shader
		const glm::mat4 pvmMatrix(projection_ * view_ * newModel);
		glUniformMatrix4fv(pvmMatrixLocation, 1, GL_FALSE, &pvmMatrix[0][0]);

		glm::mat3 normalMatrix = glm::inverse(glm::transpose(glm::mat3(view_ * newModel)));
		glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, &normalMatrix[0][0]);

		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &newModel[0][0]);

		if (r.ubo.id == 0)
		{
			glUniform1i(hasBonesLocation, 0);
			glUniform1i(hasBoneAttachmentLocation, 0);
		}
		else
		{
			glUniform1i(hasBonesLocation, r.hasBones);
			glUniform1i(hasBoneAttachmentLocation, r.hasBoneAttachment);

			const int bonesLocation = glGetUniformBlockIndex(deferredLightingGeometryPassShaderProgram, "Bones");
			ASSERT(bonesLocation >= 0);
			glBindBufferBase(GL_UNIFORM_BUFFER, bonesLocation, r.ubo.id);
//			glBindBufferBase(GL_UNIFORM_BUFFER, 0, r.ubo.id);

			ASSERT_GL_ERROR();

			if (r.hasBoneAttachment)
			{
				glUniform4iv(boneAttachmentIdsLocation, 1, &r.boneIds[0]);
				glUniform4fv(boneAttachmentWeightsLocation, 1, &r.boneWeights[0]);
			}
		}

		if (r.textureHandle)
		{
			Texture2d::activate(0);
			auto& texture = texture2ds_[r.textureHandle];
			texture.bind();
		}
		else if (r.materialHandle)
		{
			auto& material = materials_[r.materialHandle];
			Texture2d::activate(0);
			material.albedo.bind();
			Texture2d::activate(1);
			material.normal.bind();
			Texture2d::activate(2);
			material.metallicRoughnessAmbientOcclusion.bind();
		}


		glBindVertexArray(r.vao.id);
		glDrawElements(r.vao.ebo.mode, r.vao.ebo.count, r.vao.ebo.type, 0);
		glBindVertexArray(0);

		ASSERT_GL_ERROR();
	}

	// Terrain
	auto& deferredLightingTerrainGeometryPassShaderProgram = shaderPrograms_[deferredLightingTerrainGeometryPassProgramHandle_];
	deferredLightingTerrainGeometryPassShaderProgram.use();

	ASSERT(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "heightMapTexture") >= 0);
	ASSERT(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "terrainMapTexture") >= 0);
	ASSERT(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "splatMapAlbedoTextures") >= 0);

	glUniform1i(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "heightMapTexture"), 0);
	glUniform1i(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "terrainMapTexture"), 1);
	glUniform1i(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "splatMapAlbedoTextures"), 2);
	glUniform1i(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "splatMapNormalTextures"), 3);
	glUniform1i(glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "splatMapMetallicRoughnessAmbientOcclusionTextures"), 4);

	modelMatrixLocation = glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "modelMatrix");
	pvmMatrixLocation = glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "pvmMatrix");
	//normalMatrixLocation = glGetUniformLocation(deferredLightingTerrainGeometryPassShaderProgram, "normalMatrix");

	ASSERT(modelMatrixLocation >= 0);
	ASSERT(pvmMatrixLocation >= 0);
	//ASSERT(normalMatrixLocation >= 0);

	ASSERT_GL_ERROR();

	for (auto& t : renderScene.terrain)
	{
		glm::mat4 newModel = glm::translate(model_, t.graphicsData.position);
		newModel = newModel * glm::mat4_cast( t.graphicsData.orientation );
		newModel = glm::scale(newModel, t.graphicsData.scale);

		// Send uniform variable values to the shader
		const glm::mat4 pvmMatrix(projection_ * view_ * newModel);
		glUniformMatrix4fv(pvmMatrixLocation, 1, GL_FALSE, &pvmMatrix[0][0]);

		//glm::mat3 normalMatrix = glm::inverse(glm::transpose(glm::mat3(view_ * newModel)));
		//glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, &normalMatrix[0][0]);

		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &newModel[0][0]);

		if (t.ubo.id > 0)
		{
			//const int bonesLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "bones");
			//assert( bonesLocation >= 0);
			//glBindBufferBase(GL_UNIFORM_BUFFER, bonesLocation, r.ubo.id);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, t.ubo.id);
		}

		auto& terrain = terrains_[t.terrainHandle];

		Texture2d::activate(0);
		auto& texture = texture2ds_[terrain.textureHandle];
		texture.bind();

		Texture2d::activate(1);
		auto& terrainMapTexture = texture2ds_[terrain.terrainMapTextureHandle];
		terrainMapTexture.bind();

		Texture2dArray::activate(2);
		terrain.splatMapTexture2dArrays[0].bind();

		Texture2dArray::activate(3);
		terrain.splatMapTexture2dArrays[1].bind();

		Texture2dArray::activate(4);
		terrain.splatMapTexture2dArrays[2].bind();

		glBindVertexArray(t.vao.id);
		glDrawElements(t.vao.ebo.mode, t.vao.ebo.count, t.vao.ebo.type, 0);
		glBindVertexArray(0);

		ASSERT_GL_ERROR();
	}

	FrameBuffer::unbind();

	ASSERT_GL_ERROR();

	// Lighting pass
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto& lightingShaderProgram = shaderPrograms_[lightingShaderProgramHandle_];
	lightingShaderProgram.use();

	glUniform1i(glGetUniformLocation(lightingShaderProgram, "gPosition"), 0);
	glUniform1i(glGetUniformLocation(lightingShaderProgram, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(lightingShaderProgram, "gAlbedoSpec"), 2);
	glUniform1i(glGetUniformLocation(lightingShaderProgram, "gMetallicRoughnessAmbientOcclusion"), 3);
	glUniform1i(glGetUniformLocation(lightingShaderProgram, "shadowMap"), 4);
	glUniform3fv(glGetUniformLocation(lightingShaderProgram, "viewPos"), 1, &camera_.position[0]);
	glUniform3fv(glGetUniformLocation(lightingShaderProgram, "lightPos"), 1, &lightPos[0]);
	glUniformMatrix4fv(glGetUniformLocation(lightingShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

	Texture2d::activate(0);
	positionTexture_.bind();
	Texture2d::activate(1);
	normalTexture_.bind();
	Texture2d::activate(2);
	albedoTexture_.bind();
	Texture2d::activate(3);
	metallicRoughnessAmbientOcclusionTexture_.bind();
	Texture2d::activate(4);
	shadowMappingDepthMapTexture_.bind();

	for (const auto& light : renderScene.pointLights)
	{
		//glm::vec4 newPos = model_ * glm::vec4(lightPositions_[i], 1.0);

		glUniform3fv(glGetUniformLocation(lightingShaderProgram, ("lights[" + std::to_string(0) + "].Position").c_str()), 1, &light.position.x);
		glUniform3fv(glGetUniformLocation(lightingShaderProgram, ("lights[" + std::to_string(0) + "].Color").c_str()), 1, &lightColors_[0].x);
		// update attenuation parameters and calculate radius
		const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
		//const float linear = 0.7f;
		//const float quadratic = 1.8f;
		const float linear = 0.05f;
		const float quadratic = 0.05f;
		glUniform1f(glGetUniformLocation(lightingShaderProgram, ("lights[" + std::to_string(0) + "].Linear").c_str()), linear);
		glUniform1f(glGetUniformLocation(lightingShaderProgram, ("lights[" + std::to_string(0) + "].Quadratic").c_str()), quadratic);
	}

	//glm::vec4 newPos = model_ * glm::vec4(lightPositions_[i], 1.0);

	glUniform3fv(glGetUniformLocation(lightingShaderProgram, ("directionalLights[" + std::to_string(0) + "].direction").c_str()), 1, &direction.x);
	glUniform3fv(glGetUniformLocation(lightingShaderProgram, ("directionalLights[" + std::to_string(0) + "].ambient").c_str()), 1, &ambient.x);
	glUniform3fv(glGetUniformLocation(lightingShaderProgram, ("directionalLights[" + std::to_string(0) + "].diffuse").c_str()), 1, &diffuse.x);
	glUniform3fv(glGetUniformLocation(lightingShaderProgram, ("directionalLights[" + std::to_string(0) + "].specular").c_str()), 1, &specular.x);

	renderQuad();

	// copy geometry depth buffer to default frame buffers depth buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	FrameBuffer::unbind();

	// Skybox pass
	// glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	auto& skyboxShaderProgram = shaderPrograms_[skyboxShaderProgramHandle_];
	skyboxShaderProgram.use();

	auto projectionMatrixLocation = glGetUniformLocation(skyboxShaderProgram, "projectionMatrix");
	auto viewMatrixLocation = glGetUniformLocation(skyboxShaderProgram, "viewMatrix");

	ASSERT(projectionMatrixLocation >= 0);
	ASSERT(viewMatrixLocation >= 0);

	ASSERT_GL_ERROR();

	for (auto& s : renderScene.skyboxes)
	{
		// glm::mat4 newModel = glm::translate(model_, s.graphicsData.position);
		// newModel = newModel * glm::mat4_cast( s.graphicsData.orientation );
		// newModel = glm::scale(newModel, s.graphicsData.scale);

		// Send uniform variable values to the shader
		// const glm::mat4 pvmMatrix(projection_ * view_ * newModel);
		// glUniformMatrix4fv(pvmMatrixLocation, 1, GL_FALSE, &pvmMatrix[0][0]);

		//glm::mat3 normalMatrix = glm::inverse(glm::transpose(glm::mat3(view_ * newModel)));
		//glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, &normalMatrix[0][0]);

		// original source: https://learnopengl.com/Advanced-OpenGL/Cubemaps
		const glm::mat4 newView = glm::mat4(glm::mat3(view_));
		glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projection_[0][0]);
		glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &newView[0][0]);

		// if (s.ubo.id > 0)
		// {
		// 	//const int bonesLocation = glGetUniformLocation(deferredLightingGeometryPassShaderProgram, "bones");
		// 	//assert( bonesLocation >= 0);
		// 	//glBindBufferBase(GL_UNIFORM_BUFFER, bonesLocation, r.ubo.id);
		// 	glBindBufferBase(GL_UNIFORM_BUFFER, 0, t.ubo.id);
		// }

		auto& skybox = skyboxes_[s.skyboxHandle];

		TextureCubeMap::activate(0);
		skybox.textureCubeMap.bind();

		glBindVertexArray(s.vao.id);
		glDrawElements(s.vao.ebo.mode, s.vao.ebo.count, s.vao.ebo.type, 0);
		glBindVertexArray(0);

		ASSERT_GL_ERROR();
	}

	// FrameBuffer::unbind();

	// glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	ASSERT_GL_ERROR();

	/*
	// Debug draw
	auto& depthDebugShaderProgram = shaderPrograms_[depthDebugShaderProgramHandle_];
	depthDebugShaderProgram.use();

	glUniform1f(glGetUniformLocation(depthDebugShaderProgram, "near_plane"), near_plane);
	glUniform1f(glGetUniformLocation(depthDebugShaderProgram, "far_plane"), far_plane);

	Texture2d::activate(0);
	shadowMappingDepthMapTexture_.bind();

	renderQuad();
	*/
	// Render lights?

	ASSERT_GL_ERROR();
}

GLuint VBO, VAO;
size_t lastSize = 0;
void OpenGlRenderer::renderLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), &from.x);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3), sizeof(glm::vec3), &to.x);
	glBufferSubData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec3), sizeof(glm::vec3), &color.x);
	glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(glm::vec3), sizeof(glm::vec3), &color.x);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(2 * sizeof(glm::vec3)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	auto& lineShaderProgram = shaderPrograms_[lineShaderProgramHandle_];
	auto projectionMatrixLocation = glGetUniformLocation(lineShaderProgram, "projectionMatrix");
	auto viewMatrixLocation = glGetUniformLocation(lineShaderProgram, "viewMatrix");

	lineShaderProgram.use();

	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projection_[0][0]);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &view_[0][0]);

	glBindVertexArray(VAO);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
}

void OpenGlRenderer::renderLines(const std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3>>& lineData)
{
	std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3, glm::vec3>> lineData2;

	for (const auto& line : lineData)
	{
		lineData2.push_back( std::tuple<glm::vec3, glm::vec3, glm::vec3, glm::vec3>(std::get<0>(line), std::get<1>(line), std::get<2>(line), std::get<2>(line)) );
	}

	auto size = lineData2.size() * (4 * sizeof(glm::vec3));
	if (!VBO || size > lastSize)
    {
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    if (size > lastSize)
    {
        glBufferData(GL_ARRAY_BUFFER, size, &lineData2[0], GL_DYNAMIC_DRAW);

        lastSize = size;
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, lineData2.size() * (4 * sizeof(glm::vec3)), &lineData2[0]);
    }

//	glBindVertexArray(VAO);
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferData(GL_ARRAY_BUFFER, size, &lineData2[0], GL_DYNAMIC_DRAW);

//	glBufferSubData(GL_ARRAY_BUFFER, 0, lineData2.size() * 4 * sizeof(glm::vec3), &lineData2[0]);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(2 * sizeof(glm::vec3)));
	glEnableVertexAttribArray(1);
//	glBindVertexArray(0);

	auto& lineShaderProgram = shaderPrograms_[lineShaderProgramHandle_];
	auto projectionMatrixLocation = glGetUniformLocation(lineShaderProgram, "projectionMatrix");
	auto viewMatrixLocation = glGetUniformLocation(lineShaderProgram, "viewMatrix");

	lineShaderProgram.use();

	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projection_[0][0]);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &view_[0][0]);

//	glBindVertexArray(VAO);
	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(4 * lineData2.size()));
	glBindVertexArray(0);
}

void OpenGlRenderer::endRender()
{
	SDL_GL_SwapWindow(sdlWindow_);
}

RenderSceneHandle OpenGlRenderer::createRenderScene()
{
	return renderSceneHandles_.create();
}

void OpenGlRenderer::destroyRenderScene(const RenderSceneHandle& renderSceneHandle)
{
	renderSceneHandles_.destroy(renderSceneHandle);
}

CameraHandle OpenGlRenderer::createCamera(const glm::vec3& position, const glm::vec3& lookAt)
{
	camera_ = Camera();
	camera_.position = position;
	camera_.orientation = glm::quat();
	//camera_.orientation = glm::normalize(camera_.orientation);

	CameraHandle cameraHandle = CameraHandle(0, 1);

	this->lookAt(cameraHandle, lookAt);

	return cameraHandle;
}

PointLightHandle OpenGlRenderer::createPointLight(const RenderSceneHandle& renderSceneHandle, const glm::vec3& position)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto handle = renderScene.pointLights.create();
	auto& light = renderScene.pointLights[handle];

	light.position = position;
	light.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	light.orientation = glm::quat();

	return handle;
}

void OpenGlRenderer::destroy(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	renderScene.pointLights.destroy(pointLightHandle);
}

MeshHandle OpenGlRenderer::createStaticMesh(
	const std::vector<glm::vec3>& vertices,
	const std::vector<uint32>& indices,
	const std::vector<glm::vec4>& colors,
	const std::vector<glm::vec3>& normals,
	const std::vector<glm::vec2>& textureCoordinates
)
{
	auto handle = meshes_.create();
	auto& vao = meshes_[handle];

	glGenVertexArrays(1, &vao.id);
	glGenBuffers(1, &vao.vbo[0].id);
	glGenBuffers(1, &vao.ebo.id);

	auto size = vertices.size() * sizeof(glm::vec3);
	size += colors.size() * sizeof(glm::vec4);
	size += normals.size() * sizeof(glm::vec3);
	size += textureCoordinates.size() * sizeof(glm::vec2);

	glBindVertexArray(vao.id);
	glBindBuffer(GL_ARRAY_BUFFER, vao.vbo[0].id);
	glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

    GLintptr offset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, offset, vertices.size() * sizeof(glm::vec3), &vertices[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	offset += static_cast<GLintptr>(vertices.size() * sizeof(glm::vec3));
	glBufferSubData(GL_ARRAY_BUFFER, offset, colors.size() * sizeof(glm::vec4), &colors[0]);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset));
	glEnableVertexAttribArray(1);

	offset += static_cast<GLintptr>(colors.size() * sizeof(glm::vec4));
	glBufferSubData(GL_ARRAY_BUFFER, offset, normals.size() * sizeof(glm::vec3), &normals[0]);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset));
	glEnableVertexAttribArray(2);

	offset += static_cast<GLintptr>(normals.size() * sizeof(glm::vec3));
	glBufferSubData(GL_ARRAY_BUFFER, offset, textureCoordinates.size() * sizeof(glm::vec2), &textureCoordinates[0]);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao.ebo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), &indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

	vao.ebo.count = static_cast<GLsizei>(indices.size());
	vao.ebo.mode = GL_TRIANGLES;
	vao.ebo.type =  GL_UNSIGNED_INT;

	return handle;
}

MeshHandle OpenGlRenderer::createStaticMesh(const IMesh* mesh)
{
	return createStaticMesh(mesh->vertices(), mesh->indices(), mesh->colors(), mesh->normals(), mesh->textureCoordinates());
}

MeshHandle OpenGlRenderer::createDynamicMesh(const IMesh* mesh)
{
	/*
	auto handle = meshes_.create();
	auto& vao = meshes_[handle];

	glGenVertexArrays(1, &vao.id);
	glGenBuffers(1, &vao.vbo[0].id);
	glGenBuffers(1, &vao.vbo[1].id);
	glGenBuffers(1, &vao.vbo[2].id);
	glGenBuffers(1, &vao.vbo[3].id);
	glGenBuffers(1, &vao.ebo.id);

	glBindVertexArray(vao.id);
	glBindBuffer(GL_ARRAY_BUFFER, vao.vbo[0].id);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vao.vbo[1].id);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), &colors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vao.vbo[2].id);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, vao.vbo[3].id);
	glBufferData(GL_ARRAY_BUFFER, textureCoordinates.size() * sizeof(glm::vec2), &textureCoordinates[0], GL_STATIC_DRAW);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao.ebo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), &indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

	vao.ebo.count = indices.size();
	vao.ebo.mode = GL_TRIANGLES;
	vao.ebo.type =  GL_UNSIGNED_INT;

	vertexArrayObjects_.push_back(vao);
	auto index = vertexArrayObjects_.size() - 1;

	return handle;
	*/
	return MeshHandle();
}

SkeletonHandle OpenGlRenderer::createSkeleton(const MeshHandle& meshHandle, const ISkeleton* skeleton)
{
//	auto handle = skeletons_.create();
//	auto& ubo = skeletons_[handle];
//
//	glGenBuffers(1, &ubo.id);
//
//	auto size = numberOfBones * sizeof(glm::mat4);
//
//	glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
//	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STREAM_DRAW);
//
//	return handle;

	auto& vao = meshes_[meshHandle];

	if (vao.vbo[1].id != 0) throw std::runtime_error("Skeleton already exists");

	glBindVertexArray(vao.id);
	glGenBuffers(1, &vao.vbo[1].id);

	auto size = skeleton->boneIds().size() * sizeof(glm::ivec4);
	size += skeleton->boneWeights().size() * sizeof(glm::vec4);

	glBindBuffer(GL_ARRAY_BUFFER, vao.vbo[1].id);
	glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

    GLintptr offset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, offset, skeleton->boneIds().size() * sizeof(glm::ivec4), &skeleton->boneIds()[0]);
	glVertexAttribIPointer(4, 4, GL_INT, 0, 0);
	glEnableVertexAttribArray(4);

	offset += static_cast<GLintptr>(skeleton->boneIds().size() * sizeof(glm::vec4));
	glBufferSubData(GL_ARRAY_BUFFER, offset, skeleton->boneWeights().size() * sizeof(glm::vec4), &skeleton->boneWeights()[0]);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset));
	glEnableVertexAttribArray(5);

	glBindVertexArray(0);

//	return handle;

	return SkeletonHandle();
}

void OpenGlRenderer::destroy(const SkeletonHandle& skeletonHandle)
{

}

BonesHandle OpenGlRenderer::createBones(const uint32 maxNumberOfBones)
{
//	auto handle = skeletons_.create();
//	auto& ubo = skeletons_[handle];
//
//	glGenBuffers(1, &ubo.id);
//
//	auto size = numberOfBones * sizeof(glm::mat4);
//
//	glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
//	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STREAM_DRAW);
//
//	return handle;

	if (maxNumberOfBones > 100) throw std::runtime_error("cannot have more than 100 bones");

	auto handle = bones_.create();
	auto& ubo = bones_[handle];

	glGenBuffers(1, &ubo.id);

	auto size = maxNumberOfBones * sizeof(glm::mat4);
	static auto transformations = std::vector<glm::mat4>(maxNumberOfBones, glm::mat4(1.0f));

	glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
	glBufferData(GL_UNIFORM_BUFFER, size, &transformations[0], GL_STREAM_DRAW);

	return handle;
}

void OpenGlRenderer::destroy(const BonesHandle& bonesHandle)
{

}

void OpenGlRenderer::attach(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const BonesHandle& bonesHandle)
{
//	auto handle = skeletons_.create();
//	auto& ubo = skeletons_[handle];
//
//	glGenBuffers(1, &ubo.id);
//
//	auto size = numberOfBones * sizeof(glm::mat4);
//
//	glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
//	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STREAM_DRAW);
//
//	return handle;

	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto& renderable = renderScene.renderables[renderableHandle];
	auto& bones = bones_[bonesHandle];

	renderable.ubo = bones;
	renderable.hasBones = true;
}

void OpenGlRenderer::detach(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const BonesHandle& bonesHandle)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto& renderable = renderScene.renderables[renderableHandle];

	renderable.ubo.id = 0;
	renderable.hasBones = false;
	renderable.hasBoneAttachment = false;
}

void OpenGlRenderer::attachBoneAttachment(
	const RenderSceneHandle& renderSceneHandle,
	const RenderableHandle& renderableHandle,
	const BonesHandle& bonesHandle,
	const glm::ivec4& boneIds,
	const glm::vec4& boneWeights
)
{
//	auto handle = skeletons_.create();
//	auto& ubo = skeletons_[handle];
//
//	glGenBuffers(1, &ubo.id);
//
//	auto size = numberOfBones * sizeof(glm::mat4);
//
//	glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
//	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STREAM_DRAW);
//
//	return handle;

	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto& renderable = renderScene.renderables[renderableHandle];
	auto& bones = bones_[bonesHandle];

	renderable.ubo = bones;

	renderable.boneIds = boneIds;
	renderable.boneWeights = boneWeights;
	renderable.hasBoneAttachment = true;
}

void OpenGlRenderer::detachBoneAttachment(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto& renderable = renderScene.renderables[renderableHandle];

	if (!renderable.hasBones)
	{
		renderable.ubo.id = 0;
	}

	renderable.hasBoneAttachment = true;
}

TextureHandle OpenGlRenderer::createTexture2d(const ITexture* texture)
{
	auto handle = texture2ds_.create();
	auto& texture2d = texture2ds_[handle];

	auto format = getOpenGlImageFormat(texture->image()->format());

	texture2d.generate(format, texture->image()->width(), texture->image()->height(), format, GL_UNSIGNED_BYTE, &texture->image()->data()[0], true);
	//glGenTextures(1, &texture.id);
	//glBindTexture(GL_TEXTURE_2D, texture.id);
	//glTexImage2D(GL_TEXTURE_2D, 0, format, image->width(), image->height(), 0, format, GL_UNSIGNED_BYTE, &image->data()[0]);
	//glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);

	return handle;
}

MaterialHandle OpenGlRenderer::createMaterial(const IPbrMaterial* pbrMaterial)
{
	auto handle = materials_.create();
	auto& material = materials_[handle];

	material.albedo = Texture2d();
	material.albedo.generate(GL_RGBA, pbrMaterial->albedo()->width(), pbrMaterial->albedo()->height(), GL_RGBA, GL_UNSIGNED_BYTE, &pbrMaterial->albedo()->data()[0], true);
	material.normal = Texture2d();
	material.normal.generate(GL_RGBA, pbrMaterial->normal()->width(), pbrMaterial->normal()->height(), GL_RGBA, GL_UNSIGNED_BYTE, &pbrMaterial->normal()->data()[0], true);

	std::vector<byte> metalnessRoughnessAmbientOcclusionData;
	metalnessRoughnessAmbientOcclusionData.resize(pbrMaterial->albedo()->width()*pbrMaterial->albedo()->height()*4);

	const auto metalness = pbrMaterial->metalness();
	const auto roughness = pbrMaterial->roughness();
	const auto ambientOcclusion = pbrMaterial->ambientOcclusion();

	for (int j=0; j < metalnessRoughnessAmbientOcclusionData.size(); j+=4)
	{
		metalnessRoughnessAmbientOcclusionData[j] = (metalness ? (metalness->data()[j] + metalness->data()[j+1] + metalness->data()[j+2]) / 3 : 127);
		metalnessRoughnessAmbientOcclusionData[j+1] = (roughness ? (roughness->data()[j] + roughness->data()[j+1] + roughness->data()[j+2]) / 3 : 127);
		metalnessRoughnessAmbientOcclusionData[j+2] = (ambientOcclusion ? (ambientOcclusion->data()[j] + ambientOcclusion->data()[j+1] + ambientOcclusion->data()[j+2]) / 3 : 127);
		metalnessRoughnessAmbientOcclusionData[j+3] = 0;
	}

	material.metallicRoughnessAmbientOcclusion = Texture2d();
	material.metallicRoughnessAmbientOcclusion.generate(GL_RGBA, pbrMaterial->albedo()->width(), pbrMaterial->albedo()->height(), GL_RGBA, GL_UNSIGNED_BYTE, &metalnessRoughnessAmbientOcclusionData[0], true);

	return handle;
}

TerrainHandle OpenGlRenderer::createStaticTerrain(
	const IHeightMap* heightMap,
	const ISplatMap* splatMap,
	const IDisplacementMap* displacementMap
)
{
	auto handle = terrains_.create();
	auto& terrain = terrains_[handle];

	//terrain.vao = meshes_[meshHandle];
	//terrain.textureHandle = textureHandle;

	terrain.width = heightMap->image()->width();
	terrain.height = heightMap->image()->height();

	{
//		terrain.textureHandle = createTexture2d(heightMap->image());
		terrain.textureHandle = texture2ds_.create();
		auto& texture = texture2ds_[terrain.textureHandle];

		texture.generate(GL_RGBA,  heightMap->image()->width(),  heightMap->image()->height(), GL_RGBA, GL_UNSIGNED_BYTE, &heightMap->image()->data()[0], true);
	}

	//terrain.terrainMapTextureHandle = createTexture2d(*splatMap->terrainMap());
	terrain.terrainMapTextureHandle = texture2ds_.create();
	auto& texture = texture2ds_[terrain.terrainMapTextureHandle];

	texture.generate(GL_RGBA8UI, splatMap->terrainMap()->width(), splatMap->terrainMap()->height(), GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, &splatMap->terrainMap()->data()[0], true);
	texture.bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	terrain.splatMapTexture2dArrays[0] = Texture2dArray();
	terrain.splatMapTexture2dArrays[0].generate(GL_RGBA, splatMap->materialMap()[0]->albedo()->width(), splatMap->materialMap()[0]->albedo()->height(), 256, GL_RGBA, GL_UNSIGNED_BYTE);
	terrain.splatMapTexture2dArrays[0].bind();
	for (int i=0; i < splatMap->materialMap().size(); ++i)
	{
		terrain.splatMapTexture2dArrays[0].texSubImage3D(splatMap->materialMap()[i]->albedo()->width(), splatMap->materialMap()[i]->albedo()->height(), i, GL_RGBA, GL_UNSIGNED_BYTE, &splatMap->materialMap()[i]->albedo()->data()[0]);
	}
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	terrain.splatMapTexture2dArrays[1] = Texture2dArray();
	terrain.splatMapTexture2dArrays[1].generate(GL_RGBA, splatMap->materialMap()[0]->normal()->width(), splatMap->materialMap()[0]->normal()->height(), 256, GL_RGBA, GL_UNSIGNED_BYTE);
	terrain.splatMapTexture2dArrays[1].bind();
	for (int i=0; i < splatMap->materialMap().size(); ++i)
	{
		terrain.splatMapTexture2dArrays[1].texSubImage3D(splatMap->materialMap()[i]->normal()->width(), splatMap->materialMap()[i]->normal()->height(), i, GL_RGBA, GL_UNSIGNED_BYTE, &splatMap->materialMap()[i]->normal()->data()[0]);
	}
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	const uint32 width = splatMap->materialMap()[0]->albedo()->width();
	const uint32 height = splatMap->materialMap()[0]->albedo()->height();

	std::vector<byte> metalnessRoughnessAmbientOcclusionData;
	metalnessRoughnessAmbientOcclusionData.resize(width*height*4);

	terrain.splatMapTexture2dArrays[2] = Texture2dArray();
	terrain.splatMapTexture2dArrays[2].generate(GL_RGBA, width, height, 256, GL_RGBA, GL_UNSIGNED_BYTE);
	terrain.splatMapTexture2dArrays[2].bind();
	for (int i=0; i < splatMap->materialMap().size(); ++i)
	{
		const auto metalness = splatMap->materialMap()[i]->metalness();
		const auto roughness = splatMap->materialMap()[i]->roughness();
		const auto ambientOcclusion = splatMap->materialMap()[i]->ambientOcclusion();

		for (int j=0; j < metalnessRoughnessAmbientOcclusionData.size(); j+=4)
		{
			metalnessRoughnessAmbientOcclusionData[j] = (metalness ? (metalness->data()[j] + metalness->data()[j+1] + metalness->data()[j+2]) / 3 : 127);
			metalnessRoughnessAmbientOcclusionData[j+1] = (roughness ? (roughness->data()[j] + roughness->data()[j+1] + roughness->data()[j+2]) / 3 : 127);
			metalnessRoughnessAmbientOcclusionData[j+2] = (ambientOcclusion ? (ambientOcclusion->data()[j] + ambientOcclusion->data()[j+1] + ambientOcclusion->data()[j+2]) / 3 : 127);
			metalnessRoughnessAmbientOcclusionData[j+3] = 0;
		}
		terrain.splatMapTexture2dArrays[2].texSubImage3D(width, height, i, GL_RGBA, GL_UNSIGNED_BYTE, &metalnessRoughnessAmbientOcclusionData[0]);
	}
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	//terrain.splatMapTextureHandles[0] = createTexture2d(*splatMap->materialMap()[0]->albedo());
	//terrain.splatMapTextureHandles[1] = createTexture2d(*splatMap->materialMap()[1]->albedo());
	//terrain.splatMapTextureHandles[2] = createTexture2d(*splatMap->materialMap()[2]->albedo());

	std::vector<glm::vec3> vertices;
	std::vector<uint32> indices;
	std::tie(vertices, indices) = detail::generateGrid(terrain.width - 1, terrain.height - 1);

	auto meshHandle = createStaticMesh(vertices, indices, {}, {}, {});
	terrain.vao = meshes_[meshHandle];

	return handle;
}
void OpenGlRenderer::destroy(const TerrainHandle& terrainHandle)
{
	// terrains_.destroy(terrainHandle);
}

SkyboxHandle OpenGlRenderer::createStaticSkybox(const IImage& back, const IImage& down, const IImage& front, const IImage& left, const IImage& right, const IImage& up)
{
	auto handle = skyboxes_.create();
	auto& skybox = skyboxes_[handle];

	skybox.width = back.width();
	skybox.height = back.height();

	skybox.textureCubeMap = TextureCubeMap();

	skybox.textureCubeMap.generate(GL_RGBA, skybox.width, skybox.height, GL_RGBA, GL_UNSIGNED_BYTE, &back.data()[0], &down.data()[0], &front.data()[0], &left.data()[0], &right.data()[0], &up.data()[0]);
	skybox.textureCubeMap.bind();

	std::vector<glm::vec3> vertices;
	std::vector<uint32> indices;
	std::tie(vertices, indices) = detail::generateCube();

	auto meshHandle = createStaticMesh(vertices, indices, {}, {}, {});
	skybox.vao = meshes_[meshHandle];

	return handle;
}

void OpenGlRenderer::destroy(const SkyboxHandle& skyboxHandle)
{
	// const auto& skybox = skyboxes_[skyboxHandle];
	//
	// destroy(skybox.meshHandle);
	//
	// skyboxes_.destroy(skyboxHandle);
}

std::string OpenGlRenderer::loadShaderContents(const std::string& filename) const
{
	LOG_DEBUG(logger_, (std::string("Loading shader: ") + filename))

	if (!fileSystem_->exists(filename)) throw std::runtime_error("Shader with filename '" + filename + "' does not exist.");

	auto file = fileSystem_->open(filename, fs::FileFlags::READ | fs::FileFlags::BINARY);
	return file->readAll();
}

VertexShaderHandle OpenGlRenderer::createVertexShader(const std::string& data)
{
	LOG_DEBUG(logger_, "Creating vertex shader from data: " + data);
	return vertexShaders_.create( VertexShader(data) );
}

FragmentShaderHandle OpenGlRenderer::createFragmentShader(const std::string& data)
{
	LOG_DEBUG(logger_, "Creating fragment shader from data: " + data);
	return fragmentShaders_.create( FragmentShader(data) );
}

TessellationControlShaderHandle OpenGlRenderer::createTessellationControlShader(const std::string& data)
{
	LOG_DEBUG(logger_, "Creating tessellation control shader from data: " + data);
	return tessellationControlShaders_.create( TessellationControlShader(data) );
}

TessellationEvaluationShaderHandle OpenGlRenderer::createTessellationEvaluationShader(const std::string& data)
{
	LOG_DEBUG(logger_, "Creating tessellation evaluation shader from data: " + data);
	return tessellationEvaluationShaders_.create( TessellationEvaluationShader(data) );
}

bool OpenGlRenderer::valid(const VertexShaderHandle& shaderHandle) const
{
	return vertexShaders_.valid(shaderHandle);
}

bool OpenGlRenderer::valid(const FragmentShaderHandle& shaderHandle) const
{
	return fragmentShaders_.valid(shaderHandle);
}

bool OpenGlRenderer::valid(const TessellationControlShaderHandle& shaderHandle) const
{
	return tessellationControlShaders_.valid(shaderHandle);
}

bool OpenGlRenderer::valid(const TessellationEvaluationShaderHandle& shaderHandle) const
{
	return tessellationEvaluationShaders_.valid(shaderHandle);
}

void OpenGlRenderer::destroyShader(const VertexShaderHandle& shaderHandle)
{
	if (!vertexShaders_.valid(shaderHandle))
	{
		throw std::runtime_error("Invalid shader handle");
	}

	vertexShaders_.destroy(shaderHandle);
}

void OpenGlRenderer::destroyShader(const FragmentShaderHandle& shaderHandle)
{
	if (!fragmentShaders_.valid(shaderHandle))
	{
		throw std::runtime_error("Invalid shader handle");
	}

	fragmentShaders_.destroy(shaderHandle);
}

void OpenGlRenderer::destroyShader(const TessellationControlShaderHandle& shaderHandle)
{
	if (!tessellationControlShaders_.valid(shaderHandle))
	{
		throw std::runtime_error("Invalid shader handle");
	}

	tessellationControlShaders_.destroy(shaderHandle);
}

void OpenGlRenderer::destroyShader(const TessellationEvaluationShaderHandle& shaderHandle)
{
	if (!tessellationEvaluationShaders_.valid(shaderHandle))
	{
		throw std::runtime_error("Invalid shader handle");
	}

	tessellationEvaluationShaders_.destroy(shaderHandle);
}

ShaderProgramHandle OpenGlRenderer::createShaderProgram(const VertexShaderHandle& vertexShaderHandle, const FragmentShaderHandle& fragmentShaderHandle)
{
	const auto& vertexShader = vertexShaders_[vertexShaderHandle];
	const auto& fragmentShader = fragmentShaders_[fragmentShaderHandle];

	return shaderPrograms_.create( ShaderProgram(vertexShader, fragmentShader) );
}

ShaderProgramHandle OpenGlRenderer::createShaderProgram(
	const VertexShaderHandle& vertexShaderHandle,
	const TessellationControlShaderHandle& tessellationControlShaderHandle,
	const TessellationEvaluationShaderHandle& tessellationEvaluationShaderHandle,
	const FragmentShaderHandle& fragmentShaderHandle
)
{
	const auto& vertexShader = vertexShaders_[vertexShaderHandle];
	const auto& tessellationControlShader = tessellationControlShaders_[tessellationControlShaderHandle];
	const auto& tessellationEvaluationShader = tessellationEvaluationShaders_[tessellationEvaluationShaderHandle];
	const auto& fragmentShader = fragmentShaders_[fragmentShaderHandle];

	return shaderPrograms_.create( ShaderProgram(vertexShader, tessellationControlShader, tessellationEvaluationShader, fragmentShader) );
}

bool OpenGlRenderer::valid(const ShaderProgramHandle& shaderProgramHandle) const
{
	return shaderPrograms_.valid(shaderProgramHandle);
}

void OpenGlRenderer::destroyShaderProgram(const ShaderProgramHandle& shaderProgramHandle)
{
	if (!shaderPrograms_.valid(shaderProgramHandle))
	{
		throw std::runtime_error("Invalid shader program handle");
	}

	shaderPrograms_.destroy(shaderProgramHandle);
}


RenderableHandle OpenGlRenderer::createRenderable(
	const RenderSceneHandle& renderSceneHandle,
	const MeshHandle& meshHandle,
	const TextureHandle& textureHandle,
	const glm::vec3& position,
	const glm::quat& orientation,
	const glm::vec3& scale,
	const ShaderProgramHandle& shaderProgramHandle
)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto handle = renderScene.renderables.create();
	auto& renderable = renderScene.renderables[handle];

	renderScene.shaderProgramHandle = shaderProgramHandle;

	renderable.vao = meshes_[meshHandle];
	renderable.textureHandle = textureHandle;

	renderable.graphicsData.position = position;
	renderable.graphicsData.scale = scale;
	renderable.graphicsData.orientation = orientation;

	return handle;
}

RenderableHandle OpenGlRenderer::createRenderable(
	const RenderSceneHandle& renderSceneHandle,
	const MeshHandle& meshHandle,
	const MaterialHandle& materialHandle,
	const glm::vec3& position,
	const glm::quat& orientation,
	const glm::vec3& scale
)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto handle = renderScene.renderables.create();
	auto& renderable = renderScene.renderables[handle];

	//renderScene.shaderProgramHandle = shaderProgramHandle;

	renderable.vao = meshes_[meshHandle];
	//renderable.textureHandle = textureHandle;
	renderable.materialHandle = materialHandle;

	renderable.graphicsData.position = position;
	renderable.graphicsData.scale = scale;
	renderable.graphicsData.orientation = orientation;

	return handle;
}

void OpenGlRenderer::destroy(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	renderScene.renderables.destroy(renderableHandle);
}

TerrainRenderableHandle OpenGlRenderer::createTerrainRenderable(
	const RenderSceneHandle& renderSceneHandle,
	const TerrainHandle& terrainHandle
)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto handle = renderScene.terrain.create();
	auto& terrainRenderable = renderScene.terrain[handle];

	const auto& terrain = terrains_[terrainHandle];

	terrainRenderable.vao = terrain.vao;
	terrainRenderable.terrainHandle = terrainHandle;

	terrainRenderable.graphicsData.position = glm::vec3(0.0f, 0.0f, 0.0f) + glm::vec3(-(float32)terrain.width/2.0f, 0.0f, -(float32)terrain.height/2.0f);
	terrainRenderable.graphicsData.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	terrainRenderable.graphicsData.orientation = glm::quat();

	return handle;
}

void OpenGlRenderer::destroy(const RenderSceneHandle& renderSceneHandle, const TerrainRenderableHandle& terrainRenderableHandle)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	renderScene.terrain.destroy(terrainRenderableHandle);
}

SkyboxRenderableHandle OpenGlRenderer::createSkyboxRenderable(const RenderSceneHandle& renderSceneHandle, const SkyboxHandle& skyboxHandle)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto handle = renderScene.skyboxes.create();
	auto& skyboxRenderable = renderScene.skyboxes[handle];

	const auto& skybox = skyboxes_[skyboxHandle];

	skyboxRenderable.vao = skybox.vao;
	skyboxRenderable.skyboxHandle = skyboxHandle;

	// skyboxRenderable.graphicsData.position = glm::vec3(0.0f, 0.0f, 0.0f) + glm::vec3(-(float32)skybox.width/2.0f, 0.0f, -(float32)skybox.height/2.0f);
	skyboxRenderable.graphicsData.position = glm::vec3();
	skyboxRenderable.graphicsData.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	skyboxRenderable.graphicsData.orientation = glm::quat();

	return handle;
}

void OpenGlRenderer::destroy(const RenderSceneHandle& renderSceneHandle, const SkyboxRenderableHandle& skyboxRenderableHandle)
{
	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	renderScene.skyboxes.destroy(skyboxRenderableHandle);
}

void OpenGlRenderer::rotate(const CameraHandle& cameraHandle, const glm::quat& quaternion, const TransformSpace& relativeTo)
{
	switch( relativeTo )
	{
		case TransformSpace::TS_LOCAL:
			camera_.orientation = camera_.orientation * glm::normalize( quaternion );
			break;

		case TransformSpace::TS_WORLD:
			camera_.orientation =  glm::normalize( quaternion ) * camera_.orientation;
			break;

		default:
			std::string message = std::string("Invalid TransformSpace type.");
			throw std::runtime_error(message);
	}
}

void OpenGlRenderer::rotate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::quat& quaternion, const TransformSpace& relativeTo)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	switch( relativeTo )
	{
		case TransformSpace::TS_LOCAL:
			renderable.graphicsData.orientation = renderable.graphicsData.orientation * glm::normalize( quaternion );
			break;

		case TransformSpace::TS_WORLD:
			renderable.graphicsData.orientation =  glm::normalize( quaternion ) * renderable.graphicsData.orientation;
			break;

		default:
			std::string message = std::string("Invalid TransformSpace type.");
			throw std::runtime_error(message);
	}
}

void OpenGlRenderer::rotate(const CameraHandle& cameraHandle, const float32 degrees, const glm::vec3& axis, const TransformSpace& relativeTo)
{
	switch( relativeTo )
	{
		case TransformSpace::TS_LOCAL:
			camera_.orientation = glm::normalize( glm::angleAxis(glm::radians(degrees), axis) ) * camera_.orientation;
			break;

		case TransformSpace::TS_WORLD:
			camera_.orientation =  camera_.orientation * glm::normalize( glm::angleAxis(glm::radians(degrees), axis) );
			break;

		default:
			std::string message = std::string("Invalid TransformSpace type.");
			throw std::runtime_error(message);
	}
}

void OpenGlRenderer::rotate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 degrees, const glm::vec3& axis, const TransformSpace& relativeTo)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	switch( relativeTo )
	{
		case TransformSpace::TS_LOCAL:
			renderable.graphicsData.orientation = glm::normalize( glm::angleAxis(glm::radians(degrees), axis) ) * renderable.graphicsData.orientation;
			break;

		case TransformSpace::TS_WORLD:
			renderable.graphicsData.orientation =  renderable.graphicsData.orientation * glm::normalize( glm::angleAxis(glm::radians(degrees), axis) );
			break;

		default:
			std::string message = std::string("Invalid TransformSpace type.");
			throw std::runtime_error(message);
	}
}

void OpenGlRenderer::rotation(const CameraHandle& cameraHandle, const glm::quat& quaternion)
{
	camera_.orientation = glm::normalize( quaternion );
}

void OpenGlRenderer::rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::quat& quaternion)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.orientation = glm::normalize( quaternion );
}

void OpenGlRenderer::rotation(const CameraHandle& cameraHandle, const float32 degrees, const glm::vec3& axis)
{
	camera_.orientation = glm::normalize( glm::angleAxis(glm::radians(degrees), axis) );
}

void OpenGlRenderer::rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 degrees, const glm::vec3& axis)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.orientation = glm::normalize( glm::angleAxis(glm::radians(degrees), axis) );
}

glm::quat OpenGlRenderer::rotation(const CameraHandle& cameraHandle) const
{
	return camera_.orientation;
}

glm::quat OpenGlRenderer::rotation(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const
{
	return renderSceneHandles_[renderSceneHandle].renderables[renderableHandle].graphicsData.orientation;
}

void OpenGlRenderer::translate(const CameraHandle& cameraHandle, const float32 x, const float32 y, const float32 z)
{
	camera_.position += glm::vec3(x, y, z);
}

void OpenGlRenderer::translate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.position += glm::vec3(x, y, z);
}

void OpenGlRenderer::translate(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const float32 x, const float32 y, const float32 z)
{
	auto& light = renderSceneHandles_[renderSceneHandle].pointLights[pointLightHandle];

	light.position += glm::vec3(x, y, z);
}

void OpenGlRenderer::translate(const CameraHandle& cameraHandle, const glm::vec3& trans)
{
	camera_.position += trans;
}

void OpenGlRenderer::translate(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& trans)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.position += trans;
}

void OpenGlRenderer::translate(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const glm::vec3& trans)
{
	auto& light = renderSceneHandles_[renderSceneHandle].pointLights[pointLightHandle];

	light.position += trans;
}

void OpenGlRenderer::scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.scale = glm::vec3(x, y, z);
}

void OpenGlRenderer::scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& scale)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.scale = scale;
}

void OpenGlRenderer::scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 scale)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.scale = glm::vec3(scale, scale, scale);
}

glm::vec3 OpenGlRenderer::scale(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const
{
	return renderSceneHandles_[renderSceneHandle].renderables[renderableHandle].graphicsData.scale;
}

void OpenGlRenderer::position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const float32 x, const float32 y, const float32 z)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];
	renderable.graphicsData.position = glm::vec3(x, y, z);
}

void OpenGlRenderer::position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const float32 x, const float32 y, const float32 z)
{
	auto& light = renderSceneHandles_[renderSceneHandle].pointLights[pointLightHandle];

	light.position += glm::vec3(x, y, z);
}

void OpenGlRenderer::position(const CameraHandle& cameraHandle, const float32 x, const float32 y, const float32 z)
{
	camera_.position = glm::vec3(x, y, z);
}

void OpenGlRenderer::position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& position)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.graphicsData.position = position;
}

void OpenGlRenderer::position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle, const glm::vec3& position)
{
	auto& light = renderSceneHandles_[renderSceneHandle].pointLights[pointLightHandle];

	light.position = position;
}

void OpenGlRenderer::position(const CameraHandle& cameraHandle, const glm::vec3& position)
{
	camera_.position = position;
}

glm::vec3 OpenGlRenderer::position(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle) const
{
	return renderSceneHandles_[renderSceneHandle].renderables[renderableHandle].graphicsData.position;
}

glm::vec3 OpenGlRenderer::position(const RenderSceneHandle& renderSceneHandle, const PointLightHandle& pointLightHandle) const
{
	return renderSceneHandles_[renderSceneHandle].pointLights[pointLightHandle].position;
}

glm::vec3 OpenGlRenderer::position(const CameraHandle& cameraHandle) const
{
	return camera_.position;
}

void OpenGlRenderer::lookAt(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const glm::vec3& lookAt)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	assert(lookAt != renderable.graphicsData.position);

	const glm::mat4 lookAtMatrix = glm::lookAt(renderable.graphicsData.position, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
	renderable.graphicsData.orientation =  glm::normalize( renderable.graphicsData.orientation * glm::quat_cast( lookAtMatrix ) );
}

void OpenGlRenderer::lookAt(const CameraHandle& cameraHandle, const glm::vec3& lookAt)
{
	assert(lookAt != camera_.position);

	const glm::mat4 lookAtMatrix = glm::lookAt(camera_.position, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
	camera_.orientation =  glm::normalize( camera_.orientation * glm::quat_cast( lookAtMatrix ) );

	// const glm::vec3 currentForward = camera_.orientation * glm::vec3(0.0f, 0.0f, 1.0f);
	// const glm::vec3 desiredForward = lookAt - camera_.position;
	// camera_.orientation = glm::normalize( glm::rotation(glm::normalize(currentForward), glm::normalize(desiredForward)) );
}

void OpenGlRenderer::assign(const RenderSceneHandle& renderSceneHandle, const RenderableHandle& renderableHandle, const SkeletonHandle& skeletonHandle)
{
	auto& renderable = renderSceneHandles_[renderSceneHandle].renderables[renderableHandle];

	renderable.ubo = skeletons_[skeletonHandle];
}

void OpenGlRenderer::update(
	const RenderSceneHandle& renderSceneHandle,
	const RenderableHandle& renderableHandle,
	const BonesHandle& bonesHandle,
	const std::vector<glm::mat4>& transformations
)
{
//	const auto& ubo = skeletons_[skeletonHandle];
//
//	glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
//	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);

	//void* d = glMapBufferRange(GL_UNIFORM_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
	//memcpy( d, data, size );
	//glUnmapBuffer(GL_UNIFORM_BUFFER);

	auto& renderScene = renderSceneHandles_[renderSceneHandle];
	auto& renderable = renderScene.renderables[renderableHandle];

	auto size = transformations.size() * sizeof(glm::mat4);

	glBindBuffer(GL_UNIFORM_BUFFER, renderable.ubo.id);

	void* d = glMapBufferRange(GL_UNIFORM_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
	memcpy( d, &transformations[0], size );
	glUnmapBuffer(GL_UNIFORM_BUFFER);

//	glBufferData(GL_UNIFORM_BUFFER, size, &transformations[0], GL_STREAM_DRAW);
}

void OpenGlRenderer::setMouseRelativeMode(const bool enabled)
{
	auto result = SDL_SetRelativeMouseMode(static_cast<SDL_bool>(enabled));

	if (result != 0)
	{
		auto message = std::string("Unable to set relative mouse mode: ") + SDL_GetError();
		throw std::runtime_error(message);
	}
}

void OpenGlRenderer::setWindowGrab(const bool enabled)
{
	SDL_SetWindowGrab(sdlWindow_, static_cast<SDL_bool>(enabled));
}

bool OpenGlRenderer::cursorVisible() const
{
    return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE;
}

void OpenGlRenderer::setCursorVisible(const bool visible)
{
	auto toggle = (visible ? SDL_ENABLE : SDL_DISABLE);
	auto result = SDL_ShowCursor(toggle);

	if (result < 0)
	{
		auto message = std::string("Unable to set mouse cursor visible\\invisible: ") + SDL_GetError();
		throw std::runtime_error(message);
	}
}

void OpenGlRenderer::processEvents()
{
	SDL_Event evt;
	while( SDL_PollEvent( &evt ) )
	{
		Event event = convertSdlEvent(evt);
		this->handleEvent( event );
		/*
		switch( evt.type )
		{
			case SDL_WINDOWEVENT:
				//handleWindowEvent( evt );
				break;
			case SDL_QUIT:

				break;
			default:
				break;
		}
		*/
	}
}

void OpenGlRenderer::handleEvent(const Event& event)
{
	for ( auto it : eventListeners_ )
	{
		it->processEvent(event);
	}
}

Event OpenGlRenderer::convertSdlEvent(const SDL_Event& event)
{
	Event e;

	switch(event.type)
	{
		case SDL_QUIT:
			e.type = QUIT;
			break;

		case SDL_WINDOWEVENT:
			e.type = WINDOWEVENT;
			e.window.eventType = convertSdlWindowEventId(event.window.event);
			e.window.timestamp = event.window.timestamp;
			e.window.windowId = event.window.windowID;
			e.window.data1 = event.window.data1;
			e.window.data2 = event.window.data2;
			break;

		case SDL_TEXTINPUT:
			e.type = TEXTINPUT;
            e.window.timestamp = event.window.timestamp;
            strncpy(&e.text.text[0], &event.text.text[0], TEXTINPUTEVENT_TEXT_SIZE);
			break;

		case SDL_KEYDOWN:
			e.type = KEYDOWN;
			e.key.keySym = convertSdlKeySym(event.key.keysym);
			e.key.state = (event.key.state == SDL_RELEASED ? KEY_RELEASED : KEY_PRESSED);
			e.key.repeat = event.key.repeat;
			break;

		case SDL_KEYUP:
			e.type = KEYUP;
			e.key.keySym = convertSdlKeySym(event.key.keysym);
			e.key.state = (event.key.state == SDL_RELEASED ? KEY_RELEASED : KEY_PRESSED);
			e.key.repeat = event.key.repeat;
			break;

		case SDL_MOUSEMOTION:
			e.type = MOUSEMOTION;
			e.motion.x = event.motion.x;
			e.motion.y = event.motion.y;
			e.motion.xrel = event.motion.xrel;
			e.motion.yrel = event.motion.yrel;
			//e.state = (event.key.state == SDL_RELEASED ? KEY_RELEASED : KEY_PRESSED);

			break;

		case SDL_MOUSEBUTTONDOWN:
			e.type = MOUSEBUTTONDOWN;
			e.button.button = event.button.button;
			e.button.state = event.button.state;
			e.button.clicks = event.button.clicks;
			e.button.x = event.button.x;
			e.button.y = event.button.y;

			break;

		case SDL_MOUSEBUTTONUP:
			e.type = MOUSEBUTTONUP;
			e.button.button = event.button.button;
			e.button.state = event.button.state;
			e.button.clicks = event.button.clicks;
			e.button.x = event.button.x;
			e.button.y = event.button.y;

			break;

		case SDL_MOUSEWHEEL:
			e.type = MOUSEWHEEL;
			e.wheel.x = event.wheel.x;
			e.wheel.y = event.wheel.y;
			e.wheel.direction = event.wheel.direction;

			break;

		default:
			e.type = UNKNOWN;
			break;
	}

	return e;
}

WindowEventType OpenGlRenderer::convertSdlWindowEventId(const uint8 windowEventId)
{
	switch(windowEventId)
	{
		case SDL_WINDOWEVENT_NONE:
			return WINDOWEVENT_NONE;
	    case SDL_WINDOWEVENT_SHOWN:
			return WINDOWEVENT_SHOWN;
	    case SDL_WINDOWEVENT_HIDDEN:
			return WINDOWEVENT_HIDDEN;
	    case SDL_WINDOWEVENT_EXPOSED:
			return WINDOWEVENT_EXPOSED;
	    case SDL_WINDOWEVENT_MOVED:
			return WINDOWEVENT_MOVED;
	    case SDL_WINDOWEVENT_RESIZED:
			return WINDOWEVENT_RESIZED;
	    case SDL_WINDOWEVENT_SIZE_CHANGED:
			return WINDOWEVENT_SIZE_CHANGED;
	    case SDL_WINDOWEVENT_MINIMIZED:
			return WINDOWEVENT_MINIMIZED;
	    case SDL_WINDOWEVENT_MAXIMIZED:
			return WINDOWEVENT_MAXIMIZED;
	    case SDL_WINDOWEVENT_RESTORED:
			return WINDOWEVENT_RESTORED;
	    case SDL_WINDOWEVENT_ENTER:
			return WINDOWEVENT_ENTER;
	    case SDL_WINDOWEVENT_LEAVE:
			return WINDOWEVENT_LEAVE;
	    case SDL_WINDOWEVENT_FOCUS_GAINED:
			return WINDOWEVENT_FOCUS_GAINED;
	    case SDL_WINDOWEVENT_FOCUS_LOST:
			return WINDOWEVENT_FOCUS_LOST;
	    case SDL_WINDOWEVENT_CLOSE:
			return WINDOWEVENT_CLOSE;
	    case SDL_WINDOWEVENT_TAKE_FOCUS:
			return WINDOWEVENT_TAKE_FOCUS;
	    case SDL_WINDOWEVENT_HIT_TEST:
			return WINDOWEVENT_HIT_TEST;

		default:
			return WINDOWEVENT_UNKNOWN;
	}
}

KeySym OpenGlRenderer::convertSdlKeySym(const SDL_Keysym& keySym)
{
	KeySym key;

	key.sym = convertSdlKeycode(keySym.sym);
	key.scancode = convertSdlScancode(keySym.scancode);
	key.mod = convertSdlKeymod(keySym.mod);

	return key;
}

KeyCode OpenGlRenderer::convertSdlKeycode(const SDL_Keycode& sdlKeycode)
{
	switch(sdlKeycode)
	{
		case SDLK_0:
			return KEY_0;
		case SDLK_1:
			return KEY_1;
		case SDLK_2:
			return KEY_2;
		case SDLK_3:
			return KEY_3;
		case SDLK_4:
			return KEY_4;
		case SDLK_5:
			return KEY_5;
		case SDLK_6:
			return KEY_6;
		case SDLK_7:
			return KEY_7;
		case SDLK_8:
			return KEY_8;
		case SDLK_9:
			return KEY_9;
		case SDLK_a:
			return KEY_a;
		case SDLK_AC_BACK:
			return KEY_AC_BACK;
		case SDLK_AC_BOOKMARKS:
			return KEY_AC_BOOKMARKS;
		case SDLK_AC_FORWARD:
			return KEY_AC_FORWARD;
		case SDLK_AC_HOME:
			return KEY_AC_HOME;
		case SDLK_AC_REFRESH:
			return KEY_AC_REFRESH;
		case SDLK_AC_SEARCH:
			return KEY_AC_SEARCH;
		case SDLK_AC_STOP:
			return KEY_AC_STOP;
		case SDLK_AGAIN:
			return KEY_AGAIN;
		case SDLK_ALTERASE:
			return KEY_ALTERASE;
		case SDLK_QUOTE:
			return KEY_QUOTE;
		case SDLK_APPLICATION:
			return KEY_APPLICATION;
		case SDLK_AUDIOMUTE:
			return KEY_AUDIOMUTE;
		case SDLK_AUDIONEXT:
			return KEY_AUDIONEXT;
		case SDLK_AUDIOPLAY:
			return KEY_AUDIOPLAY;
		case SDLK_AUDIOPREV:
			return KEY_AUDIOPREV;
		case SDLK_AUDIOSTOP:
			return KEY_AUDIOSTOP;
		case SDLK_b:
			return KEY_b;
		case SDLK_BACKSLASH:
			return KEY_BACKSLASH;
		case SDLK_BACKSPACE:
			return KEY_BACKSPACE;
		case SDLK_BRIGHTNESSDOWN:
			return KEY_BRIGHTNESSDOWN;
		case SDLK_BRIGHTNESSUP:
			return KEY_BRIGHTNESSUP;
		case SDLK_c:
			return KEY_c;
		case SDLK_CALCULATOR:
			return KEY_CALCULATOR;
		case SDLK_CANCEL:
			return KEY_CANCEL;
		case SDLK_CAPSLOCK:
			return KEY_CAPSLOCK;
		case SDLK_CLEAR:
			return KEY_CLEAR;
		case SDLK_CLEARAGAIN:
			return KEY_CLEARAGAIN;
		case SDLK_COMMA:
			return KEY_COMMA;
		case SDLK_COMPUTER:
			return KEY_COMPUTER;
		case SDLK_COPY:
			return KEY_COPY;
		case SDLK_CRSEL:
			return KEY_CRSEL;
		case SDLK_CURRENCYSUBUNIT:
			return KEY_CURRENCYSUBUNIT;
		case SDLK_CURRENCYUNIT:
			return KEY_CURRENCYUNIT;
		case SDLK_CUT:
			return KEY_CUT;
		case SDLK_d:
			return KEY_d;
		case SDLK_DECIMALSEPARATOR:
			return KEY_DECIMALSEPARATOR;
		case SDLK_DELETE:
			return KEY_DELETE;
		case SDLK_DISPLAYSWITCH:
			return KEY_DISPLAYSWITCH;
		case SDLK_DOWN:
			return KEY_DOWN;
		case SDLK_e:
			return KEY_e;
		case SDLK_EJECT:
			return KEY_EJECT;
		case SDLK_END:
			return KEY_END;
		case SDLK_EQUALS:
			return KEY_EQUALS;
		case SDLK_ESCAPE:
			return KEY_ESCAPE;
		case SDLK_EXECUTE:
			return KEY_EXECUTE;
		case SDLK_EXSEL:
			return KEY_EXSEL;
		case SDLK_f:
			return KEY_f;
		case SDLK_F1:
			return KEY_F1;
		case SDLK_F10:
			return KEY_F10;
		case SDLK_F11:
			return KEY_F11;
		case SDLK_F12:
			return KEY_F12;
		case SDLK_F13:
			return KEY_F13;
		case SDLK_F14:
			return KEY_F14;
		case SDLK_F15:
			return KEY_F15;
		case SDLK_F16:
			return KEY_F16;
		case SDLK_F17:
			return KEY_F17;
		case SDLK_F18:
			return KEY_F18;
		case SDLK_F19:
			return KEY_F19;
		case SDLK_F2:
			return KEY_F2;
		case SDLK_F20:
			return KEY_F20;
		case SDLK_F21:
			return KEY_F21;
		case SDLK_F22:
			return KEY_F22;
		case SDLK_F23:
			return KEY_F23;
		case SDLK_F24:
			return KEY_F24;
		case SDLK_F3:
			return KEY_F3;
		case SDLK_F4:
			return KEY_F4;
		case SDLK_F5:
			return KEY_F5;
		case SDLK_F6:
			return KEY_F6;
		case SDLK_F7:
			return KEY_F7;
		case SDLK_F8:
			return KEY_F8;
		case SDLK_F9:
			return KEY_F9;
		case SDLK_FIND:
			return KEY_FIND;
		case SDLK_g:
			return KEY_g;
		case SDLK_BACKQUOTE:
			return KEY_BACKQUOTE;
		case SDLK_h:
			return KEY_h;
		case SDLK_HELP:
			return KEY_HELP;
		case SDLK_HOME:
			return KEY_HOME;
		case SDLK_i:
			return KEY_i;
		case SDLK_INSERT:
			return KEY_INSERT;
		case SDLK_j:
			return KEY_j;
		case SDLK_k:
			return KEY_k;
		case SDLK_KBDILLUMDOWN:
			return KEY_KBDILLUMDOWN;
		case SDLK_KBDILLUMTOGGLE:
			return KEY_KBDILLUMTOGGLE;
		case SDLK_KBDILLUMUP:
			return KEY_KBDILLUMUP;
		case SDLK_KP_0:
			return KEY_KP_0;
		case SDLK_KP_00:
			return KEY_KP_00;
		case SDLK_KP_000:
			return KEY_KP_000;
		case SDLK_KP_1:
			return KEY_KP_1;
		case SDLK_KP_2:
			return KEY_KP_2;
		case SDLK_KP_3:
			return KEY_KP_3;
		case SDLK_KP_4:
			return KEY_KP_4;
		case SDLK_KP_5:
			return KEY_KP_5;
		case SDLK_KP_6:
			return KEY_KP_6;
		case SDLK_KP_7:
			return KEY_KP_7;
		case SDLK_KP_8:
			return KEY_KP_8;
		case SDLK_KP_9:
			return KEY_KP_9;
		case SDLK_KP_A:
			return KEY_KP_A;
		case SDLK_KP_AMPERSAND:
			return KEY_KP_AMPERSAND;
		case SDLK_KP_AT:
			return KEY_KP_AT;
		case SDLK_KP_B:
			return KEY_KP_B;
		case SDLK_KP_BACKSPACE:
			return KEY_KP_BACKSPACE;
		case SDLK_KP_BINARY:
			return KEY_KP_BINARY;
		case SDLK_KP_C:
			return KEY_KP_C;
		case SDLK_KP_CLEAR:
			return KEY_KP_CLEAR;
		case SDLK_KP_CLEARENTRY:
			return KEY_KP_CLEARENTRY;
		case SDLK_KP_COLON:
			return KEY_KP_COLON;
		case SDLK_KP_COMMA:
			return KEY_KP_COMMA;
		case SDLK_KP_D:
			return KEY_KP_D;
		case SDLK_KP_DBLAMPERSAND:
			return KEY_KP_DBLAMPERSAND;
		case SDLK_KP_DBLVERTICALBAR:
			return KEY_KP_DBLVERTICALBAR;
		case SDLK_KP_DECIMAL:
			return KEY_KP_DECIMAL;
		case SDLK_KP_DIVIDE:
			return KEY_KP_DIVIDE;
		case SDLK_KP_E:
			return KEY_KP_E;
		case SDLK_KP_ENTER:
			return KEY_KP_ENTER;
		case SDLK_KP_EQUALS:
			return KEY_KP_EQUALS;
		case SDLK_KP_EQUALSAS400:
			return KEY_KP_EQUALSAS400;
		case SDLK_KP_EXCLAM:
			return KEY_KP_EXCLAM;
		case SDLK_KP_F:
			return KEY_KP_F;
		case SDLK_KP_GREATER:
			return KEY_KP_GREATER;
		case SDLK_KP_HASH:
			return KEY_KP_HASH;
		case SDLK_KP_HEXADECIMAL:
			return KEY_KP_HEXADECIMAL;
		case SDLK_KP_LEFTBRACE:
			return KEY_KP_LEFTBRACE;
		case SDLK_KP_LEFTPAREN:
			return KEY_KP_LEFTPAREN;
		case SDLK_KP_LESS:
			return KEY_KP_LESS;
		case SDLK_KP_MEMADD:
			return KEY_KP_MEMADD;
		case SDLK_KP_MEMCLEAR:
			return KEY_KP_MEMCLEAR;
		case SDLK_KP_MEMDIVIDE:
			return KEY_KP_MEMDIVIDE;
		case SDLK_KP_MEMMULTIPLY:
			return KEY_KP_MEMMULTIPLY;
		case SDLK_KP_MEMRECALL:
			return KEY_KP_MEMRECALL;
		case SDLK_KP_MEMSTORE:
			return KEY_KP_MEMSTORE;
		case SDLK_KP_MEMSUBTRACT:
			return KEY_KP_MEMSUBTRACT;
		case SDLK_KP_MINUS:
			return KEY_KP_MINUS;
		case SDLK_KP_MULTIPLY:
			return KEY_KP_MULTIPLY;
		case SDLK_KP_OCTAL:
			return KEY_KP_OCTAL;
		case SDLK_KP_PERCENT:
			return KEY_KP_PERCENT;
		case SDLK_KP_PERIOD:
			return KEY_KP_PERIOD;
		case SDLK_KP_PLUS:
			return KEY_KP_PLUS;
		case SDLK_KP_PLUSMINUS:
			return KEY_KP_PLUSMINUS;
		case SDLK_KP_POWER:
			return KEY_KP_POWER;
		case SDLK_KP_RIGHTBRACE:
			return KEY_KP_RIGHTBRACE;
		case SDLK_KP_RIGHTPAREN:
			return KEY_KP_RIGHTPAREN;
		case SDLK_KP_SPACE:
			return KEY_KP_SPACE;
		case SDLK_KP_TAB:
			return KEY_KP_TAB;
		case SDLK_KP_VERTICALBAR:
			return KEY_KP_VERTICALBAR;
		case SDLK_KP_XOR:
			return KEY_KP_XOR;
		case SDLK_l:
			return KEY_l;
		case SDLK_LALT:
			return KEY_LALT;
		case SDLK_LCTRL:
			return KEY_LCTRL;
		case SDLK_LEFT:
			return KEY_LEFT;
		case SDLK_LEFTBRACKET:
			return KEY_LEFTBRACKET;
		case SDLK_LGUI:
			return KEY_LGUI;
		case SDLK_LSHIFT:
			return KEY_LSHIFT;
		case SDLK_m:
			return KEY_m;
		case SDLK_MAIL:
			return KEY_MAIL;
		case SDLK_MEDIASELECT:
			return KEY_MEDIASELECT;
		case SDLK_MENU:
			return KEY_MENU;
		case SDLK_MINUS:
			return KEY_MINUS;
		case SDLK_MODE:
			return KEY_MODE;
		case SDLK_MUTE:
			return KEY_MUTE;
		case SDLK_n:
			return KEY_n;
		case SDLK_NUMLOCKCLEAR:
			return KEY_NUMLOCKCLEAR;
		case SDLK_o:
			return KEY_o;
		case SDLK_OPER:
			return KEY_OPER;
		case SDLK_OUT:
			return KEY_OUT;
		case SDLK_p:
			return KEY_p;
		case SDLK_PAGEDOWN:
			return KEY_PAGEDOWN;
		case SDLK_PAGEUP:
			return KEY_PAGEUP;
		case SDLK_PASTE:
			return KEY_PASTE;
		case SDLK_PAUSE:
			return KEY_PAUSE;
		case SDLK_PERIOD:
			return KEY_PERIOD;
		case SDLK_POWER:
			return KEY_POWER;
		case SDLK_PRINTSCREEN:
			return KEY_PRINTSCREEN;
		case SDLK_PRIOR:
			return KEY_PRIOR;
		case SDLK_q:
			return KEY_q;
		case SDLK_r:
			return KEY_r;
		case SDLK_RALT:
			return KEY_RALT;
		case SDLK_RCTRL:
			return KEY_RCTRL;
		case SDLK_RETURN:
			return KEY_RETURN;
		case SDLK_RETURN2:
			return KEY_RETURN2;
		case SDLK_RGUI:
			return KEY_RGUI;
		case SDLK_RIGHT:
			return KEY_RIGHT;
		case SDLK_RIGHTBRACKET:
			return KEY_RIGHTBRACKET;
		case SDLK_RSHIFT:
			return KEY_RSHIFT;
		case SDLK_s:
			return KEY_s;
		case SDLK_SCROLLLOCK:
			return KEY_SCROLLLOCK;
		case SDLK_SELECT:
			return KEY_SELECT;
		case SDLK_SEMICOLON:
			return KEY_SEMICOLON;
		case SDLK_SEPARATOR:
			return KEY_SEPARATOR;
		case SDLK_SLASH:
			return KEY_SLASH;
		case SDLK_SLEEP:
			return KEY_SLEEP;
		case SDLK_SPACE:
			return KEY_SPACE;
		case SDLK_STOP:
			return KEY_STOP;
		case SDLK_SYSREQ:
			return KEY_SYSREQ;
		case SDLK_t:
			return KEY_t;
		case SDLK_TAB:
			return KEY_TAB;
		case SDLK_THOUSANDSSEPARATOR:
			return KEY_THOUSANDSSEPARATOR;
		case SDLK_u:
			return KEY_u;
		case SDLK_UNDO:
			return KEY_UNDO;
		case SDLK_UNKNOWN:
			return KEY_UNKNOWN;
		case SDLK_UP:
			return KEY_UP;
		case SDLK_v:
			return KEY_v;
		case SDLK_VOLUMEDOWN:
			return KEY_VOLUMEDOWN;
		case SDLK_VOLUMEUP:
			return KEY_VOLUMEUP;
		case SDLK_w:
			return KEY_w;
		case SDLK_WWW:
			return KEY_WWW;
		case SDLK_x:
			return KEY_x;
		case SDLK_y:
			return KEY_y;
		case SDLK_z:
			return KEY_z;
		case SDLK_AMPERSAND:
			return KEY_AMPERSAND;
		case SDLK_ASTERISK:
			return KEY_ASTERISK;
		case SDLK_AT:
			return KEY_AT;
		case SDLK_CARET:
			return KEY_CARET;
		case SDLK_COLON:
			return KEY_COLON;
		case SDLK_DOLLAR:
			return KEY_DOLLAR;
		case SDLK_EXCLAIM:
			return KEY_EXCLAIM;
		case SDLK_GREATER:
			return KEY_GREATER;
		case SDLK_HASH:
			return KEY_HASH;
		case SDLK_LEFTPAREN:
			return KEY_LEFTPAREN;
		case SDLK_LESS:
			return KEY_LESS;
		case SDLK_PERCENT:
			return KEY_PERCENT;
		case SDLK_PLUS:
			return KEY_PLUS;
		case SDLK_QUESTION:
			return KEY_QUESTION;
		case SDLK_QUOTEDBL:
			return KEY_QUOTEDBL;
		case SDLK_RIGHTPAREN:
			return KEY_RIGHTPAREN;
		case SDLK_UNDERSCORE:
			return KEY_UNDERSCORE;

		default:
			return KEY_UNKNOWN;
	}
}

ScanCode OpenGlRenderer::convertSdlScancode(const SDL_Scancode& sdlScancode)
{
	switch(sdlScancode)
	{
		case SDL_SCANCODE_0:
			return SCANCODE_0;
		case SDL_SCANCODE_1:
			return SCANCODE_1;
		case SDL_SCANCODE_2:
			return SCANCODE_2;
		case SDL_SCANCODE_3:
			return SCANCODE_3;
		case SDL_SCANCODE_4:
			return SCANCODE_4;
		case SDL_SCANCODE_5:
			return SCANCODE_5;
		case SDL_SCANCODE_6:
			return SCANCODE_6;
		case SDL_SCANCODE_7:
			return SCANCODE_7;
		case SDL_SCANCODE_8:
			return SCANCODE_8;
		case SDL_SCANCODE_9:
			return SCANCODE_9;
		case SDL_SCANCODE_A:
			return SCANCODE_A;
		case SDL_SCANCODE_AC_BACK:
			return SCANCODE_AC_BACK;
		case SDL_SCANCODE_AC_BOOKMARKS:
			return SCANCODE_AC_BOOKMARKS;
		case SDL_SCANCODE_AC_FORWARD:
			return SCANCODE_AC_FORWARD;
		case SDL_SCANCODE_AC_HOME:
			return SCANCODE_AC_HOME;
		case SDL_SCANCODE_AC_REFRESH:
			return SCANCODE_AC_REFRESH;
		case SDL_SCANCODE_AC_SEARCH:
			return SCANCODE_AC_SEARCH;
		case SDL_SCANCODE_AC_STOP:
			return SCANCODE_AC_STOP;
		case SDL_SCANCODE_AGAIN:
			return SCANCODE_AGAIN;
		case SDL_SCANCODE_ALTERASE:
			return SCANCODE_ALTERASE;
		case SDL_SCANCODE_APOSTROPHE:
			return SCANCODE_APOSTROPHE;
		case SDL_SCANCODE_APPLICATION:
			return SCANCODE_APPLICATION;
		case SDL_SCANCODE_AUDIOMUTE:
			return SCANCODE_AUDIOMUTE;
		case SDL_SCANCODE_AUDIONEXT:
			return SCANCODE_AUDIONEXT;
		case SDL_SCANCODE_AUDIOPLAY:
			return SCANCODE_AUDIOPLAY;
		case SDL_SCANCODE_AUDIOPREV:
			return SCANCODE_AUDIOPREV;
		case SDL_SCANCODE_AUDIOSTOP:
			return SCANCODE_AUDIOSTOP;
		case SDL_SCANCODE_B:
			return SCANCODE_B;
		case SDL_SCANCODE_BACKSLASH:
			return SCANCODE_BACKSLASH;
		case SDL_SCANCODE_BACKSPACE:
			return SCANCODE_BACKSPACE;
		case SDL_SCANCODE_BRIGHTNESSDOWN:
			return SCANCODE_BRIGHTNESSDOWN;
		case SDL_SCANCODE_BRIGHTNESSUP:
			return SCANCODE_BRIGHTNESSUP;
		case SDL_SCANCODE_C:
			return SCANCODE_C;
		case SDL_SCANCODE_CALCULATOR:
			return SCANCODE_CALCULATOR;
		case SDL_SCANCODE_CANCEL:
			return SCANCODE_CANCEL;
		case SDL_SCANCODE_CAPSLOCK:
			return SCANCODE_CAPSLOCK;
		case SDL_SCANCODE_CLEAR:
			return SCANCODE_CLEAR;
		case SDL_SCANCODE_CLEARAGAIN:
			return SCANCODE_CLEARAGAIN;
		case SDL_SCANCODE_COMMA:
			return SCANCODE_COMMA;
		case SDL_SCANCODE_COMPUTER:
			return SCANCODE_COMPUTER;
		case SDL_SCANCODE_COPY:
			return SCANCODE_COPY;
		case SDL_SCANCODE_CRSEL:
			return SCANCODE_CRSEL;
		case SDL_SCANCODE_CURRENCYSUBUNIT:
			return SCANCODE_CURRENCYSUBUNIT;
		case SDL_SCANCODE_CURRENCYUNIT:
			return SCANCODE_CURRENCYUNIT;
		case SDL_SCANCODE_CUT:
			return SCANCODE_CUT;
		case SDL_SCANCODE_D:
			return SCANCODE_D;
		case SDL_SCANCODE_DECIMALSEPARATOR:
			return SCANCODE_DECIMALSEPARATOR;
		case SDL_SCANCODE_DELETE:
			return SCANCODE_DELETE;
		case SDL_SCANCODE_DISPLAYSWITCH:
			return SCANCODE_DISPLAYSWITCH;
		case SDL_SCANCODE_DOWN:
			return SCANCODE_DOWN;
		case SDL_SCANCODE_E:
			return SCANCODE_E;
		case SDL_SCANCODE_EJECT:
			return SCANCODE_EJECT;
		case SDL_SCANCODE_END:
			return SCANCODE_END;
		case SDL_SCANCODE_EQUALS:
			return SCANCODE_EQUALS;
		case SDL_SCANCODE_ESCAPE:
			return SCANCODE_ESCAPE;
		case SDL_SCANCODE_EXECUTE:
			return SCANCODE_EXECUTE;
		case SDL_SCANCODE_EXSEL:
			return SCANCODE_EXSEL;
		case SDL_SCANCODE_F:
			return SCANCODE_F;
		case SDL_SCANCODE_F1:
			return SCANCODE_F1;
		case SDL_SCANCODE_F10:
			return SCANCODE_F10;
		case SDL_SCANCODE_F11:
			return SCANCODE_F11;
		case SDL_SCANCODE_F12:
			return SCANCODE_F12;
		case SDL_SCANCODE_F13:
			return SCANCODE_F13;
		case SDL_SCANCODE_F14:
			return SCANCODE_F14;
		case SDL_SCANCODE_F15:
			return SCANCODE_F15;
		case SDL_SCANCODE_F16:
			return SCANCODE_F16;
		case SDL_SCANCODE_F17:
			return SCANCODE_F17;
		case SDL_SCANCODE_F18:
			return SCANCODE_F18;
		case SDL_SCANCODE_F19:
			return SCANCODE_F19;
		case SDL_SCANCODE_F2:
			return SCANCODE_F2;
		case SDL_SCANCODE_F20:
			return SCANCODE_F20;
		case SDL_SCANCODE_F21:
			return SCANCODE_F21;
		case SDL_SCANCODE_F22:
			return SCANCODE_F22;
		case SDL_SCANCODE_F23:
			return SCANCODE_F23;
		case SDL_SCANCODE_F24:
			return SCANCODE_F24;
		case SDL_SCANCODE_F3:
			return SCANCODE_F3;
		case SDL_SCANCODE_F4:
			return SCANCODE_F4;
		case SDL_SCANCODE_F5:
			return SCANCODE_F5;
		case SDL_SCANCODE_F6:
			return SCANCODE_F6;
		case SDL_SCANCODE_F7:
			return SCANCODE_F7;
		case SDL_SCANCODE_F8:
			return SCANCODE_F8;
		case SDL_SCANCODE_F9:
			return SCANCODE_F9;
		case SDL_SCANCODE_FIND:
			return SCANCODE_FIND;
		case SDL_SCANCODE_G:
			return SCANCODE_G;
		case SDL_SCANCODE_GRAVE:
			return SCANCODE_GRAVE;
		case SDL_SCANCODE_H:
			return SCANCODE_H;
		case SDL_SCANCODE_HELP:
			return SCANCODE_HELP;
		case SDL_SCANCODE_HOME:
			return SCANCODE_HOME;
		case SDL_SCANCODE_I:
			return SCANCODE_I;
		case SDL_SCANCODE_INSERT:
			return SCANCODE_INSERT;
		case SDL_SCANCODE_J:
			return SCANCODE_J;
		case SDL_SCANCODE_K:
			return SCANCODE_K;
		case SDL_SCANCODE_KBDILLUMDOWN:
			return SCANCODE_KBDILLUMDOWN;
		case SDL_SCANCODE_KBDILLUMTOGGLE:
			return SCANCODE_KBDILLUMTOGGLE;
		case SDL_SCANCODE_KBDILLUMUP:
			return SCANCODE_KBDILLUMUP;
		case SDL_SCANCODE_KP_0:
			return SCANCODE_KP_0;
		case SDL_SCANCODE_KP_00:
			return SCANCODE_KP_00;
		case SDL_SCANCODE_KP_000:
			return SCANCODE_KP_000;
		case SDL_SCANCODE_KP_1:
			return SCANCODE_KP_1;
		case SDL_SCANCODE_KP_2:
			return SCANCODE_KP_2;
		case SDL_SCANCODE_KP_3:
			return SCANCODE_KP_3;
		case SDL_SCANCODE_KP_4:
			return SCANCODE_KP_4;
		case SDL_SCANCODE_KP_5:
			return SCANCODE_KP_5;
		case SDL_SCANCODE_KP_6:
			return SCANCODE_KP_6;
		case SDL_SCANCODE_KP_7:
			return SCANCODE_KP_7;
		case SDL_SCANCODE_KP_8:
			return SCANCODE_KP_8;
		case SDL_SCANCODE_KP_9:
			return SCANCODE_KP_9;
		case SDL_SCANCODE_KP_A:
			return SCANCODE_KP_A;
		case SDL_SCANCODE_KP_AMPERSAND:
			return SCANCODE_KP_AMPERSAND;
		case SDL_SCANCODE_KP_AT:
			return SCANCODE_KP_AT;
		case SDL_SCANCODE_KP_B:
			return SCANCODE_KP_B;
		case SDL_SCANCODE_KP_BACKSPACE:
			return SCANCODE_KP_BACKSPACE;
		case SDL_SCANCODE_KP_BINARY:
			return SCANCODE_KP_BINARY;
		case SDL_SCANCODE_KP_C:
			return SCANCODE_KP_C;
		case SDL_SCANCODE_KP_CLEAR:
			return SCANCODE_KP_CLEAR;
		case SDL_SCANCODE_KP_CLEARENTRY:
			return SCANCODE_KP_CLEARENTRY;
		case SDL_SCANCODE_KP_COLON:
			return SCANCODE_KP_COLON;
		case SDL_SCANCODE_KP_COMMA:
			return SCANCODE_KP_COMMA;
		case SDL_SCANCODE_KP_D:
			return SCANCODE_KP_D;
		case SDL_SCANCODE_KP_DBLAMPERSAND:
			return SCANCODE_KP_DBLAMPERSAND;
		case SDL_SCANCODE_KP_DBLVERTICALBAR:
			return SCANCODE_KP_DBLVERTICALBAR;
		case SDL_SCANCODE_KP_DECIMAL:
			return SCANCODE_KP_DECIMAL;
		case SDL_SCANCODE_KP_DIVIDE:
			return SCANCODE_KP_DIVIDE;
		case SDL_SCANCODE_KP_E:
			return SCANCODE_KP_E;
		case SDL_SCANCODE_KP_ENTER:
			return SCANCODE_KP_ENTER;
		case SDL_SCANCODE_KP_EQUALS:
			return SCANCODE_KP_EQUALS;
		case SDL_SCANCODE_KP_EQUALSAS400:
			return SCANCODE_KP_EQUALSAS400;
		case SDL_SCANCODE_KP_EXCLAM:
			return SCANCODE_KP_EXCLAM;
		case SDL_SCANCODE_KP_F:
			return SCANCODE_KP_F;
		case SDL_SCANCODE_KP_GREATER:
			return SCANCODE_KP_GREATER;
		case SDL_SCANCODE_KP_HASH:
			return SCANCODE_KP_HASH;
		case SDL_SCANCODE_KP_HEXADECIMAL:
			return SCANCODE_KP_HEXADECIMAL;
		case SDL_SCANCODE_KP_LEFTBRACE:
			return SCANCODE_KP_LEFTBRACE;
		case SDL_SCANCODE_KP_LEFTPAREN:
			return SCANCODE_KP_LEFTPAREN;
		case SDL_SCANCODE_KP_LESS:
			return SCANCODE_KP_LESS;
		case SDL_SCANCODE_KP_MEMADD:
			return SCANCODE_KP_MEMADD;
		case SDL_SCANCODE_KP_MEMCLEAR:
			return SCANCODE_KP_MEMCLEAR;
		case SDL_SCANCODE_KP_MEMDIVIDE:
			return SCANCODE_KP_MEMDIVIDE;
		case SDL_SCANCODE_KP_MEMMULTIPLY:
			return SCANCODE_KP_MEMMULTIPLY;
		case SDL_SCANCODE_KP_MEMRECALL:
			return SCANCODE_KP_MEMRECALL;
		case SDL_SCANCODE_KP_MEMSTORE:
			return SCANCODE_KP_MEMSTORE;
		case SDL_SCANCODE_KP_MEMSUBTRACT:
			return SCANCODE_KP_MEMSUBTRACT;
		case SDL_SCANCODE_KP_MINUS:
			return SCANCODE_KP_MINUS;
		case SDL_SCANCODE_KP_MULTIPLY:
			return SCANCODE_KP_MULTIPLY;
		case SDL_SCANCODE_KP_OCTAL:
			return SCANCODE_KP_OCTAL;
		case SDL_SCANCODE_KP_PERCENT:
			return SCANCODE_KP_PERCENT;
		case SDL_SCANCODE_KP_PERIOD:
			return SCANCODE_KP_PERIOD;
		case SDL_SCANCODE_KP_PLUS:
			return SCANCODE_KP_PLUS;
		case SDL_SCANCODE_KP_PLUSMINUS:
			return SCANCODE_KP_PLUSMINUS;
		case SDL_SCANCODE_KP_POWER:
			return SCANCODE_KP_POWER;
		case SDL_SCANCODE_KP_RIGHTBRACE:
			return SCANCODE_KP_RIGHTBRACE;
		case SDL_SCANCODE_KP_RIGHTPAREN:
			return SCANCODE_KP_RIGHTPAREN;
		case SDL_SCANCODE_KP_SPACE:
			return SCANCODE_KP_SPACE;
		case SDL_SCANCODE_KP_TAB:
			return SCANCODE_KP_TAB;
		case SDL_SCANCODE_KP_VERTICALBAR:
			return SCANCODE_KP_VERTICALBAR;
		case SDL_SCANCODE_KP_XOR:
			return SCANCODE_KP_XOR;
		case SDL_SCANCODE_L:
			return SCANCODE_L;
		case SDL_SCANCODE_LALT:
			return SCANCODE_LALT;
		case SDL_SCANCODE_LCTRL:
			return SCANCODE_LCTRL;
		case SDL_SCANCODE_LEFT:
			return SCANCODE_LEFT;
		case SDL_SCANCODE_LEFTBRACKET:
			return SCANCODE_LEFTBRACKET;
		case SDL_SCANCODE_LGUI:
			return SCANCODE_LGUI;
		case SDL_SCANCODE_LSHIFT:
			return SCANCODE_LSHIFT;
		case SDL_SCANCODE_M:
			return SCANCODE_M;
		case SDL_SCANCODE_MAIL:
			return SCANCODE_MAIL;
		case SDL_SCANCODE_MEDIASELECT:
			return SCANCODE_MEDIASELECT;
		case SDL_SCANCODE_MENU:
			return SCANCODE_MENU;
		case SDL_SCANCODE_MINUS:
			return SCANCODE_MINUS;
		case SDL_SCANCODE_MODE:
			return SCANCODE_MODE;
		case SDL_SCANCODE_MUTE:
			return SCANCODE_MUTE;
		case SDL_SCANCODE_N:
			return SCANCODE_N;
		case SDL_SCANCODE_NUMLOCKCLEAR:
			return SCANCODE_NUMLOCKCLEAR;
		case SDL_SCANCODE_O:
			return SCANCODE_O;
		case SDL_SCANCODE_OPER:
			return SCANCODE_OPER;
		case SDL_SCANCODE_OUT:
			return SCANCODE_OUT;
		case SDL_SCANCODE_P:
			return SCANCODE_P;
		case SDL_SCANCODE_PAGEDOWN:
			return SCANCODE_PAGEDOWN;
		case SDL_SCANCODE_PAGEUP:
			return SCANCODE_PAGEUP;
		case SDL_SCANCODE_PASTE:
			return SCANCODE_PASTE;
		case SDL_SCANCODE_PAUSE:
			return SCANCODE_PAUSE;
		case SDL_SCANCODE_PERIOD:
			return SCANCODE_PERIOD;
		case SDL_SCANCODE_POWER:
			return SCANCODE_POWER;
		case SDL_SCANCODE_PRINTSCREEN:
			return SCANCODE_PRINTSCREEN;
		case SDL_SCANCODE_PRIOR:
			return SCANCODE_PRIOR;
		case SDL_SCANCODE_Q:
			return SCANCODE_Q;
		case SDL_SCANCODE_R:
			return SCANCODE_R;
		case SDL_SCANCODE_RALT:
			return SCANCODE_RALT;
		case SDL_SCANCODE_RCTRL:
			return SCANCODE_RCTRL;
		case SDL_SCANCODE_RETURN:
			return SCANCODE_RETURN;
		case SDL_SCANCODE_RETURN2:
			return SCANCODE_RETURN2;
		case SDL_SCANCODE_RGUI:
			return SCANCODE_RGUI;
		case SDL_SCANCODE_RIGHT:
			return SCANCODE_RIGHT;
		case SDL_SCANCODE_RIGHTBRACKET:
			return SCANCODE_RIGHTBRACKET;
		case SDL_SCANCODE_RSHIFT:
			return SCANCODE_RSHIFT;
		case SDL_SCANCODE_S:
			return SCANCODE_S;
		case SDL_SCANCODE_SCROLLLOCK:
			return SCANCODE_SCROLLLOCK;
		case SDL_SCANCODE_SELECT:
			return SCANCODE_SELECT;
		case SDL_SCANCODE_SEMICOLON:
			return SCANCODE_SEMICOLON;
		case SDL_SCANCODE_SEPARATOR:
			return SCANCODE_SEPARATOR;
		case SDL_SCANCODE_SLASH:
			return SCANCODE_SLASH;
		case SDL_SCANCODE_SLEEP:
			return SCANCODE_SLEEP;
		case SDL_SCANCODE_SPACE:
			return SCANCODE_SPACE;
		case SDL_SCANCODE_STOP:
			return SCANCODE_STOP;
		case SDL_SCANCODE_SYSREQ:
			return SCANCODE_SYSREQ;
		case SDL_SCANCODE_T:
			return SCANCODE_T;
		case SDL_SCANCODE_TAB:
			return SCANCODE_TAB;
		case SDL_SCANCODE_THOUSANDSSEPARATOR:
			return SCANCODE_THOUSANDSSEPARATOR;
		case SDL_SCANCODE_U:
			return SCANCODE_U;
		case SDL_SCANCODE_UNDO:
			return SCANCODE_UNDO;
		case SDL_SCANCODE_UNKNOWN:
			return SCANCODE_UNKNOWN;
		case SDL_SCANCODE_UP:
			return SCANCODE_UP;
		case SDL_SCANCODE_V:
			return SCANCODE_V;
		case SDL_SCANCODE_VOLUMEDOWN:
			return SCANCODE_VOLUMEDOWN;
		case SDL_SCANCODE_VOLUMEUP:
			return SCANCODE_VOLUMEUP;
		case SDL_SCANCODE_W:
			return SCANCODE_W;
		case SDL_SCANCODE_WWW:
			return SCANCODE_WWW;
		case SDL_SCANCODE_X:
			return SCANCODE_X;
		case SDL_SCANCODE_Y:
			return SCANCODE_Y;
		case SDL_SCANCODE_Z:
			return SCANCODE_Z;
		case SDL_SCANCODE_INTERNATIONAL1:
			return SCANCODE_INTERNATIONAL1;
		case SDL_SCANCODE_INTERNATIONAL2:
			return SCANCODE_INTERNATIONAL2;
		case SDL_SCANCODE_INTERNATIONAL3:
			return SCANCODE_INTERNATIONAL3;
		case SDL_SCANCODE_INTERNATIONAL4:
			return SCANCODE_INTERNATIONAL4;
		case SDL_SCANCODE_INTERNATIONAL5:
			return SCANCODE_INTERNATIONAL5;
		case SDL_SCANCODE_INTERNATIONAL6:
			return SCANCODE_INTERNATIONAL6;
		case SDL_SCANCODE_INTERNATIONAL7:
			return SCANCODE_INTERNATIONAL7;
		case SDL_SCANCODE_INTERNATIONAL8:
			return SCANCODE_INTERNATIONAL8;
		case SDL_SCANCODE_INTERNATIONAL9:
			return SCANCODE_INTERNATIONAL9;
		case SDL_SCANCODE_LANG1:
			return SCANCODE_LANG1;
		case SDL_SCANCODE_LANG2:
			return SCANCODE_LANG2;
		case SDL_SCANCODE_LANG3:
			return SCANCODE_LANG3;
		case SDL_SCANCODE_LANG4:
			return SCANCODE_LANG4;
		case SDL_SCANCODE_LANG5:
			return SCANCODE_LANG5;
		case SDL_SCANCODE_LANG6:
			return SCANCODE_LANG6;
		case SDL_SCANCODE_LANG7:
			return SCANCODE_LANG7;
		case SDL_SCANCODE_LANG8:
			return SCANCODE_LANG8;
		case SDL_SCANCODE_LANG9:
			return SCANCODE_LANG9;
		case SDL_SCANCODE_NONUSBACKSLASH:
			return SCANCODE_NONUSBACKSLASH;
		case SDL_SCANCODE_NONUSHASH:
			return SCANCODE_NONUSHASH;

		default:
			return SCANCODE_UNKNOWN;
	}
}

uint16 OpenGlRenderer::convertSdlKeymod(const uint16 sdlKeymod)
{
    uint16 keymod = 0;
    keymod = keymod | (sdlKeymod & KMOD_NONE ? KEYMOD_NONE : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_LSHIFT ? KEYMOD_LSHIFT : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_RSHIFT ? KEYMOD_RSHIFT : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_LCTRL ? KEYMOD_LCTRL : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_RCTRL ? KEYMOD_RCTRL : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_LALT ? KEYMOD_LALT : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_RALT ? KEYMOD_RALT : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_LGUI ? KEYMOD_LGUI : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_RGUI ? KEYMOD_RGUI : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_NUM ? KEYMOD_NUM : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_CAPS ? KEYMOD_CAPS : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_MODE ? KEYMOD_MODE : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_RESERVED ? KEYMOD_RESERVED : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_CTRL ? KEYMOD_CTRL : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_SHIFT ? KEYMOD_SHIFT : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_ALT ? KEYMOD_ALT : KEYMOD_NONE);
    keymod = keymod | (sdlKeymod & KMOD_GUI ? KEYMOD_GUI : KEYMOD_NONE);

    return keymod;
}

void OpenGlRenderer::addEventListener(IEventListener* eventListener)
{
	if (eventListener == nullptr)
	{
		throw std::invalid_argument("IEventListener cannot be null.");
	}

	eventListeners_.push_back(eventListener);
}

void OpenGlRenderer::removeEventListener(IEventListener* eventListener)
{
	auto it = std::find(eventListeners_.begin(), eventListeners_.end(), eventListener);

	if (it != eventListeners_.end())
	{
		eventListeners_.erase( it );
	}
}

}
}
}
}
