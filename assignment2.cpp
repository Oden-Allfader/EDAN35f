#include "assignment2.hpp"
#include "helpers.hpp"
#include "node.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/GLStateInspection.h"
#include "core/GLStateInspectionView.h"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <cstdlib>
#include <stdexcept>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

namespace constant
{
	constexpr uint32_t shadowmap_res_x = 4096;
	constexpr uint32_t shadowmap_res_y = 4096;

	constexpr size_t lights_nb           = 6;
	constexpr float  light_intensity     = 720000.0f;
	constexpr float  light_angle_falloff = 0.8f;
	constexpr float  light_cutoff        = 0.05f;

	constexpr int MaxFires = 10;
	constexpr int MaxSmokes = 30;
}

static eda221::mesh_data loadCone();

edan35::Assignment2::Assignment2()
{
	Log::View::Init();

	window = Window::Create("EDAN35: Assignment 2", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);

	GLStateInspection::Init();
	GLStateInspection::View::Init();

	eda221::init();
}

edan35::Assignment2::~Assignment2()
{
	eda221::deinit();

	GLStateInspection::View::Destroy();
	GLStateInspection::Destroy();

	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edan35::Assignment2::run()
{



	// Load the geometry of Sponza
	auto const sponza_geometry = eda221::loadObjects("../crysponza/sponza.obj");
	if (sponza_geometry.empty()) {
		LogError("Failed to load the Sponza model");
		return;
	}
	std::vector<Node> sponza_elements;
	sponza_elements.reserve(sponza_geometry.size());
	for (auto const& shape : sponza_geometry) {
		Node node;
		node.set_geometry(shape);
		sponza_elements.push_back(node);
	}

	auto const cone_geometry = loadCone();
	Node cone;
	cone.set_geometry(cone_geometry);

	auto const fire_shape = parametric_shapes::createQuad(50, 50);
	auto const smoke_shape = parametric_shapes::createQuad(20, 20);



	auto const window_size = window->GetDimensions();

	//
	// Setup the camera
	//
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(window_size.x) / static_cast<float>(window_size.y),
	                   1.0f, 10000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 100.0f, 180.0f));
	mCamera.mWorld.LookAt(glm::vec3(0.0f, 0.0f, 0.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f;
	window->SetCamera(&mCamera);

	//
	// Load all the shader programs used
	//
	auto fallback_shader = eda221::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	auto fire_shader = eda221::createProgram("fire.vert", "fire.frag");
	if (fire_shader == 0u) {
		LogError("Failed to load fire shader");
		return;
	}
	auto smoke_shader = eda221::createProgram("smoke.vert", "smoke.frag");
	if (smoke_shader == 0u) {
		LogError("Failed to load smoke shader");
		return;
	}
	auto const reload_shader = [fallback_shader](std::string const& vertex_path, std::string const& fragment_path, GLuint& program){
		if (program != 0u && program != fallback_shader)
			glDeleteProgram(program);
		program = eda221::createProgram("../EDAN35/" + vertex_path, "../EDAN35/" + fragment_path);
		if (program == 0u) {
			LogError("Failed to load \"%s\" and \"%s\"", vertex_path.c_str(), fragment_path.c_str());
			program = fallback_shader;
		}
	};
	GLuint fill_gbuffer_shader = 0u, fill_shadowmap_shader = 0u, accumulate_lights_shader = 0u, resolve_deferred_shader = 0u;
	auto const reload_shaders = [&reload_shader,&fill_gbuffer_shader,&fill_shadowmap_shader,&accumulate_lights_shader,&resolve_deferred_shader,&smoke_shader,&fire_shader](){
		LogInfo("Reloading shaders");
		reload_shader("fill_gbuffer.vert",      "fill_gbuffer.frag",      fill_gbuffer_shader);
		reload_shader("fill_shadowmap.vert",    "fill_shadowmap.frag",    fill_shadowmap_shader);
		reload_shader("accumulate_lights.vert", "accumulate_lights.frag", accumulate_lights_shader);
		reload_shader("resolve_deferred.vert",  "resolve_deferred.frag",  resolve_deferred_shader);
		reload_shader("smoke.vert", "smoke.frag", smoke_shader);
		reload_shader("fire.vert", "fire.frag", fire_shader);
	};
	reload_shaders();
	double ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;
	float number = 0;
	auto const set_uniforms = [](GLuint /*program*/){};
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto const set_uniforms_p = [&mCamera,&camera_position, &nowTime, &number, &window_size](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1f(glGetUniformLocation(program, "time"), static_cast<float>(nowTime) / 1000.0f);
		glUniform1f(glGetUniformLocation(program, "number"), number);
		glUniform2f(glGetUniformLocation(program, "inv_res"),
			1.0f / static_cast<float>(window_size.x),
			1.0f / static_cast<float>(window_size.y));
		glUniformMatrix4fv(glGetUniformLocation(program, "view_projection_inverse"), 1, GL_FALSE,
			glm::value_ptr(mCamera.GetClipToWorldMatrix()));
	};

	//
	// Setup textures
	//
	auto const diffuse_texture                     = eda221::createTexture(window_size.x, window_size.y);
	auto const specular_texture                    = eda221::createTexture(window_size.x, window_size.y);
	auto const normal_texture                      = eda221::createTexture(window_size.x, window_size.y);
	auto const light_diffuse_contribution_texture  = eda221::createTexture(window_size.x, window_size.y);
	auto const light_specular_contribution_texture = eda221::createTexture(window_size.x, window_size.y);
	auto const depth_texture                       = eda221::createTexture(window_size.x, window_size.y, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
	auto const shadowmap_texture                   = eda221::createTexture(constant::shadowmap_res_x, constant::shadowmap_res_y, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
	auto fireTex = eda221::loadTexture2D("flame.png");
	auto smokeTex = eda221::loadTexture2D("smoke.png");
	//
	// Setup FBOs
	//
	auto const deferred_fbo  = eda221::createFBO({diffuse_texture, specular_texture, normal_texture}, depth_texture);
	auto const shadowmap_fbo = eda221::createFBO({}, shadowmap_texture);
	auto const light_fbo     = eda221::createFBO({light_diffuse_contribution_texture, light_specular_contribution_texture}, depth_texture);

	//
	// Setup samplers
	//
	auto const default_sampler = eda221::createSampler([](GLuint sampler){
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	});
	auto const depth_sampler = eda221::createSampler([](GLuint sampler){
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	});
	auto const shadow_sampler = eda221::createSampler([](GLuint sampler){
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
		GLfloat border_color[4] = { 1.0f, 0.0f, 0.0f, 0.0f};
		glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border_color);


	});
	auto const bind_texture_with_sampler = [](GLenum target, unsigned int slot, GLuint program, std::string const& name, GLuint texture, GLuint sampler){
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(target, texture);
		glUniform1i(glGetUniformLocation(program, name.c_str()), static_cast<GLint>(slot));
		glBindSampler(slot, sampler);
	};


	//
	// Setup lights properties
	//
	std::array<TRSTransform<float, glm::defaultp>, constant::lights_nb> lightTransforms;
	std::array<glm::vec3, constant::lights_nb> lightColors;

	for (size_t i = 0; i < constant::lights_nb; ++i) {
		lightTransforms[i].SetTranslate(glm::vec3(0.0f, 10.0f, 0.0f));
		lightColors[i] = glm::vec3(0.886,
		                           0.345,
		                           0.133);
	}

	TRSTransform<f32, glm::defaultp> coneScaleTransform = TRSTransform<f32, glm::defaultp>();
	coneScaleTransform.SetScale(glm::vec3(sqrt(constant::light_intensity / constant::light_cutoff)));

	TRSTransform<f32, glm::defaultp> lightOffsetTransform = TRSTransform<f32, glm::defaultp>();
	lightOffsetTransform.SetTranslate(glm::vec3(0.0f, 0.0f, -40.0f));

	auto lightProjection = glm::perspective(bonobo::pi * 0.5f,
	                                        static_cast<float>(constant::shadowmap_res_x) / static_cast<float>(constant::shadowmap_res_y),
	                                        1.0f, 10000.0f);


	auto seconds_nb = 0.0f;


	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	//Fire Node
	glm::vec3 fires[constant::MaxFires];
	Node quad[constant::MaxFires];

	//Initializing positions.
	for (int i = 0; i < constant::MaxFires; i++) {
		quad[i].set_geometry(fire_shape);
		quad[i].add_texture("diffuse_texture", fireTex);
		quad[i].set_program(fire_shader, set_uniforms_p);
	}
	float x_offset = 0.0;//1125.0;
	float y_offset = 10.0;//120.0;
	float z_offset = 0.0;//400.0;
	fires[0] = glm::vec3(-20+x_offset, 0+ y_offset, 0+z_offset);
	quad[0].set_translation(fires[0]);
	fires[1] = glm::vec3(-15 + x_offset, 0+ y_offset, -15+z_offset);
	quad[1].set_translation(fires[1]);
	fires[2] = glm::vec3(15 + x_offset, 0+ y_offset, -15 + z_offset);
	quad[2].set_translation(fires[2]);
	fires[3] = glm::vec3(20 + x_offset, 0+y_offset, 0 + z_offset);
	quad[3].set_translation(fires[3]);
	fires[4] = glm::vec3(15 + x_offset, 0+y_offset, 15 + z_offset);
	quad[4].set_translation(fires[4]);
	fires[5] = glm::vec3(-15 + x_offset, 0+y_offset, 15 + z_offset);
	quad[5].set_translation(fires[5]);
	fires[6] = glm::vec3(-10 + x_offset, 5 + y_offset, -10 + z_offset);
	quad[6].set_translation(fires[6]);
	fires[7] = glm::vec3(10 + x_offset, 5 + y_offset, -10 + z_offset);
	quad[7].set_translation(fires[7]);
	fires[8] = glm::vec3(0 + x_offset, 5 + y_offset, 10 + z_offset);
	quad[8].set_translation(fires[8]);
	fires[9] = glm::vec3(0 + x_offset, 10 + y_offset, 0 + z_offset);
	quad[9].set_translation(fires[9]);

	////SMOKE MY LIFE AWAY
	glm::vec3 smokes[constant::MaxSmokes];
	Node smokeQuad[constant::MaxSmokes];
	for (int i = 0; i < constant::MaxSmokes; i++) {
		smokeQuad[i].set_geometry(smoke_shape);
		smokeQuad[i].add_texture("diffuse_texture", smokeTex);
		smokeQuad[i].set_program(smoke_shader, set_uniforms_p);
		smokes[i] = glm::vec3(0.0 + x_offset, i / 2 + y_offset, 0.0 + z_offset);
		smokeQuad[i].set_translation(smokes[i]);
	}



	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;
		seconds_nb += static_cast<float>(ddeltatime / 1000.0);

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}



		glDepthFunc(GL_LESS);
		//
		// Pass 1: Render scene into the g-buffer
		//
		glBindFramebuffer(GL_FRAMEBUFFER, deferred_fbo);
		GLenum const deferred_draw_buffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, deferred_draw_buffers);
		auto const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			LogError("Something went wrong with framebuffer %u", deferred_fbo);
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		// XXX: Is any other clearing needed?

		GLStateInspection::CaptureSnapshot("Filling Pass");

		for (auto const& element : sponza_elements)
			element.render(mCamera.GetWorldToClipMatrix(), element.get_transform(), fill_gbuffer_shader, set_uniforms);



		glCullFace(GL_FRONT);
		//
		// Pass 2: Generate shadowmaps and accumulate lights' contribution
		//
		glBindFramebuffer(GL_FRAMEBUFFER, light_fbo);
		GLenum light_draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, light_draw_buffers);
		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear( GL_COLOR_BUFFER_BIT);
		for (size_t i = 0; i < constant::lights_nb; ++i) {
			auto& lightTransform = lightTransforms[i];
			lightTransform.SetRotate(1 * 0.1f + i * bonobo::pi/3, glm::vec3(0.0f, 1.0f, 0.0f));

			auto light_matrix = lightProjection * lightOffsetTransform.GetMatrixInverse() * lightTransform.GetMatrixInverse();

			//
			// Pass 2.1: Generate shadow map for light i
			//
			glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_fbo);
			glViewport(0, 0, constant::shadowmap_res_x, constant::shadowmap_res_y);
			// XXX: Is any clearing needed?
			glClearDepthf(1.0f);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			GLStateInspection::CaptureSnapshot("Shadow Map Generation");

			for (auto const& element : sponza_elements)
				element.render(light_matrix, glm::mat4(), fill_gbuffer_shader, set_uniforms);


			glEnable(GL_BLEND);
			glDepthFunc(GL_GREATER);
			glDepthMask(GL_FALSE);
			glBlendEquationSeparate(GL_FUNC_ADD, GL_MIN);
			glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
			//
			// Pass 2.2: Accumulate light i contribution
			glBindFramebuffer(GL_FRAMEBUFFER, light_fbo);
			glDrawBuffers(2, light_draw_buffers);
			glUseProgram(accumulate_lights_shader);
			glViewport(0, 0, window_size.x, window_size.y);
			// XXX: Is any clearing needed?
			//glClearDepthf(1.0f);
			//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			//glClear(GL_COLOR_BUFFER_BIT);
			auto const spotlight_set_uniforms = [&window_size,&mCamera,&light_matrix,&lightColors,&lightTransform,&i,&nowTime](GLuint program){
				glUniform2f(glGetUniformLocation(program, "inv_res"),
				            1.0f / static_cast<float>(window_size.x),
				            1.0f / static_cast<float>(window_size.y));
				glUniformMatrix4fv(glGetUniformLocation(program, "view_projection_inverse"), 1, GL_FALSE,
				                   glm::value_ptr(mCamera.GetClipToWorldMatrix()));
				glUniform3fv(glGetUniformLocation(program, "camera_position"), 1,
				                   glm::value_ptr(mCamera.mWorld.GetTranslation()));
				glUniformMatrix4fv(glGetUniformLocation(program, "shadow_view_projection"), 1, GL_FALSE,
				                   glm::value_ptr(light_matrix));
				glUniform1f(glGetUniformLocation(program, "time"), static_cast<float>(nowTime) / 1000.0f );
				glUniform3fv(glGetUniformLocation(program, "light_color"), 1, glm::value_ptr(lightColors[i]));
				glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(lightTransform.GetTranslation()));
				glUniform3fv(glGetUniformLocation(program, "light_direction"), 1, glm::value_ptr(lightTransform.GetFront()));
				glUniform1f(glGetUniformLocation(program, "light_intensity"), constant::light_intensity);
				glUniform1f(glGetUniformLocation(program, "light_angle_falloff"), constant::light_angle_falloff);
				glUniform2f(glGetUniformLocation(program, "shadowmap_texel_size"),
				            1.0f / static_cast<float>(constant::shadowmap_res_x),
				            1.0f / static_cast<float>(constant::shadowmap_res_y));
			};

			bind_texture_with_sampler(GL_TEXTURE_2D, 0, accumulate_lights_shader, "depth_texture", depth_texture, depth_sampler);
			bind_texture_with_sampler(GL_TEXTURE_2D, 1, accumulate_lights_shader, "normal_texture", normal_texture, default_sampler);
			bind_texture_with_sampler(GL_TEXTURE_2D, 2, accumulate_lights_shader, "shadow_texture", shadowmap_texture, shadow_sampler);

			GLStateInspection::CaptureSnapshot("Accumulating");

			cone.render(mCamera.GetWorldToClipMatrix(),
			            lightTransform.GetMatrix() * lightOffsetTransform.GetMatrix() * coneScaleTransform.GetMatrix(),
			            accumulate_lights_shader, spotlight_set_uniforms);

			glBindSampler(2u, 0u);
			glBindSampler(1u, 0u);
			glBindSampler(0u, 0u);

			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
			glDisable(GL_BLEND);
		}


		glCullFace(GL_BACK);
		glDepthFunc(GL_ALWAYS);
		//
		// Pass 3: Compute final image using both the g-buffer and  the light accumulation buffer
		//
		glBindFramebuffer(GL_FRAMEBUFFER, 0u);
		glUseProgram(resolve_deferred_shader);
		glViewport(0, 0, window_size.x, window_size.y);
		// XXX: Is any clearing needed?
		//glClearDepthf(1.0f);
		//glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
		//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		bind_texture_with_sampler(GL_TEXTURE_2D, 0, resolve_deferred_shader, "diffuse_texture", diffuse_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 1, resolve_deferred_shader, "specular_texture", specular_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 2, resolve_deferred_shader, "light_d_texture", light_diffuse_contribution_texture, default_sampler);
		bind_texture_with_sampler(GL_TEXTURE_2D, 3, resolve_deferred_shader, "light_s_texture", light_specular_contribution_texture, default_sampler);

		GLStateInspection::CaptureSnapshot("Resolve Pass");

		eda221::drawFullscreen();

		glBindSampler(3, 0u);
		glBindSampler(2, 0u);
		glBindSampler(1, 0u);
		glBindSampler(0, 0u);
		glUseProgram(0u);



		//
		// Pass 4: Draw wireframe cones on top of the final image for debugging purposes
		//
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//for (size_t i = 0; i < constant::lights_nb; ++i) {
		//	cone.render(mCamera.GetWorldToClipMatrix(),
		//	            lightTransforms[i].GetMatrix() * lightOffsetTransform.GetMatrix() * coneScaleTransform.GetMatrix(),
		//	            fill_shadowmap_shader, set_uniforms);
		//}
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Pass 5: Draw transparent objects
		//
		
		glDepthFunc(GL_ALWAYS);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_WRITEMASK);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		camera_position = mCamera.mWorld.GetTranslation();
		for (int i = 0; i < constant::MaxSmokes / 3; i++) {
			smokes[i].x = x_offset+10*sin((1000 * static_cast<float>(i) + nowTime) / 1500) + 10*pow(sin((1000 * static_cast<float>(i) + nowTime) / 1900), 2);
			smokes[i].y = y_offset + fmod((10000 * static_cast<float>(i) + 10*nowTime) / 1000, 100);
			smokes[i].z = z_offset + 10*sin((1000 * static_cast<float>(i) + nowTime) / 2000) + 10*pow(sin(nowTime / 1000), 2);
		}
		for (int i = constant::MaxSmokes / 3; i < 2 * constant::MaxSmokes / 3; i++) {
			smokes[i].x = x_offset+ -10 - 10*sin((1000 * static_cast<float>(i) + nowTime) / 1750) + 10*pow(sin((1000 * static_cast<float>(i - 10) + nowTime) / 1700), 2);
			smokes[i].y = y_offset + 10 + fmod((10000 * static_cast<float>(i) + 10*nowTime) / 1000, 100);
			smokes[i].z = z_offset + 10*sin((1000 * static_cast<float>(i) + nowTime) / 1337) + 10*pow(sin((1000 * static_cast<float>(i) + nowTime) / 1200), 3);
		}
		for (int i = 2 * constant::MaxSmokes / 3; i < constant::MaxSmokes; i++) {
			smokes[i].x =x_offset + -15 + sin((1000 * static_cast<float>(i) + nowTime) / 1337) + pow(sin((1000 * static_cast<float>(i - 20) + nowTime) / 1200), 3);
			smokes[i].y =y_offset +  20 + fmod((10000 * static_cast<float>(i) + 10*nowTime) / 1000, 100);
			smokes[i].z =z_offset +  -10 - 10*sin((1000 * static_cast<float>(i) + nowTime) / 1337) - 10*pow(sin((1000 * static_cast<float>(i - 20) + nowTime) / 2000), 5);
		}
		

		
		for (int i = 0; i < constant::MaxFires; i++) {
			bind_texture_with_sampler(GL_TEXTURE_2D, 1, fire_shader, "depth_texture", depth_texture, depth_sampler);
			glm::vec3 q_position = fires[i];
			quad[i].render(mCamera.GetWorldToClipMatrix(), quad[i].get_transform(), mCamera.GetWorldToViewMatrix(), mCamera.GetViewToClipMatrix(), q_position);
		}
		bind_texture_with_sampler(GL_TEXTURE_2D, 0, smoke_shader, "depth_texture", depth_texture, depth_sampler);
		for (int i = 0; i < constant::MaxSmokes; i++) {
			smokeQuad[i].set_translation(smokes[i]);
			number = fmod((1000 * static_cast<float>(i) + nowTime) / 1000, 10);
			glm::vec3 q_position = smokes[i];
			smokeQuad[i].render(mCamera.GetWorldToClipMatrix(), smokeQuad[i].get_transform(), mCamera.GetWorldToViewMatrix(), mCamera.GetViewToClipMatrix(), q_position);
		}
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDepthFunc(GL_ALWAYS);
		glEnable(GL_DEPTH_WRITEMASK);
		//
		// Output content of the g-buffer as well as of the shadowmap, for debugging purposes
		//
		eda221::displayTexture({-0.95f, -0.95f}, {-0.55f, -0.55f}, diffuse_texture,                     default_sampler, {0, 1, 2, -1}, window_size);
		eda221::displayTexture({-0.45f, -0.95f}, {-0.05f, -0.55f}, specular_texture,                    default_sampler, {0, 1, 2, -1}, window_size);
		eda221::displayTexture({ 0.05f, -0.95f}, { 0.45f, -0.55f}, normal_texture,                      default_sampler, {0, 1, 2, -1}, window_size);
		eda221::displayTexture({ 0.55f, -0.95f}, { 0.95f, -0.55f}, depth_texture,                       default_sampler, {0, 0, 0, -1}, window_size, &mCamera);
		eda221::displayTexture({-0.95f,  0.55f}, {-0.55f,  0.95f}, shadowmap_texture,                   default_sampler, {0, 0, 0, -1}, window_size, &mCamera);
		eda221::displayTexture({-0.45f,  0.55f}, {-0.05f,  0.95f}, light_diffuse_contribution_texture,  default_sampler, {0, 1, 2, -1}, window_size);
		eda221::displayTexture({ 0.05f,  0.55f}, { 0.45f,  0.95f}, light_specular_contribution_texture, default_sampler, {0, 1, 2, -1}, window_size);
		//
		// Reset viewport back to normal
		//
		glViewport(0, 0, window_size.x, window_size.y);

		GLStateInspection::View::Render();
		Log::View::Render();

		bool opened = ImGui::Begin("Render Time", nullptr, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms", ddeltatime);
		ImGui::End();

		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(resolve_deferred_shader);
	resolve_deferred_shader = 0u;
	glDeleteProgram(accumulate_lights_shader);
	accumulate_lights_shader = 0u;
	glDeleteProgram(fill_shadowmap_shader);
	fill_shadowmap_shader = 0u;
	glDeleteProgram(fill_gbuffer_shader);
	fill_gbuffer_shader = 0u;
	glDeleteProgram(fallback_shader);
	fallback_shader = 0u;
	glDeleteProgram(fire_shader);
	fire_shader = 0u;
	glDeleteProgram(smoke_shader);
	smoke_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edan35::Assignment2 assignment2;
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}

