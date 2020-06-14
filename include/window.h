#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "camera.h"
#include "world.h"

#include <exception>
#include <cassert>
#include <memory>
#include <cmath>

namespace sstm {
	class MainWindow {
	private:
		using This = MainWindow;

		int aspect_ratio_vert = 9;
		int aspect_ratio_hori = 16;

		GLFWwindow *handle;

		double last_mouse_x = 0.;
		double last_mouse_y = 0.;
		bool mouse_tracked = false;
		bool last_mouse_position_set = false;

	

	public:

		std::unique_ptr<World> world_ptr;

		[[nodiscard]] auto wants_to_close() const -> bool {
			return glfwWindowShouldClose(handle);
		}

		[[nodiscard]] auto get_aspect_ratio() const {
			return static_cast<float>(aspect_ratio_hori) / static_cast<float>(aspect_ratio_vert);
		}

		void swap_buffer() const {
			glfwSwapBuffers(handle);
		}

		MainWindow() try {
			[[maybe_unused]] static auto was_constructed_before = false;
			assert(!was_constructed_before);
			was_constructed_before = true;
			
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

			glfwWindowHint(GLFW_SAMPLES, 16);

			auto initial_width = 802;
			auto initial_height = 200;

			handle = glfwCreateWindow(initial_width, initial_height, "sstm", nullptr, nullptr);
			if (!handle) {
				throw std::runtime_error{"Failed to create main window."};
			}
			

			glfwMakeContextCurrent(handle);
			
			
			if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) { //NOLINT
				throw std::runtime_error{"Failed to initialize GLAD."};
			}
				
			glEnable(GL_MULTISAMPLE);

			
    		world_ptr = std::make_unique<World>();

			glfwSetWindowUserPointer(handle, this);
			glfwSetFramebufferSizeCallback(handle, framebuffer_size_callback);
			glfwSetCursorPosCallback(handle, mouse_callback);
			glfwSetScrollCallback(handle, scroll_callback);
			glfwSetKeyCallback(handle, key_callback);
			glfwSetMouseButtonCallback(handle, mouse_button_callback);

			//Initialize the viewport correcly.
			framebuffer_size_callback(handle, initial_width, initial_height);

		} catch (...) {
			glfwTerminate();
			throw;
		}

		~MainWindow() noexcept {
			glfwDestroyWindow(handle);
			glfwTerminate();
		}
		constexpr MainWindow(const This &) = delete;
		constexpr auto operator=(const This &) & -> MainWindow & = delete;
		constexpr MainWindow(This &&) noexcept = delete;
		constexpr auto operator=(This &&) &noexcept-> MainWindow & = delete;

		// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
		// ---------------------------------------------------------------------------------------------------------
		void process_keyboard_input(float deltaTime)
		{
			if (glfwGetKey(handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(handle, true);
			}

			if (glfwGetKey(handle, GLFW_KEY_W) == GLFW_PRESS) {
				world_ptr->camera.ProcessKeyboard(FORWARD, deltaTime);
			}

			if (glfwGetKey(handle, GLFW_KEY_S) == GLFW_PRESS) {
				world_ptr->camera.ProcessKeyboard(BACKWARD, deltaTime);
			}

			if (glfwGetKey(handle, GLFW_KEY_A) == GLFW_PRESS) {
				world_ptr->camera.ProcessKeyboard(LEFT, deltaTime);
			}

			if (glfwGetKey(handle, GLFW_KEY_D) == GLFW_PRESS) {
				world_ptr->camera.ProcessKeyboard(RIGHT, deltaTime);
			}
		}

		static void mouse_button_callback(GLFWwindow *handle, int button, int action, int) {
			auto &window = *static_cast<This *>(glfwGetWindowUserPointer(handle));
			
			if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
				glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				window.mouse_tracked = false;
				window.last_mouse_position_set = false;
			}
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
				glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				window.mouse_tracked = true;
			}
		}

		static void key_callback(GLFWwindow *handle, int key, int, int action, int mods) {
			auto &window = *static_cast<This *>(glfwGetWindowUserPointer(handle));
			
			if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
				auto translation = glm::ivec3{1, 0, 0};
				window.world_ptr->move(translation);
			}
			if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
				auto translation = glm::ivec3{-1, 0, 0};
				window.world_ptr->move(translation);
			}
			if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
				auto translation = glm::ivec3{0, 0, -1};
				window.world_ptr->move(translation);
			}
			if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
				auto translation = glm::ivec3{0, 0, 1};
				window.world_ptr->move(translation);
			}
			if (key == GLFW_KEY_R && action == GLFW_PRESS) {
				window.world_ptr->reload_level();
			}
			if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS) {
				switch (mods) {
					case 0: window.world_ptr->maybe_undo_previous_turn(); break;
					case GLFW_MOD_SHIFT: window.world_ptr->maybe_do_next_turn();
					default: ; //no effect
				}
			}
		}

		void render(float deltaTime) const {
			using namespace stdc::literals;

			auto viewport_data = std::array<GLint, 4>{};
			glGetIntegerv(GL_VIEWPORT, viewport_data.data());
			
			glDisable(GL_SCISSOR_TEST);
			glClearColor(0.f, 0.f, 0.f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_SCISSOR_TEST);

			glClearColor(.05f, .05f, .05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			

			// don't forget to enable shader before setting uniforms
			world_ptr->shader.use();
			
			//todo wo?
			//todo camera zoom not used anymore now

			// view/projection transformations
			auto center = glm::vec3{.5f, .5f, .5f} + glm::vec3{world_ptr->controlled_pos};

			glm::mat4 projection = glm::infinitePerspective(world_ptr->fov_vert, get_aspect_ratio(), 0.1f);
			glm::mat4 view = world_ptr->camera.GetViewMatrix(center, deltaTime);
			world_ptr->shader.setMat4("projection", projection);
			world_ptr->shader.setMat4("view", view);

			//Move the light.
			auto radius = 10.f;
			auto past_time = static_cast<float>(glfwGetTime());
			auto light_angle = past_time * .1f;
			
			auto light_source_pos = glm::vec3{
				std::cos(light_angle) * radius,
				0.f,
				std::sin(light_angle) * radius};

			//lighting, I guess.
			world_ptr->shader.setVec3("light.position", light_source_pos);
			world_ptr->shader.setVec3("light.ambient", .2f, .2f, .2f);
			world_ptr->shader.setVec3("light.diffuse", .5f, .5f, .5f);
			world_ptr->shader.setVec3("light.specular", 1.f, 1.f, 1.f);

			world_ptr->shader.setVec3("viewPos", world_ptr->camera.Position);

			// render all entities
			for (auto x = 0_z; x < world_ptr->grid.size(); ++x) {
				for (auto y = 0_z; y < world_ptr->grid[x].size(); ++y) {
					for (auto z = 0_z; z < world_ptr->grid[x][y].size(); ++z) {
						auto entity = world_ptr->grid[x][y][z];

						const auto &maybe_model_3d = world_ptr->maybe_models[entity];

						if (!maybe_model_3d) {
							continue;
						}

						const auto &model_3d = *maybe_model_3d;

						const auto &aabb = model_3d.aabb;
						auto expansion = aabb.max - aabb.min;
						auto max_expansion = std::max(std::max(expansion.x, expansion.y), expansion.z);
						
						auto scale = glm::vec3{
							1.f / max_expansion,
							1.f / max_expansion,
							1.f / max_expansion
						};

						auto translation = glm::vec3{
							static_cast<float>(x),
							static_cast<float>(y),
							static_cast<float>(z)
						};
					
						glm::mat4 model = glm::mat4(1.0f);
						model = glm::translate(model, translation);
						model = glm::scale(model, scale);
						model = glm::translate(model, -aabb.min);
						
						world_ptr->shader.setMat4("model", model);
			
						model_3d.Draw(world_ptr->shader);
					}
				}
			}

			//show what we got.
			swap_buffer();
		}

	private:
		

		static void mouse_callback(GLFWwindow *handle, double xpos, double ypos) {
			auto &window = *static_cast<This *>(glfwGetWindowUserPointer(handle));
			
			if (!window.mouse_tracked) {
				assert(!window.last_mouse_position_set);
				return;
			}

			if (!window.last_mouse_position_set) {
				window.last_mouse_x = xpos;
				window.last_mouse_y = ypos;
				window.last_mouse_position_set = true;
				return;
			}

			auto xoffset = static_cast<float>(xpos - window.last_mouse_x);
			auto yoffset = static_cast<float>(window.last_mouse_y - ypos); // reversed since y-coordinates go from bottom to top
			window.world_ptr->camera.ProcessMouseMovement(xoffset, yoffset);

			window.last_mouse_x = xpos;
			window.last_mouse_y = ypos;
		}

		// glfw: whenever the window size changed (by OS or user resize) this callback function executes
		// ---------------------------------------------------------------------------------------------
		//TODO: Das ist alles noch sehr sketchy mit den Rundungen und Extrapixeln für Symmetrie.
		static void framebuffer_size_callback(GLFWwindow *handle, int width, int height) {
			auto &window = *static_cast<This*>(glfwGetWindowUserPointer(handle));
			
			auto aspect_ratio = window.get_aspect_ratio();
			auto window_aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

			auto x_offset = 0;
			auto y_offset = 0;

			if (window_aspect_ratio < aspect_ratio) {
				auto scaled_height = static_cast<int>(std::lround(static_cast<float>(width) / aspect_ratio));
				assert(scaled_height <= height);
				if (height - scaled_height % 2) {
					scaled_height += 1;
				}
				y_offset = (height - scaled_height) / 2;
				height = scaled_height;
			} else {
				auto scaled_width = static_cast<int>(std::lround(static_cast<float>(height) * aspect_ratio));
				assert(scaled_width <= width);
				if (width - scaled_width % 2) {
					scaled_width += 1;
				}
				x_offset = (width - scaled_width) / 2;
				width = scaled_width;
			}
		
			glViewport(x_offset, y_offset, width, height);
			glScissor(x_offset, y_offset, width, height);
		}

		// glfw: whenever the mouse scroll wheel scrolls, this callback is called
		// ----------------------------------------------------------------------
		static void scroll_callback(GLFWwindow *handle, double, double yoffset) {
			auto &window = *static_cast<This *>(glfwGetWindowUserPointer(handle));
			window.world_ptr->camera.ProcessMouseScroll(static_cast<float>(yoffset));
		}

	};
} //namespace sstm
