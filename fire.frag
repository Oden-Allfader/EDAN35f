#version 410

uniform vec3 light_position;
uniform sampler2D diffuse_texture;
uniform float time;
in VS_OUT {
	vec3 vertex;
	vec2 texCoord;
} fs_in;

out vec4 frag_color;

void main()
{	
	vec2 uv = fs_in.texCoord;
    // Generate noisy x value
    vec2 n0Uv = vec2(uv.x*1.4 + 0.01, uv.y + time*0.69);
    vec2 n1Uv = vec2(uv.x*0.5 - 0.033, uv.y*2.0 + time*0.12);
    vec2 n2Uv = vec2(uv.x*0.94 + 0.02, uv.y*3.0 + time*0.61);
    float n0 = (texture2D(diffuse_texture, n0Uv).w-0.5)*2.0;
    float n1 = (texture2D(diffuse_texture, n1Uv).w-0.5)*2.0;
    float n2 = (texture2D(diffuse_texture, n2Uv).w-0.5)*2.0;
    float noiseA = clamp(n0 + n1 + n2, -1.0, 1.0);

    // Generate noisy y value
    vec2 n0UvB = vec2(uv.x*0.7 - 0.01, uv.y + time*0.27);
    vec2 n1UvB = vec2(uv.x*0.45 + 0.033, uv.y*1.9 + time*0.61);
    vec2 n2UvB = vec2(uv.x*0.8 - 0.02, uv.y*2.5 + time*0.51);
    float n0B = (texture2D(diffuse_texture, n0UvB).w-0.5)*2.0;
    float n1B = (texture2D(diffuse_texture, n1UvB).w-0.5)*2.0;
    float n2B = (texture2D(diffuse_texture, n2UvB).w-0.5)*2.0;
    float noiseB = clamp(n0B + n1B + n2B, -1.0, 1.0);

	vec2 finalNoise = vec2(noiseA, noiseB);
    float perturb = (1.0 - uv.y) * 0.35 + 0.02;
    finalNoise = (finalNoise * perturb) + uv - 0.02;

	vec4 color = texture2D(diffuse_texture, finalNoise);
	color = vec4(color.x*2.0, color.y*0.9, (color.y/color.x)*0.2, 1.0);
    finalNoise = clamp(finalNoise, 0.05, 1.0);
    color.w = texture2D(diffuse_texture, finalNoise).z*2.0;
    color.w = color.w*texture2D(diffuse_texture, uv).z;


	frag_color = color;
}
