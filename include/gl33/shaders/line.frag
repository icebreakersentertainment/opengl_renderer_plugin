#version 330 core
in vec3 ourColor;
out vec4 color;

void main()
{
    color = vec4(ourColor, 1.0f);
    //gl_FragColor= vec4(1.0, 1.0, 0.0, 1.0);
}
