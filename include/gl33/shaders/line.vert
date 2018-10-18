// Source: http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=11517
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 ourColor;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

void main()
{
    gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0f);

    ourColor = color;
}
