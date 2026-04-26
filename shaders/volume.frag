#version 410 core

in vec2 texCoord;
uniform sampler2D uEntryTexture;
uniform sampler2D uExitTexture;
uniform sampler3D uVolume;
uniform float uStepSize;

uniform sampler1D uTransferFunc;

uniform float uClipMin;
uniform float uClipMax;

out vec4 FragColor;

   vec3 computeGradient(vec3 pos, float offset)
   {
		float dx = texture(uVolume, pos + vec3(offset, 0.0, 0.0)).r - texture(uVolume, pos - vec3(offset, 0.0, 0.0)).r;

		float dy = texture(uVolume, pos + vec3(0.0,offset, 0.0)).r - texture(uVolume, pos - vec3(0.0, offset, 0.0)).r;

		float dz = texture(uVolume, pos + vec3(0.0,0.0, offset)).r - texture(uVolume, pos - vec3(0.0, 0.0, offset)).r;

		return normalize(vec3(dx, dy, dz));
   }

void main()
{
   vec3 entry = texture(uEntryTexture, texCoord).rgb;
   vec3 exit = texture(uExitTexture, texCoord).rgb;
   vec3 dir = exit - entry ;
   float rayLen = length(dir);
   dir = normalize(dir);

   vec3 pos = entry;

   vec4 accumulated = vec4(0.0);



   for (float t = 0.0; t < rayLen ; t += uStepSize)
   {
		// ----Clip Check Start -----
		if  (pos.x >= uClipMin && pos.x <= uClipMax)
		{
			float density = texture(uVolume, pos).r * 32767.0;
			float normalized = (density + 1000.0) / 2000.0;
			vec4 color = texture(uTransferFunc, normalized);

		// -------Cliping Check End --------


			if (color.a > 0.01)
			{
				vec3 normal = computeGradient(pos, 0.005);
				vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
				float diffuse = max(dot(normal, lightDir), 0.0);
				float ambient = 0.2;
				float lit = ambient + 0.8 * diffuse;
				color.rgb *= lit;
			}

		// fornt - to - back compositing
		color.a *=  uStepSize * 100.0;
		accumulated.rgb += (1.0 - accumulated.a) * color.a * color.rgb;
		accumulated.a += (1.0 - accumulated.a) * color.a;
			
		}
		if (accumulated.a > 0.99) break;
		pos += dir * uStepSize;
   }


   FragColor = accumulated;

}
