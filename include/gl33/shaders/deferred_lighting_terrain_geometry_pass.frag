// Adapted from: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/8.1.deferred_shading/8.1.g_buffer.fs
#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec3 gMetallicRoughnessAmbientOcclusion;

in vec3 Position;
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform usampler2D terrainMapTexture;
uniform sampler2DArray splatMapAlbedoTextures;
uniform sampler2DArray splatMapNormalTextures;
uniform sampler2DArray splatMapMetallicRoughnessAmbientOcclusionTextures;

void main()
{
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // and the diffuse per-fragment color
    
    uint whichTexture0 = texture(terrainMapTexture, Position.xz/257).r;
    uint whichTexture1 = texture(terrainMapTexture, Position.xz/257).g;
    uint whichTexture2 = texture(terrainMapTexture, Position.xz/257).b;
    
    uint whichTexturePercentAsUint0 = texture(terrainMapTexture, Position.xz/257).a >> 4;
    uint whichTexturePercentAsUint1 = (texture(terrainMapTexture, Position.xz/257).a << 28) >> 28;
    
    float whichTexturePercent0 = float(whichTexturePercentAsUint0) / 16.0f;
    float whichTexturePercent1 = float(whichTexturePercentAsUint1) / 16.0f;
    float whichTexturePercent2 = 1.0f - whichTexturePercent0 - whichTexturePercent1;
    
    gAlbedoSpec.rgb = 	texture(splatMapAlbedoTextures, vec3(TexCoords, whichTexture0)).rgb * whichTexturePercent0 +
						texture(splatMapAlbedoTextures, vec3(TexCoords, whichTexture1)).rgb * whichTexturePercent1 +
						texture(splatMapAlbedoTextures, vec3(TexCoords, whichTexture2)).rgb * whichTexturePercent2
						;

    //gAlbedoSpec.rgb = vec3(0.3f, 0.3f, 0.3f);
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 0.1f; //texture(texture_specular1, TexCoords).r;
    
    gMetallicRoughnessAmbientOcclusion.rgb = 	texture(splatMapMetallicRoughnessAmbientOcclusionTextures, vec3(TexCoords, whichTexture0)).rgb * whichTexturePercent0 +
												texture(splatMapMetallicRoughnessAmbientOcclusionTextures, vec3(TexCoords, whichTexture1)).rgb * whichTexturePercent1 +
												texture(splatMapMetallicRoughnessAmbientOcclusionTextures, vec3(TexCoords, whichTexture2)).rgb * whichTexturePercent2
												;
}
