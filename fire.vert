#version 410

// Remember how we enabled vertex attributes in assignment 2 and attached some
// data to each of them, here we retrieve that data. Attribute 0 pointed to the
// vertices inside the OpenGL buffer object, so if we say that our input
// variable `vertex` is at location 0, which corresponds to attribute 0 of our
// vertex array, vertex will be effectively filled with vertices from our
// buffer.
// Similarly, normal is set to location 1, which corresponds to attribute 1 of
// the vertex array, and therefore will be filled with normals taken out of our
// buffer.
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoords;
uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 billboard_position;
uniform vec3 camera_position;
uniform mat4 vertex_world_to_view;
uniform mat4 vertex_view_to_clip;
uniform float time;

// This is the custom output of this shader. If you want to retrieve this data
// from another shader further down the pipeline, you need to declare the exact
// same structure as in (for input), with matching name for the structure
// members and matching structure type. Have a look at
// shaders/EDA221/diffuse.frag.
out VS_OUT {
	vec3 vertex;
	vec2 texCoord;
} vs_out;


void main()
{	
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	mat4 ModelView = vertex_world_to_view*vertex_model_to_world;
	// Column 0:
	ModelView[0][0] = 1;
	ModelView[0][1] = 0;
	ModelView[0][2] = 0;

	// Column 1:
	ModelView[1][0] = 0;
	ModelView[1][1] = 1;
	ModelView[1][2] = 0;

	// Column 2:
	ModelView[2][0] = 0;
	ModelView[2][1] = 0;
	ModelView[2][2] = 1;


	vs_out.texCoord = texcoords;
	gl_Position = vertex_view_to_clip   *ModelView   * vec4(vertex, 1.0);
}



