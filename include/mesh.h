#pragma once

#include "glad/glad.h" // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gl_helper.h"

#include "shader.h"

#include <string>
#include <vector>

namespace sstm {

	struct Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;

		//clang doesn't support emplace back for aggregates yet.
		Vertex(
			glm::vec3 _Position,
			glm::vec3 _Normal,
			glm::vec2 _TexCoords,
			glm::vec3 _Tangent,
			glm::vec3 _Bitangent
		 ) :
			Position{_Position},
			Normal{_Normal},
			TexCoords{_TexCoords},
			Tangent{_Tangent},
			Bitangent{_Bitangent}
		{}
	};

	struct Texture {
		unsigned int id;
		std::string type;
		std::string path;
	};

	class Mesh {
	public:
		// mesh Data
		std::vector<Vertex>       vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture>      textures;
		unsigned int VAO;

		// constructor
		Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, std::vector<Texture> _textures) :
			vertices{std::move(_vertices)},
			indices{std::move(_indices)},
			textures{std::move(_textures)},
			VAO{}, VBO{}, EBO{} //TODO
		{
			// now that we have all the required data, set the vertex buffers and its attribute pointers.
			setupMesh();
		}

		// render the mesh
		void Draw(Shader shader) const {
			// bind appropriate textures
			unsigned int diffuseNr = 1;
			unsigned int specularNr = 1;
			unsigned int normalNr = 1;
			unsigned int heightNr = 1;
			for (unsigned int i = 0; i < textures.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
				// retrieve texture number (the N in diffuse_textureN)
				std::string number;
				std::string name = textures[i].type;
				if (name == "texture_diffuse")
					number = std::to_string(diffuseNr++);
				else if (name == "texture_specular")
					number = std::to_string(specularNr++); // transfer unsigned int to stream
				else if (name == "texture_normal")
					number = std::to_string(normalNr++); // transfer unsigned int to stream
				else if (name == "texture_height")
					number = std::to_string(heightNr++); // transfer unsigned int to stream

				// now set the sampler to the correct texture unit
				glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), static_cast<GLint>(i));
				// and finally bind the texture
				glBindTexture(GL_TEXTURE_2D, textures[i].id);
			}

			// draw mesh
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, to_gl_offset(0));
			glBindVertexArray(0);

			// always good practice to set everything back to defaults once configured.
			//TODO: Wieso anders als bei Text?
			glActiveTexture(GL_TEXTURE0);
		}

	private:
		// render data 
		unsigned int VBO, EBO;

		// initializes all the buffer objects/arrays
		void setupMesh()
		{
			// create buffers/arrays
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);
			// load data into vertex buffers
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			// A great thing about structs is that their memory layout is sequential for all its items.
			// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
			// again translates to 3/2 floats which translates to a byte array.
			glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), &indices[0], GL_STATIC_DRAW);

			// set the vertex attribute pointers
			// vertex Positions
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), to_gl_offset(0));
			// vertex normals
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), to_gl_offset(offsetof(Vertex, Normal)));
			// vertex texture coords
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), to_gl_offset(offsetof(Vertex, TexCoords)));
			// vertex tangent
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), to_gl_offset(offsetof(Vertex, Tangent)));
			// vertex bitangent
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), to_gl_offset(offsetof(Vertex, Bitangent)));

			glBindVertexArray(0);
		}
	};
} //namespace sstm