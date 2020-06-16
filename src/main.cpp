#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "world.h"
#include "shader.h"
#include "model.h"
#include "window.h"

#include <iostream>

int main() try {
	
	//TODO: name duplication
	stdc::fs::create_directory("saves");

	//The main window
	auto window = sstm::MainWindow{};
  
	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	// stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
		
	// glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// timing
	auto deltaTime = 0.f;
	auto lastFrame = 0.f;

	auto frame_count = size_t{};


	//Fonts.
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

	FT_Face face;
	if (FT_New_Face(ft, "fonts/DejaVuSerif.ttf", 0, &face))
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl; 

	FT_Set_Pixel_Sizes(face, 0, 48);  
	
	if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
    	std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl; 


	//TODO!!!!!!!!!!!!!!
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction 
  
	for (unsigned char c = 0; c < 128; c++) {
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// now store character for later use
		Character character = {
			texture, 
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<char, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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