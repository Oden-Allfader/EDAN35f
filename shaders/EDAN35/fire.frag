#version 410
uniform vec2 inv_res; //
uniform sampler2D depth_texture;
uniform sampler2D diffuse_texture; //

uniform float time; //
uniform vec3 billboard_position; //
uniform vec3 camera_position; //
uniform mat4 vertex_world_to_clip; //
uniform mat4 view_projection_inverse;

in VS_OUT {
	vec3 vertex;
	vec2 texCoord;
} fs_in;

out vec4 frag_color;

void main()
{	
	vec2 screenCoord = inv_res*gl_FragCoord.xy;
	float depth = texture2D(depth_texture,screenCoord).x;

	vec4 world_position = view_projection_inverse*vec4(screenCoord*2-1,2*depth-1,1.0);
	world_position /= world_position.w;
	
	vec3 norm_world = normalize(world_position.xyz);
	vec3 norm_billboard = normalize(camera_position);

	float distance_from_camera_to_buffer = distance(world_position.xyz,camera_position);
	float distance_from_camera_to_billboard = distance(billboard_position,camera_position);
/*	if(distance_from_camera_to_buffer < distance_from_camera_to_billboard){
		discard;
	}

	vec4 screenBillboard_position = (vertex_world_to_clip*vec4(billboard_position,1.0));
	vec4 screenCamera_position = (vertex_world_to_clip*vec4(camera_position,1.0));
	float zb = screenBillboard_position.z;
	vec3 clipBillboard_position = vec3(screenBillboard_position.x/zb,screenBillboard_position.y/zb,screenBillboard_position.z/zb);
	float zc = screenCamera_position.w;
	vec3 clipCamera_position = vec3(screenCamera_position.x/zc,screenCamera_position.y/zc,screenCamera_position.z/zc);
	
	//float dist = distance(clipBillboard_position,clipCamera_position);
	float dist = (clipCamera_position.z);
	
	float bias = 1.0;
	
	if((depth)<(dist-bias)){
		discard;
	}*/


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
	
	//color = texture2D(depth_texture,uv);
	frag_color = color;
}
