
#version 410 core

out vec4 FragColor;

in vec2 texCoord;

uniform sampler3D uVolume;
uniform float uSliceZ;
uniform float uWindowCenter;
uniform float uWindowWidth;
uniform int uViewMode;

void main()
{
    vec3 coord;
    if (uViewMode == 0)       // axial
        coord = vec3(texCoord, uSliceZ);
    else if (uViewMode == 1)  // sagittal
        coord = vec3(uSliceZ, texCoord.y, texCoord.x);
    else                       // coronal
        coord = vec3(texCoord.x, uSliceZ, texCoord.y);
    float density = texture(uVolume, coord).r;
    density = density * 32767.0;    // convert back from normalized to HU range
    float minHU = uWindowCenter - uWindowWidth / 2.0;
    float brightness = (density - minHU) / uWindowWidth;
    brightness = clamp(brightness, 0.0, 1.0);
    FragColor = vec4(brightness, brightness, brightness, 1.0);
}