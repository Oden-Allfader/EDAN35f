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
	float x = (rand() % 12 - 6) / 3;
	float z = (rand() % 12 - 6) / 3;
	float h = 4;
	float y = (rand() % 12 - 6) / 3;
	return glm::vec3(x, y, z);
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
	const int MaxFires = 10;
	const int MaxSmokes = 30;

	// Loading the level geometry
	auto const fire_shape = parametric_shapes::createQuad(6, 6);
	auto const smoke_shape = parametric_shapes::createQuad(2, 2);

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





	auto const reload_shaders = [&fire_shader]() {
		if (fire_shader != 0u)
			glDeleteProgram(fire_shader);
		fire_shader = eda221::createProgram("fire.vert", "fire.frag");
		if (fire_shader == 0u)
			LogError("Failed to load diffuse shader");
	};
	reload_shaders();



	// Initializing the time variables.
	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;
	float number = 0;

	// Set the uniforms of the shaders
	auto light_position = glm::vec3(-90.0f, 100.0f, -90.0f); // Light position;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto const set_uniforms = [&light_position, &camera_position, &nowTime, &number](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1f(glGetUniformLocation(program, "time"), static_cast<float>(nowTime) / 1000.0f);
		glUniform1f(glGetUniformLocation(program, "number"), number);
	};

	auto polygon_mode = polygon_mode_t::fill;

	auto fireTex = loadTexture2D("flame.png");



	//Fire Node
	glm::vec3 fires[MaxFires];
	Node quad[MaxFires];

	//Initializing positions.
	for (int i = 0; i < MaxFires; i++) {
		quad[i].set_geometry(fire_shape);
		quad[i].add_texture("diffuse_texture", fireTex);
		quad[i].set_program(fire_shader, set_uniforms);
	}
	fires[0] = glm::vec3(-2, 0, 0);
	quad[0].set_translation(fires[0]);
	fires[1] = glm::vec3(-1.5, 0, -1.5);
	quad[1].set_translation(fires[1]);
	fires[2] = glm::vec3(1.5, 0, -1.5);
	quad[2].set_translation(fires[2]);
	fires[3] = glm::vec3(2, 0, 0);
	quad[3].set_translation(fires[3]);
	fires[4] = glm::vec3(1.5, 0, 1.5);
	quad[4].set_translation(fires[4]);
	fires[5] = glm::vec3(-1.5, 0, 1.5);
	quad[5].set_translation(fires[5]);
	fires[6] = glm::vec3(-1, 0.5, -1);
	quad[6].set_translation(fires[6]);
	fires[7] = glm::vec3(1, 0.5, -1);
	quad[7].set_translation(fires[7]);
	fires[8] = glm::vec3(0, 0.5, 1);
	quad[8].set_translation(fires[8]);
	fires[9] = glm::vec3(0, 1, 0);
	quad[9].set_translation(fires[9]);

	////SMOKE MY LIFE AWAY
	auto smokeTex = loadTexture2D("smoke.png");
	glm::vec3 smokes[MaxSmokes];
	Node smokeQuad[MaxSmokes];
	for (int i = 0; i < MaxSmokes; i++) {
		smokeQuad[i].set_geometry(smoke_shape);
		smokeQuad[i].add_texture("diffuse_texture", smokeTex);
		smokeQuad[i].set_program(smoke_shader, set_uniforms);
		smokes[i] = glm::vec3(0.0, i / 2, 0.0);
		smokeQuad[i].set_translation(smokes[i]);
	}


	////SMOKE ENDS


	glEnable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);
	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


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

		for (int i = 0; i < MaxSmokes / 3; i++) {
			smokes[i].x = sin((1000 * static_cast<float>(i) + nowTime) / 1500) + pow(sin((1000 * static_cast<float>(i)+nowTime) / 1900), 2);
			smokes[i].y = fmod((1000 * static_cast<float>(i) + nowTime) / 1000, 10);
			smokes[i].z = sin((1000 * static_cast<float>(i) + nowTime) / 2000) + pow(sin(nowTime / 1000), 2);
		}
		for (int i = MaxSmokes / 3; i < 2 * MaxSmokes / 3; i++) {
			smokes[i].x = -1.0 - sin((1000 * static_cast<float>(i) + nowTime) / 1750) + pow(sin((1000 * static_cast<float>(i-10) + nowTime) / 1700), 2);
			smokes[i].y = 1.0 + fmod((1000 * static_cast<float>(i) + nowTime) / 1000, 10);
			smokes[i].z = sin((1000 * static_cast<float>(i) + nowTime) / 1337) + pow(sin((1000 * static_cast<float>(i) + nowTime) / 1200), 3);
		}
		for (int i = 2 * MaxSmokes / 3; i < MaxSmokes; i++) {
			smokes[i].x = -1.5 + sin((1000 * static_cast<float>(i) + nowTime) / 1337) + pow(sin((1000 * static_cast<float>(i-20) + nowTime) / 1200), 3);
			smokes[i].y = 2.0 + fmod((1000 * static_cast<float>(i) + nowTime) / 1000, 10);
			smokes[i].z = -1 - sin((1000 * static_cast<float>(i) + nowTime) / 1337) - pow(sin((1000 * static_cast<float>(i-20) + nowTime) / 2000), 5);
		}
		//sort here
		//std::map<float, glm::vec3> sorted;
		//for (GLuint i = 0; i < MaxFires; i++) // windows contains all window positions
		//{
		//	GLfloat distance = glm::distance(camera_position, fires[i]);
		//	sorted[distance] = fires[i];
		//}

		//std::map<float, glm::vec3> sortedSmoke;
		//for (GLuint i = 0; i < MaxSmokes; i++) // windows contains all window positions
		//{
		//	GLfloat distance = glm::distance(camera_position, smokes[i]);
		//	sortedSmoke[distance] = smokes[i];
		//}


		//render here
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (int i = 0; i < MaxFires; i++) {
			glm::vec3 q_position = glm::vec3(0, 0, 0);
			quad[i].render(mCamera.GetWorldToClipMatrix(), quad[i].get_transform(), mCamera.GetWorldToViewMatrix(), mCamera.GetViewToClipMatrix(), q_position);
		}

		for (int i = 0; i < MaxSmokes; i++) {
			smokeQuad[i].set_translation(smokes[i]);
			number = fmod((1000 * static_cast<float>(i) + nowTime) / 1000, 10);
			glm::vec3 q_position = glm::vec3(0, 0, 0);
			smokeQuad[i].render(mCamera.GetWorldToClipMatrix(), smokeQuad[i].get_transform(), mCamera.GetWorldToViewMatrix(), mCamera.GetViewToClipMatrix(), q_position);
		}

		//int i = 0;
		//for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
		//{


		//	glm::vec3 q_position = it->second;
		//	quad[i].set_translation(q_position);


		//	quad[i].render(mCamera.GetWorldToClipMatrix(), quad[i].get_transform(), mCamera.GetWorldToViewMatrix(), mCamera.GetViewToClipMatrix(), q_position);
		//	i++;
		//}
		//int i = 0;

		//for (std::map<float, glm::vec3>::reverse_iterator it = sortedSmoke.rbegin(); it != sortedSmoke.rend(); ++it)
		//{


		//	glm::vec3 q_position = it->second;
		//	smokeQuad[i].set_translation(q_position);
		//	
		//	number = fmod((1000*static_cast<float>(i) + nowTime) / 1000, 10);
		//	smokeQuad[i].render(mCamera.GetWorldToClipMatrix(), smokeQuad[i].get_transform(), mCamera.GetWorldToViewMatrix(), mCamera.GetViewToClipMatrix(), q_position);
		//	i++;
		//}



		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			//ImGui::SliderFloat("Speed", &speed, 0.0f, 50.0f);
			//			ImGui::SliderInt("Faces Nb", &faces_nb, 1u, 16u);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text(" %.0f fps,  Camera Postion: %f, %f, %f \t Quad1 Position: %f, %f, %f", 1000 / (ddeltatime), camera_position.x, camera_position.y, camera_position.z, fires[0].x, fires[0].y, fires[0].z);
		ImGui::End();

		ImGui::Render();
		window->Swap();
		lastTime = nowTime;

	}
	glDeleteProgram(fire_shader);
	fire_shader = 0u;
	glDeleteProgram(smoke_shader);
	smoke_shader = 0u;
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

