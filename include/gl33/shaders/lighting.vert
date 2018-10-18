// Source: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/8.1.deferred_shading/8.1.deferred_shading.vs
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
