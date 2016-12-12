#include "parametric_shapes.hpp"
#include "core/utils.h"

#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

eda221::mesh_data
parametric_shapes::createQuad(unsigned int width, unsigned int height)
{
	auto const vertices_nb = 4;

	//! \todo Fill in the blanks
	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);


	vertices = {
		glm::vec3(-0.5*width,0.5*height,0.0),
		glm::vec3(0.5*width, 0.5*height,0.0 ),
		glm::vec3(0.5*width, -0.5*height, 0.0),
		glm::vec3(-0.5*width, -0.5*height, 0.0)
	};
	texcoords = {
		glm::vec3(0.0,0.0,0.0),
		glm::vec3(1.0,0.0,0.0),
		glm::vec3(1.0,1.0,0.0),
		glm::vec3(0.0,1.0,0.0)
	};

	auto indices = std::vector<glm::uvec3>(2);
	indices = {
		glm::uvec3(0,1,2),
		glm::uvec3(0,2,3)
	};


	eda221::mesh_data data;

	//
	// NOTE:
	//
	// Only the values preceeded by a `\todo` tag should be changed, the
	// other ones are correct!
	//

	// Create a Vertex Array Object: it will remember where we stored the
	// data on the GPU, and  which part corresponds to the vertices, which
	// one for the normals, etc.
	//
	// The following function will create new Vertex Arrays, and pass their
	// name in the given array (second argument). Since we only need one,
	// pass a pointer to `data.vao`.
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	// To be able to store information, the Vertex Array has to be bound
	// first.
	glBindVertexArray(data.vao);

	// To store the data, we need to allocate buffers on the GPU. Let's
	// allocate a first one for the vertices.
	//
	// The following function's syntax is similar to `glGenVertexArray()`:
	// it will create multiple OpenGL objects, in this case buffers, and
	// return their names in an array. Have the buffer's name stored into
	// `data.bo`.
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	// Similar to the Vertex Array, we need to bind it first before storing
	// anything in it. The data stored in it can be interpreted in
	// different ways. Here, we will say that it is just a simple 1D-array
	// and therefore bind the buffer to the corresponding target.
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const texcoords_offset = vertices_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		+ texcoords_size
		);

	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);


	return data;
}


eda221::mesh_data
parametric_shapes::createQuad(unsigned int res_width, unsigned int res_height, unsigned int width, unsigned int height)
{
	auto const vertices_nb = res_width * res_height;

	//! \todo Fill in the blanks
	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float w = 0.0f,
		dw = width / (static_cast<float>(res_width) - 1.0f);
	float h = 0.0f,
		dh = height / (static_cast<float>(res_height) - 1.0f);

	size_t index = 0u;
	for (unsigned int i = 0; i < res_height; i++) {
		w = 0.0f;
		for (unsigned int j = 0; j < res_width; j++) {
			// vertex
			vertices[index] = glm::vec3(w, 0, h);

			//texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_width) - 1.0f),
				static_cast<float>(i) / (static_cast<float>(res_height) - 1.0f),
				0.0f);

			// tangent
			auto t = glm::vec3(0.0f, 0.0f, 1.0f);
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(1.0f, 0.0f, 0.0f);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			++index;
			w += dw;
		}
		h += dh;

	}


	auto indices = std::vector<glm::uvec3>(2u * (res_width - 1u) * (res_height - 1u));

	index = 0u;
	for (unsigned int i = 0u; i < res_height - 1u; ++i)
	{
		for (unsigned int j = 0u; j < res_width - 1u; ++j)
		{
			indices[index] = glm::uvec3(res_width * i + j,
				res_width * i + j + 1u,
				res_width * i + j + 1u + res_width);
			++index;

			indices[index] = glm::uvec3(res_width * i + j,
				res_width * i + j + res_width + 1u,
				res_width * i + j + res_width);
			++index;
		}
	}


	eda221::mesh_data data;

	//
	// NOTE:
	//
	// Only the values preceeded by a `\todo` tag should be changed, the
	// other ones are correct!
	//

	// Create a Vertex Array Object: it will remember where we stored the
	// data on the GPU, and  which part corresponds to the vertices, which
	// one for the normals, etc.
	//
	// The following function will create new Vertex Arrays, and pass their
	// name in the given array (second argument). Since we only need one,
	// pass a pointer to `data.vao`.
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	// To be able to store information, the Vertex Array has to be bound
	// first.
	glBindVertexArray(data.vao);

	// To store the data, we need to allocate buffers on the GPU. Let's
	// allocate a first one for the vertices.
	//
	// The following function's syntax is similar to `glGenVertexArray()`:
	// it will create multiple OpenGL objects, in this case buffers, and
	// return their names in an array. Have the buffer's name stored into
	// `data.bo`.
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	// Similar to the Vertex Array, we need to bind it first before storing
	// anything in it. The data stored in it can be interpreted in
	// different ways. Here, we will say that it is just a simple 1D-array
	// and therefore bind the buffer to the corresponding target.
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		+ normals_size
		+ texcoords_size
		/*+ tangents_size
		+ binormals_size*/
		);

	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	/*glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));*/

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);


	return data;
}

