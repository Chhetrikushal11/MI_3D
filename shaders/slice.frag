#version 410 core
out vec4 FragColor;
in vec2 texCoord;
uniform sampler3D uVolume;
uniform float uSliceZ;
uniform float uWindowCenter;
uniform float uWindowWidth;
uniform int uViewMode;
uniform float uRescaleSlope;
uniform float uRescaleIntercept;

void main()
{
    vec3 coord;
    if (uViewMode == 0)
        coord = vec3(texCoord, uSliceZ);
    else if (uViewMode == 1)
        coord = vec3(uSliceZ, texCoord.y, texCoord.x);
    else
        coord = vec3(texCoord.x, uSliceZ, texCoord.y);

    float raw = texture(uVolume, coord).r * 32767.0;
    float hu = raw * uRescaleSlope + uRescaleIntercept;

    float minHU = uWindowCenter - uWindowWidth / 2.0;
    float brightness = (hu - minHU) / uWindowWidth;
    brightness = clamp(brightness, 0.0, 1.0);

    FragColor = vec4(brightness, brightness, brightness, 1.0);
}