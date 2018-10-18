// Adapted from: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/8.1.deferred_shading/8.1.g_buffer.vs
#version 330 core

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 pvmMatrix;
uniform mat3 normalMatrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 textureCoordinate;

uniform sampler2D heightMapTexture;

out vec3 Position;
out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

const float MAX_HEIGHT = 15.0f;

void main()
{
	Position = position;

	vec4 textureValue = texture(heightMapTexture, position.xz/257);
	//float height = MAX_HEIGHT * (textureValue.a - 1.0f);
	float height = MAX_HEIGHT * (textureValue.a - 0.5f);
	//float height = MAX_HEIGHT * textureValue.a;
	vec3 newPosition = vec3(position.x, height, position.z);
	
    vec4 worldPos = modelMatrix * vec4(newPosition, 1.0);
    FragPos = worldPos.xyz; 
    /*
    TexCoords = textureCoordinate;
    */
    mat3 normalMatrix2 = transpose(inverse(mat3(modelMatrix)));
    Normal = normalMatrix2 * textureValue.rgb;
    
    TexCoords = position.xz/16;
    
    gl_Position = pvmMatrix * vec4(newPosition, 1.0);

    //gl_Position = projection * view * worldPos;
    
	//texCoord = textureCoordinate;
}
