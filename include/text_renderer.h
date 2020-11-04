#pragma once

#include <vector>
#include <array>
#include <iostream>

#include <glm/vec2.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "glad/glad.h"

namespace sstm {
	
	class TextRenderer {
	private:
		struct CharacterRenderInfo {
			GLuint texture_id;  // ID handle of the glyph texture
			glm::vec2 size; // Size of glyph
			glm::vec2 bearing; // Offset from baseline to left/top of glyph
			float advance; // Offset to advance to next glyph
		};

		std::vector<CharacterRenderInfo> character_render_infos{};
		GLuint VAO{};
		GLuint VBO{};
	
	public:

		explicit TextRenderer(int) {};

		TextRenderer() {
			
			//Fonts.
			auto *ft = FT_Library{};
			if (FT_Init_FreeType(&ft))
				std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

			auto *face = FT_Face{};
			if (FT_New_Face(ft, "fonts/DejaVuSerif.ttf", 0, &face))
				std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl; 

			FT_Set_Pixel_Sizes(face, 0, 32);  
			
			if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
				std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl; 


			auto original_alignment = GLint{};
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &original_alignment);
			assert(original_alignment == 4);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction 
		
			for (unsigned char c = 0; c < 128; c++) {
				// load character glyph 
				if (FT_Load_Char(face, c, FT_LOAD_RENDER))
				{
					std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
					continue;
				}
				// generate texture
				auto texture = GLuint{};
				glGenTextures(1, &texture);
				glBindTexture(GL_TEXTURE_2D, texture);
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_RED,
					static_cast<GLsizei>(face->glyph->bitmap.width),
					static_cast<GLsizei>(face->glyph->bitmap.rows),
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
				
				//TODO: emplace_back as soon as tidy supports this for POD in C++20.
				character_render_infos.push_back(CharacterRenderInfo{
					texture, 
					glm::vec2{face->glyph->bitmap.width, face->glyph->bitmap.rows},
					glm::vec2{face->glyph->bitmap_left, face->glyph->bitmap_top},
					static_cast<float>(face->glyph->advance.x)
				});
			}
			glBindTexture(GL_TEXTURE_2D, 0);
			glPixelStorei(GL_UNPACK_ALIGNMENT, original_alignment);
			
			FT_Done_Face(face);
			FT_Done_FreeType(ft);

			// configure VAO/VBO for texture quads
			// -----------------------------------
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		// render a line of text
		void render_text(Shader &shader, std::string_view text, float x, float y, float scale, glm::vec3 color) const {
			// activate corresponding render state	
			shader.use();
			
			shader.setVec3("textColor", color);
			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(VAO);

			//iterate through all characters
			for (auto c : text) {

				if (c < 0) {
					c = '*';
				}

				assert(static_cast<size_t>(c) < character_render_infos.size());
				auto ch = character_render_infos[static_cast<size_t>(c)];

				float xpos = x + ch.bearing.x * scale;
				float ypos = y - (ch.size.y - ch.bearing.y) * scale;

				float w = ch.size.x * scale;
				float h = ch.size.y * scale;
				// update VBO for each character
				auto vertices = std::array{
					xpos,     ypos + h,   0.0f, 0.0f,            
					xpos,     ypos,       0.0f, 1.0f,
					xpos + w, ypos,       1.0f, 1.0f,

					xpos,     ypos + h,   0.0f, 0.0f,
					xpos + w, ypos,       1.0f, 1.0f,
					xpos + w, ypos + h,   1.0f, 0.0f           
				};
				// render glyph texture over quad
				glBindTexture(GL_TEXTURE_2D, ch.texture_id);
				// update content of VBO memory
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data()); // be sure to use glBufferSubData and not glBufferData

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				// render quad
				glDrawArrays(GL_TRIANGLES, 0, 6);
				// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
				x += (ch.advance / 64) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
			}
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

	};
} //namespace sstm