static
eda221::mesh_data
loadCone()
{
	eda221::mesh_data cone;
	cone.vertices_nb = 65;
	cone.drawing_mode = GL_TRIANGLE_STRIP;
	float vertexArrayData[65 * 3] = {
		0.f, 1.f, -1.f,
		0.f, 0.f, 0.f,
		0.38268f, 0.92388f, -1.f,
		0.f, 0.f, 0.f,
		0.70711f, 0.70711f, -1.f,
		0.f, 0.f, 0.f,
		0.92388f, 0.38268f, -1.f,
		0.f, 0.f, 0.f,
		1.f, 0.f, -1.f,
		0.f, 0.f, 0.f,
		0.92388f, -0.38268f, -1.f,
		0.f, 0.f, 0.f,
		0.70711f, -0.70711f, -1.f,
		0.f, 0.f, 0.f,
		0.38268f, -0.92388f, -1.f,
		0.f, 0.f, 0.f,
		0.f, -1.f, -1.f,
		0.f, 0.f, 0.f,
		-0.38268f, -0.92388f, -1.f,
		0.f, 0.f, 0.f,
		-0.70711f, -0.70711f, -1.f,
		0.f, 0.f, 0.f,
		-0.92388f, -0.38268f, -1.f,
		0.f, 0.f, 0.f,
		-1.f, 0.f, -1.f,
		0.f, 0.f, 0.f,
		-0.92388f, 0.38268f, -1.f,
		0.f, 0.f, 0.f,
		-0.70711f, 0.70711f, -1.f,
		0.f, 0.f, 0.f,
		-0.38268f, 0.92388f, -1.f,
		0.f, 1.f, -1.f,
		0.f, 1.f, -1.f,
		0.38268f, 0.92388f, -1.f,
		0.f, 1.f, -1.f,
		0.70711f, 0.70711f, -1.f,
		0.f, 0.f, -1.f,
		0.92388f, 0.38268f, -1.f,
		0.f, 0.f, -1.f,
		1.f, 0.f, -1.f,
		0.f, 0.f, -1.f,
		0.92388f, -0.38268f, -1.f,
		0.f, 0.f, -1.f,
		0.70711f, -0.70711f, -1.f,
		0.f, 0.f, -1.f,
		0.38268f, -0.92388f, -1.f,
		0.f, 0.f, -1.f,
		0.f, -1.f, -1.f,
		0.f, 0.f, -1.f,
		-0.38268f, -0.92388f, -1.f,
		0.f, 0.f, -1.f,
		-0.70711f, -0.70711f, -1.f,
		0.f, 0.f, -1.f,
		-0.92388f, -0.38268f, -1.f,
		0.f, 0.f, -1.f,
		-1.f, 0.f, -1.f,
		0.f, 0.f, -1.f,
		-0.92388f, 0.38268f, -1.f,
		0.f, 0.f, -1.f,
		-0.70711f, 0.70711f, -1.f,
		0.f, 0.f, -1.f,
		-0.38268f, 0.92388f, -1.f,
		0.f, 0.f, -1.f,
		0.f, 1.f, -1.f,
		0.f, 0.f, -1.f
	};

	glGenVertexArrays(1, &cone.vao);
	assert(cone.vao != 0u);
	glBindVertexArray(cone.vao);
	{
		glGenBuffers(1, &cone.bo);
		assert(cone.bo != 0u);
		glBindBuffer(GL_ARRAY_BUFFER, cone.bo);
		glBufferData(GL_ARRAY_BUFFER, cone.vertices_nb * 3 * sizeof(float), vertexArrayData, GL_STATIC_DRAW);

		glVertexAttribPointer(static_cast<int>(eda221::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));
		glEnableVertexAttribArray(static_cast<int>(eda221::shader_bindings::vertices));

		glBindBuffer(GL_ARRAY_BUFFER, 0u);
	}
	glBindVertexArray(0u);

	return cone;
}
