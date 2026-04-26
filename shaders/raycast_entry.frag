
#version 410 core

in vec3 worldPos;

out vec4 FragColor;

void main()
{
    FragColor = vec4(worldPos + 0.5, 1.0);
}
