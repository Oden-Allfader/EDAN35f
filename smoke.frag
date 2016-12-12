#version 410

uniform vec3 light_position;
uniform sampler2D diffuse_texture;
uniform float time;
uniform float number;
in VS_OUT {
	vec3 vertex;
	vec2 texCoord;
} fs_in;

out vec4 frag_color;

void main()
{	
	vec2 uv = fs_in.texCoord;
	/*uv = uv*2-1;
	float a = sin(time);
	float b = cos(time);
	mat2 rotation = mat2(b , -a , a , b);
	uv = rotation*uv;
	uv = uv/2+0.5;*/
	vec4 color  = texture(diffuse_texture,uv);
	color.w *= pow((10-number)/10,2);
	frag_color = color;
}
