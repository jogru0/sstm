#pragma once

#include "glad/glad.h" 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <cool/utility.h>
#include <cool/filesystem.h>

#include <span>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

namespace sstm {

	inline unsigned int TextureFromFile(std::string_view filename, const stdc::fs::path &directory) {
		auto file = directory / filename;

		auto textureID = GLuint{};
		glGenTextures(1, &textureID);

		auto width = int{};
		auto height = int{};
		auto nrComponents = int{};
		
		WATCH("stbi_load").reset();
		WATCH("stbi_load").start();
		auto *data = stbi_load(file.c_str(), &width, &height, &nrComponents, 0);
		WATCH("stbi_load").stop();
			
		std::cout << "stbi_load(" << file << ", ...): " << WATCH("stbi_load").elapsed<std::chrono::milliseconds>() << " ms\n";		

		if (data)
		{
			auto format = [&]() {
				switch (nrComponents) {
					case 1: return GL_RED;
					case 3: return GL_RGB;
					case 4: return GL_RGBA;
					default: assert(false); return -1;
				}
			}();

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, static_cast<GLenum>(format), GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			std::cout << "Texture failed to load at path: " << file << std::endl;
		}

		stbi_image_free(data);
		return textureID;
	}

	struct AABB {
		glm::vec3 min = glm::vec3{
			std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::infinity()
		};

		glm::vec3 max = glm::vec3{
			-std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity()
		};
	};

	void inline update_aabb(AABB &aabb, const glm::vec3 &vec) {
		if (vec.x < aabb.min.x) {
			aabb.min.x = vec.x;
		};
		if (vec.x > aabb.max.x) {
			aabb.max.x = vec.x;
		};
		if (vec.y < aabb.min.y) {
			aabb.min.y = vec.y;
		};
		if (vec.y > aabb.max.y) {
			aabb.max.y = vec.y;
		};
		if (vec.z < aabb.min.z) {
			aabb.min.z = vec.z;
		};
		if (vec.z > aabb.max.z) {
			aabb.max.z = vec.z;
		};

	}


	[[nodiscard]] constexpr auto to_vec3(const aiVector3D &vert) {
		return glm::vec3{
			vert.x,
			vert.y,
			vert.z
		};
	}

	[[nodiscard]] constexpr auto to_vec2(const aiVector3D &vert) {
		return glm::vec2{
			vert.x,
			vert.y,
		};
	}

	class Model {
	public:
		// model data 
		std::vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
		std::vector<Mesh> meshes;
		stdc::fs::path directory;
		AABB aabb;

		// constructor, expects a filepath to a 3D model.
		explicit Model(const std::filesystem::path &path) {
			loadModel(path);
			std::cout << "Number of Meshes: " << meshes.size() << std::endl;
		}

		// draws the model, and thus all its meshes
		void Draw(Shader shader) const
		{
			for (const auto &mesh : meshes) {
				mesh.Draw(shader);
			}
		}

	private:
		// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
		void loadModel(const stdc::fs::path &path) {
			// read file via ASSIMP
			Assimp::Importer importer;
			const aiScene *scene = importer.ReadFile(
				path,
				aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
			);
			// check for errors
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
			{
				std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
				return;
			}
			// retrieve the directory path of the filepath
			directory = path.parent_path();

			//lets set the aabb.

			
			for (const auto &mesh : std::span{scene->mMeshes, scene->mNumMeshes}) {
				for (const auto &vert : std::span{mesh->mVertices, mesh->mNumVertices}) {
					update_aabb(aabb, to_vec3(vert));
				}
			}


			// process ASSIMP's root node recursively
			processNode(scene->mRootNode, scene);
		}

		// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
		void processNode(aiNode *node, const aiScene *scene) {

			auto meshes_span = std::span{scene->mMeshes, scene->mNumMeshes};

			// process each mesh located at the current node
			for (auto mesh_index : std::span{node->mMeshes, node->mNumMeshes}) {
				// the node object only contains indices to index the actual objects in the scene. 
				// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
				const aiMesh *mesh = meshes_span[mesh_index];
				meshes.push_back(processMesh(mesh, scene));
			}

			// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
			for (auto *child_ptr : std::span{node->mChildren, node->mNumChildren}) {
				processNode(child_ptr, scene);
			}

		}

		Mesh processMesh(const aiMesh *mesh, const aiScene *scene) {
			// data to fill
			std::vector<Vertex> vertices;
			std::vector<unsigned int> indices;
			std::vector<Texture> textures;

			auto *c_arr_of_texture_coords = *mesh->mTextureCoords;
			assert(c_arr_of_texture_coords);

			auto positions = std::span{mesh->mVertices, mesh->mNumVertices};
			auto normals = std::span{mesh->mNormals, mesh->mNumVertices};
			auto tex_coords = std::span{c_arr_of_texture_coords, mesh->mNumVertices};
			auto tangents = std::span{mesh->mTangents, mesh->mNumVertices};
			auto bitangents = std::span{mesh->mBitangents, mesh->mNumVertices};

			// walk through each of the mesh's vertices
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				auto position = to_vec3(positions[i]);
				auto normal = to_vec3(normals[i]);
				auto tex_coord = to_vec2(tex_coords[i]);
				auto tangent = to_vec3(tangents[i]);
				auto bitangent = to_vec3(bitangents[i]);

				//TODO: New named initialization!
				vertices.emplace_back(position, normal, tex_coord, tangent, bitangent);
			}

			// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
			for (const auto &face : std::span{mesh->mFaces, mesh->mNumFaces}) {
				// retrieve all indices of the face and store them in the indices vector
				assert(face.mNumIndices == 3);
				for (auto index : std::span{face.mIndices, face.mNumIndices}) {
					indices.push_back(index);
				}
			}

			// process materials
			auto materials = std::span{scene->mMaterials, scene->mNumMaterials};
			aiMaterial *material = materials[mesh->mMaterialIndex];
			// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
			// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
			// Same applies to other texture as the following list summarizes:
			// diffuse: texture_diffuseN
			// specular: texture_specularN
			// normal: texture_normalN

			// 1. diffuse maps
			std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			// 2. specular maps
			std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			// 3. normal maps
			std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
			// 4. height maps
			std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

			// return a mesh object created from the extracted mesh data
			return Mesh(vertices, indices, textures);
		}

		// checks all material textures of a given type and loads the textures if they're not loaded yet.
		// the required info is returned as a Texture struct.
		std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string_view typeName)
		{
			std::vector<Texture> textures;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
				bool skip = false;
				for (const auto &loaded_texture : textures_loaded)
				{
					if (std::strcmp(loaded_texture.path.data(), str.C_Str()) == 0)
					{
						textures.push_back(loaded_texture);
						skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
						break;
					}
				}
				if (!skip)
				{   // if texture hasn't been loaded already, load it
					auto texture = Texture{};
					texture.id = TextureFromFile(str.C_Str(), this->directory);
					texture.type = typeName;
					texture.path = str.C_Str();
					textures.push_back(texture);
					textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
				}
			}
			return textures;
		}
	};
} //namespace sstm

