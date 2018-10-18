// Adapted from: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/8.1.deferred_shading/8.1.g_buffer.fs
#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec3 gMetallicRoughnessAmbientOcclusion;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
//uniform sampler2D albedoTextures;
uniform sampler2D normalTextures;
uniform sampler2D metallicRoughnessAmbientOcclusionTextures;
//uniform sampler2D texture_specular1;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 0.1f; //texture(texture_specular1, TexCoords).r;
    
    //gMetallicRoughnessAmbientOcclusion.rgb = vec3(0.0f, 0.0f, 0.0f);
    gMetallicRoughnessAmbientOcclusion.rgb = texture(metallicRoughnessAmbientOcclusionTextures, TexCoords).rgb;
}
