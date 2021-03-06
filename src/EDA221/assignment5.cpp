#include "assignment5.hpp"
#include "interpolation.hpp"
#include "node.hpp"
#include "parametric_shapes.hpp"


#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>
#include <map>
#include <stddef.h>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};





static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}
static glm::vec3 startPosition() {
	float x = pow(rand() % 600 - 300, 2)/300;
	return glm::vec3(x, (rand() % 500)/250, 9 - x);
}
bool testSphereSphere(glm::vec3 const&p1, float const r1, glm::vec3 const&p2, float const r2) {
	return glm::length(p1 - p2) < (r1 + r2);
}

eda221::Assignment5::Assignment5()
{
	Log::View::Init();

	window = Window::Create("EDA221: Assignment 5", config::resolution_x,
		config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}


eda221::Assignment5::~Assignment5()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}
void eda221::Assignment5::run()
{
	// Loading the level geometry
	auto const quad_shape = parametric_shapes::createQuad(100, 100);
	
	

	// Set up the camera 
	// Get the camera to follow the snake.
	// Maybe start by having the camera looking down on the map to see if it works,
	// We can add two camera modes perhaps.
	FPSCameraf mCamera(bonobo::pi / 4.0f,
		static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
		0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	// Water shader for the level
	auto diffuse_shader = eda221::createProgram("diffuse.vert", "diffuse.frag");
	if (diffuse_shader == 0u) {
		LogError("Failed to load water shader");
		return;
	}
	auto const reload_shaders = [&diffuse_shader]() {
		if (diffuse_shader != 0u)
			glDeleteProgram(diffuse_shader);
		diffuse_shader = eda221::createProgram("diffuse.vert", "diffuse.frag");
		if (diffuse_shader == 0u)
			LogError("Failed to load diffuse shader");
	};
	reload_shaders();

	// Initializing the time variables.
	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;


	// Set the uniforms of the shaders
	auto light_position = glm::vec3(-90.0f, 100.0f, -90.0f); // Light position;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto const set_uniforms = [&light_position, &camera_position, &nowTime](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1f(glGetUniformLocation(program, "time"), static_cast<float>(nowTime) / 1000.0f);



	};

	auto polygon_mode = polygon_mode_t::fill;

	auto fireTex = loadTexture2D("flame.png");

	//Fire Node
	const int MaxFires = 1;
	glm::vec3 fires[MaxFires];
	Node quad[MaxFires];
	for (int i=0; i < MaxFires; i++) {
		quad[i].set_geometry(quad_shape);
		quad[i].add_texture("diffuse_texture", fireTex);
		quad[i].set_program(diffuse_shader, set_uniforms);
		//quad[i].scale(glm::vec3(0.1, 0.1, 0.1));
		quad[i].rotate_x(bonobo::pi / 2);
		fires[i] = startPosition();
		quad[i].set_translation(fires[i]);
	}

	glEnable(GL_DEPTH_TEST);
	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	//Initializing positions.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);





		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		switch (polygon_mode) {
		case polygon_mode_t::fill:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case polygon_mode_t::line:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case polygon_mode_t::point:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}

		camera_position = mCamera.mWorld.GetTranslation();



		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		//sort here
		std::map<float, glm::vec3> sorted;
		for (GLuint i = 0; i < MaxFires; i++) // windows contains all window positions
		{
			GLfloat distance = glm::length(camera_position - fires[i]);
			sorted[distance] = fires[i];
		}




		//render here
		int i = 0;
		for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
		{	
			




			glm::vec3 position = it->second;
			quad[i].set_translation(position);

			auto normal_model_to_world = glm::transpose(glm::inverse(quad[i].get_transform()));

			glm::vec3 up = glm::vec3(normal_model_to_world * glm::vec4(1.0,0.0,0.0,0.0));
			glm::mat4 rotate = glm::lookAt(position,camera_position,up);
			for (int j = 0; j < 4; j++) {
				for (int k = 0; k < 4; k++) {
					std::printf("%f ",rotate[j][k]);
				}
				std::printf("\n");
			}
				
				
			quad[i].render(mCamera.GetWorldToClipMatrix(),rotate,position);
			i++;
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			//ImGui::SliderFloat("Speed", &speed, 0.0f, 50.0f);
			//			ImGui::SliderInt("Faces Nb", &faces_nb, 1u, 16u);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text(" %.0f fps", 1000 / (ddeltatime));
		ImGui::End();

		ImGui::Render();
		window->Swap();
		lastTime = nowTime;

	}
	glDeleteProgram(diffuse_shader);
	diffuse_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		eda221::Assignment5 assignment5;
		assignment5.run();
	}
	catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}

