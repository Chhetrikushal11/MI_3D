#version 410 core
in vec2 texCoord;
uniform sampler2D uEntryTexture;
uniform sampler2D uExitTexture;
uniform sampler3D uVolume;
uniform float uStepSize;
uniform sampler1D uTransferFunc;
uniform float uClipMin;
uniform float uClipMax;
uniform float uRescaleSlope;
uniform float uRescaleIntercept;
out vec4 FragColor;

void main()
{
    vec3 entry = texture(uEntryTexture, texCoord).rgb;
    vec3 exit_ = texture(uExitTexture, texCoord).rgb;
    vec3 dir = exit_ - entry;
    float rayLen = length(dir);

    if (rayLen < 0.001)
    {
        FragColor = vec4(0.0);
        return;
    }

    dir = normalize(dir);
    vec3 pos = entry;
    vec4 accumulated = vec4(0.0);

    for (float t = 0.0; t < rayLen; t += uStepSize)
    {
        if (pos.x >= uClipMin && pos.x <= uClipMax)
        {
            float raw = texture(uVolume, pos).r;
            float hu = raw * 32767.0 * uRescaleSlope + uRescaleIntercept;
            float normalized = clamp((hu + 1024.0) / 4096.0, 0.0, 1.0);
            vec4 color = texture(uTransferFunc, normalized);

            if (color.a > 0.01)
            {
                float dx = texture(uVolume, pos + vec3(0.005, 0.0, 0.0)).r
                         - texture(uVolume, pos - vec3(0.005, 0.0, 0.0)).r;
                float dy = texture(uVolume, pos + vec3(0.0, 0.005, 0.0)).r
                         - texture(uVolume, pos - vec3(0.0, 0.005, 0.0)).r;
                float dz = texture(uVolume, pos + vec3(0.0, 0.0, 0.005)).r
                         - texture(uVolume, pos - vec3(0.0, 0.0, 0.005)).r;
                vec3 normal = normalize(vec3(dx, dy, dz));
                vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
                float diffuse = max(dot(normal, lightDir), 0.0);
                float lit = 0.2 + 0.8 * diffuse;
                color.rgb *= lit;
            }

            color.a *= uStepSize * 100.0;
            accumulated.rgb += (1.0 - accumulated.a) * color.a * color.rgb;
            accumulated.a += (1.0 - accumulated.a) * color.a;
        }

        if (accumulated.a > 0.99) break;
        pos += dir * uStepSize;
    }

    FragColor = accumulated;
}