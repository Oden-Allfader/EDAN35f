#version 410

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normals_texture;
uniform sampler2D opacity_texture;
uniform bool has_opacity_texture;
uniform mat4 normal_model_to_world;

in VS_OUT {
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
} fs_in;

layout (location = 0) out vec4 geometry_diffuse;
layout (location = 1) out vec4 geometry_specular;
layout (location = 2) out vec4 geometry_normal;


void main()
{
	if (has_opacity_texture && texture(opacity_texture, fs_in.texcoord).r < 1.0)
		discard;

	// Diffuse color
	geometry_diffuse = texture(diffuse_texture, fs_in.texcoord);

	// Specular color
	geometry_specular = texture(specular_texture, fs_in.texcoord);

	// Worldspace normal
	vec3 normal = (normal_model_to_world*vec4(normalize(fs_in.normal),0.0)).xyz;
	geometry_normal.xyz = normal*0.5 + 0.5;
	//geometry_normal.xyz = texture(normals_texture,fs_in.texcoord).xyz;
}
