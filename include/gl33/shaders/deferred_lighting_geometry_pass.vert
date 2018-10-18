// Adapted from: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/8.1.deferred_shading/8.1.g_buffer.vs
#version 330 core

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 pvmMatrix;
uniform mat3 normalMatrix;
uniform bool hasBones = false;
uniform bool hasBoneAttachment = false;
uniform ivec4 boneAttachmentIds;
uniform vec4 boneAttachmentWeights;

layout (std140) uniform Bones
{
	mat4 bones[100];
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in ivec4 boneIds;
layout (location = 5) in vec4 boneWeights;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

void main()
{
	vec4 tempPosition = vec4(position, 1.0);
	
	if (hasBones)
	{
		// Calculate the transformation on the vertex position based on the bone weightings
		mat4 boneTransform = bones[ boneIds[0] ] * boneWeights[0];
		boneTransform     += bones[ boneIds[1] ] * boneWeights[1];
		boneTransform     += bones[ boneIds[2] ] * boneWeights[2];
		boneTransform     += bones[ boneIds[3] ] * boneWeights[3];
	
		//mat4 tempM = mat4(1.0);
		//boneTransform = tempM;
	
		// This is for animating the model
		tempPosition = boneTransform * vec4(position, 1.0);
	}
	if (hasBoneAttachment)
	{
		mat4 boneTransform = bones[ boneAttachmentIds[0] ] * boneAttachmentWeights[0];
		boneTransform     += bones[ boneAttachmentIds[1] ] * boneAttachmentWeights[1];
		boneTransform     += bones[ boneAttachmentIds[2] ] * boneAttachmentWeights[2];
		boneTransform     += bones[ boneAttachmentIds[3] ] * boneAttachmentWeights[3];
		
		tempPosition = boneTransform * vec4(position, 1.0);
	}

//	vec4 worldPos = modelMatrix * vec4(position, 1.0);
    vec4 worldPos = modelMatrix * tempPosition;
    FragPos = worldPos.xyz; 
    TexCoords = textureCoordinate;
    
    mat3 normalMatrix2 = transpose(inverse(mat3(modelMatrix)));
    Normal = normalMatrix2 * normal;
    
//    gl_Position = pvmMatrix * vec4(position, 1.0);
    gl_Position = pvmMatrix * tempPosition;

    //gl_Position = projection * view * worldPos;
    
	//texCoord = textureCoordinate;
}
