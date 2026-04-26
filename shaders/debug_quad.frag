#version 410 core
in vec2 texCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
void main()
{
    FragColor = texture(uTexture, texCoord);
}