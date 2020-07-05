// Source: https://learnopengl.com/Advanced-OpenGL/Cubemaps
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

void main()
{
    TexCoords = aPos;
    // gl_Position = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
    vec4 pos = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
