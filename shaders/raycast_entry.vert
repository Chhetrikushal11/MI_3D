#version 410 core

layout(location = 0) in vec3 aPos;


uniform mat4 uViewProjection;

out vec3 worldPos; 

void main()
{
    worldPos = aPos;
    gl_Position = uViewProjection * vec4(aPos, 1.0); 
}