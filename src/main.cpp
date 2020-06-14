#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "world.h"
#include "shader.h"
#include "model.h"
#include "window.h"

#include <iostream>

int main() try {
	
	//The main window
	auto window = sstm::MainWindow{};
  
	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	// stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	
	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// timing
	auto deltaTime = 0.f;
	auto lastFrame = 0.f;

	auto frame_count = size_t{};


	// render loop
	// -----------
	while (!window.wants_to_close())
	{
		// per-frame time logic
		// --------------------
		auto currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		++frame_count;
		if (!(frame_count % 1'000)) {
			std::cout << "FPS: " << 1.f / deltaTime << '\n';
		}
	 

		// input
		glfwPollEvents();
		window.process_keyboard_input(deltaTime);

		//TODO: No.
		window.render(deltaTime);
	} 
} catch (std::exception &e) {
	std::cerr << "Could not recover from exception: \"" << e.what() << "\" Exiting." << std::endl;
	exit(1);
}