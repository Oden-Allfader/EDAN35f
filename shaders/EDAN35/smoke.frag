#version 410
uniform vec2 inv_res; //
uniform sampler2D depth_texture;
uniform sampler2D diffuse_texture; //
uniform float time; //
uniform float number; //
uniform vec3 billboard_position; //
uniform vec3 camera_position; //
uniform mat4 vertex_world_to_clip; //
in VS_OUT {
	vec3 vertex;
	vec2 texCoord;
} fs_in;

out vec4 frag_color;

void main()
{	
/*	vec2 screenCoord = inv_res*gl_FragCoord.xy;

	vec4 screenBillboard_position = (vertex_world_to_clip*vec4(billboard_position,1.0));
	vec4 screenCamera_position = (vertex_world_to_clip*vec4(camera_position,1.0));

	vec3 clipBillboard_Position = (screenBillboard_position.xyz)/screenBillboard_position.w;
	vec3 clipCamera_Position = (screenCamera_position.xyz)/screenCamera_position.w;

	float dist = distance(clipCamera_position,clipBillboard_position);
	float depth = texture(depth_texture,screenCoord).x;
	if(depth*2-1<dist){
		discard;
	}*/
	vec2 uv = fs_in.texCoord;
	uv = uv*2-1;
	float a = sin(time/4);
	float b = cos(time/4);
	mat2 rotation = mat2(b , -a , a , b);
	uv = rotation*uv;
	uv = uv/2+0.5;
	uv.x = clamp(uv.x,0.0,1.0);
	uv.y = clamp(uv.y,0.0,1.0);
	vec4 color  = texture(diffuse_texture,uv);
	color.w *= pow((10-number)/10,2);
	frag_color = color;
}