eda221::mesh_data
parametric_shapes::createSphere(unsigned int const res_theta,
	unsigned int const res_phi, float const radius)
{
	//! \todo Implement this function
	auto const vertices_nb = res_phi * res_theta;

	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float theta = 0.0f,
		dtheta = 2.0f*bonobo::pi / (static_cast<float>(res_theta) - 1.0f);
	float phi = 0.0f,
		dphi = bonobo::pi / (static_cast<float>(res_phi) - 1.0f);

	// generate vertices iteratively
	size_t index = 0u;
	for (unsigned int i = 0u; i < res_theta; ++i) {
		float cos_theta = std::cos(theta),
			sin_theta = std::sin(theta);

		phi = 0.0f;

		for (unsigned int j = 0u; j < res_phi; ++j) {
			// vertex
			float cos_phi = std::cos(phi),
				sin_phi = std::sin(phi);
			vertices[index] = glm::vec3(radius * sin_theta*sin_phi,
				-radius*cos(phi),
				radius*cos_theta*sin_phi);

			//texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(i) / (static_cast<float>(res_theta) - 1.0f), static_cast<float>(j) / (static_cast<float>(res_phi) - 1.0f),
				0.0f);

			// tangent
			auto t = glm::vec3(radius*cos_theta*(sin_phi + 0.01f), 0.0f, -radius*sin_theta*(sin_phi + 0.01f));
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(radius*sin_theta*cos_phi, radius*sin_phi, radius*cos_theta*cos_phi);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			phi += dphi;
			++index;
		}

		theta += dtheta;
	}

	// create index array
	auto indices = std::vector<glm::uvec3>(2u * (res_theta - 1u) * (res_phi - 1u));

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < res_theta - 1u; ++i)
	{
		for (unsigned int j = 0u; j < res_phi - 1u; ++j)
		{
			indices[index] = glm::uvec3(res_phi * i + j,
				res_phi * i + j + 1u,
				res_phi * i + j + 1u + res_phi);
			++index;

			indices[index] = glm::uvec3(res_phi * i + j,
				res_phi * i + j + res_phi + 1u,
				res_phi * i + j + res_phi);
			++index;
		}
	}

	eda221::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		+ normals_size
		+ texcoords_size
		+ tangents_size
		+ binormals_size
		);
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}

//eda221::mesh_data
//parametric_shapes::createTorus(unsigned int const res_theta,
//	unsigned int const res_phi, float const rA,
//	float const rB)
//{
//	//! \todo (Optional) Implement this function
//	return eda221::mesh_data{ 0u, 0u, 0u, 0u };
//}

eda221::mesh_data
parametric_shapes::createCircleRing(unsigned int const res_radius,
	unsigned int const res_theta,
	float const inner_radius,
	float const outer_radius)
{
	auto const vertices_nb = res_radius * res_theta;

	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float theta = 0.0f,                                                        // 'stepping'-variable for theta: will go 0 - 2PI
		dtheta = 2.0f * bonobo::pi / (static_cast<float>(res_theta) - 1.0f); // step size, depending on the resolution

	float radius = 0.0f,                                                                     // 'stepping'-variable for radius: will go inner_radius - outer_radius
		dradius = (outer_radius - inner_radius) / (static_cast<float>(res_radius) - 1.0f); // step size, depending on the resolution

																						   // generate vertices iteratively
	size_t index = 0u;
	for (unsigned int i = 0u; i < res_theta; ++i) {
		float cos_theta = std::cos(theta),
			sin_theta = std::sin(theta);

		radius = inner_radius;

		for (unsigned int j = 0u; j < res_radius; ++j) {
			// vertex
			vertices[index] = glm::vec3(radius * cos_theta,
				radius * sin_theta,
				0.0f);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(res_radius) - 1.0f),
				static_cast<float>(i) / (static_cast<float>(res_theta) - 1.0f),
				0.0f);

			// tangent
			auto t = glm::vec3(cos_theta, sin_theta, 0.0f);
			t = glm::normalize(t);
			tangents[index] = t;

			// binormal
			auto b = glm::vec3(-sin_theta, cos_theta, 0.0f);
			b = glm::normalize(b);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			radius += dradius;
			++index;
		}

		theta += dtheta;
	}

	// create index array
	auto indices = std::vector<glm::uvec3>(2u * (res_theta - 1u) * (res_radius - 1u));

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < res_theta - 1u; ++i)
	{
		for (unsigned int j = 0u; j < res_radius - 1u; ++j)
		{
			indices[index] = glm::uvec3(res_radius * i + j,
				res_radius * i + j + 1u,
				res_radius * i + j + 1u + res_radius);
			++index;

			indices[index] = glm::uvec3(res_radius * i + j,
				res_radius * i + j + res_radius + 1u,
				res_radius * i + j + res_radius);
			++index;
		}
	}

	eda221::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		+ normals_size
		+ texcoords_size
		+ tangents_size
		+ binormals_size
		);
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = indices.size() * 3u;
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(indices.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}