#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

#include "serialization.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/string.hpp>

#include <cool/WATCH.h>
#include <cool/literals.h>
#include <cool/algorithm.h>
#include <cool/iterator.h>
#include <cool/filesystem.h>
#include <cool/metaprogramming.h>
#include <cool/mathematics.h>

#include "shader.h"
#include "model.h"
#include "sokoban_parser.h"
#include "camera.h"

#include <cmath>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stack>

namespace sstm {
	
	[[nodiscard]] inline constexpr auto to_fov_hori(float fov_vert, float aspect_ratio) {
		return 2 * std::atan(std::tan(fov_vert / 2) * aspect_ratio);
	}

	template<typename World> //TODO
	[[nodiscard]] auto deserialize_save(const stdc::fs::path &) -> std::tuple<size_t, std::vector<typename World::Turn>, std::optional<stdc::fs::path>>;

	class World {

	private:
		using This = World;

		enum class Entity {
			Nothing = 0,
			Wall, Ground, Player, Goal, Box
		};

	public:
		glm::ivec3 controlled_pos;
		
		std::unordered_map<Entity, std::optional<Model>> maybe_models;
		std::vector<std::vector<std::vector<Entity>>> grid;
		Shader shader;
		Shader text_shader;

		std::vector<Level> levels;
		size_t loaded_level_id;

		size_t number_of_steps;

		std::optional<stdc::fs::path> maybe_path_previous_save;
		//Current is saved iff non-empty, in that case it is on top.
		std::vector<stdc::fs::path> cached_saves_for_redo;

		std::vector<glm::ivec3> goal_positions;

		Camera camera;
		float fov_vert = glm::radians(60.f);

		std::vector<size_t> high_scores;

		
		[[nodiscard]] const auto &entity_at(const glm::ivec3 &pos) const {
			assert(is_in_bounds(pos));
			return grid[static_cast<size_t>(pos.x)][static_cast<size_t>(pos.y)][static_cast<size_t>(pos.z)];
		}

		[[nodiscard]] auto &entity_at(const glm::ivec3 &pos) {
			return stdc::as_mutable(std::as_const(*this).entity_at(pos));
		}

		[[nodiscard]] auto satisfies_goal_condition() const -> bool {
			return std::all_of(RANGE(goal_positions), [&](const auto &goal_pos) {
				return entity_at(goal_pos) == Entity::Box;
			});
		}

		[[nodiscard]] auto save_state_to_file() const; 


		void load_level(size_t level_id) {
			using namespace stdc::literals;

			std::cout << "Loading level " << level_id << ".\n";
			loaded_level_id = level_id;

			number_of_steps = 0;

			const auto &level = levels[level_id];
			grid = std::vector(level.size(), std::vector<std::vector<Entity>>(2));

			auto error_pos =  glm::ivec3{-1, -1, -1};;
			controlled_pos = error_pos;
			goal_positions.clear();

			auto y_below = 0_z;
			auto y_above = y_below + 1;

			auto max_z = 0_z;

			for (auto x = 0_z; x < level.size(); ++x) {
				const auto &row = level[level.size() - x - 1]; //TODO
				stdc::maximize(max_z, row.size());
				
				auto &grid_row_below = grid[x][y_below];
				auto &grid_row_above = grid[x][y_above];
				
				grid_row_below.reserve(row.size());
				grid_row_above.reserve(row.size());		
				auto z = 0_z;
				for (auto piece: row) {
					switch (piece) {
						case SokobanPiece::Wall:
							grid_row_below.push_back(Entity::Wall);
							grid_row_above.push_back(Entity::Wall);
							break;

						case SokobanPiece::Player :
							grid_row_below.push_back(Entity::Ground);
							grid_row_above.push_back(Entity::Player);
							assert(controlled_pos == error_pos);
							controlled_pos = glm::ivec3{x, y_above, z};
							break;

						case SokobanPiece::PlayerAndGoal:
							grid_row_below.push_back(Entity::Goal);
							grid_row_above.push_back(Entity::Player);
							assert(controlled_pos == error_pos);
							controlled_pos = glm::ivec3{x, y_above, z};
							goal_positions.emplace_back(x, y_above, z);
							break;

						case SokobanPiece::Box:
							grid_row_below.push_back(Entity::Ground);
							grid_row_above.push_back(Entity::Box);
							break;

						case SokobanPiece::BoxAndGoal:
							grid_row_below.push_back(Entity::Goal);
							grid_row_above.push_back(Entity::Box);
							goal_positions.emplace_back(x, y_above, z);
							break;

						case SokobanPiece::Goal:
							grid_row_below.push_back(Entity::Goal);
							grid_row_above.push_back(Entity::Nothing);
							goal_positions.emplace_back(x, y_above, z);
							break;
						
						case SokobanPiece::Floor:
							grid_row_below.push_back(Entity::Ground);
							grid_row_above.push_back(Entity::Nothing);
							break;

						case SokobanPiece::Nothing:
							grid_row_below.push_back(Entity::Nothing);
							grid_row_above.push_back(Entity::Nothing);
							break;

						default: assert(false);
					}

					++z;
				} //for z
			} //for x
			
			auto camera_x = static_cast<float>(grid.size()) / 2.f;
			assert(max_z);
			auto camera_z = static_cast<float>(max_z) / 2.f;
			
			assert(0.f < fov_vert && fov_vert < stdc::pi<float>);
			auto fov_hori = to_fov_hori(fov_vert, 16.f / 9.f); //TODO
			assert(0.f < fov_hori && fov_hori < stdc::pi<float>);

			auto y_dist_hori = camera_z / std::tan(fov_hori / 2.f);
			auto y_dist_vert = camera_x / std::tan(fov_vert / 2.f);
			assert(y_dist_hori > 0.f);
			assert(y_dist_vert > 0.f); 
			auto y_dist = std::max(y_dist_hori, y_dist_vert);

			auto max_y = y_above + 1;
			auto camera_y = static_cast<float>(max_y) + y_dist;
			
			auto center = glm::vec3{.5f, .5f, .5f} + glm::vec3{controlled_pos}; //TODO auslagern, vereinigen
			camera = Camera{glm::vec3{camera_x, camera_y, camera_z}, center};

			assert(controlled_pos != error_pos);
			assert(!satisfies_goal_condition());

			turns.clear();
			next_turn_id = 0;
		}

		[[nodiscard]] auto serialize_level_state() const {
			using namespace stdc::literals;
			auto save_folder = stdc::fs::path{"saves"s};
			auto i = 0;
			auto save_path = stdc::fs::path{};
			do {
				save_path = save_folder / std::to_string(i);
				++i;
			} while (stdc::fs::exists(save_path));
			
			auto os = std::ofstream{save_path};
			auto oa = boost::archive::text_oarchive{os};
			// write class instance to archive
			//Todo auslagern für deserialize
			oa << loaded_level_id;
			oa << turns;
			oa << maybe_path_previous_save;
			
			return save_path;
		}

		void deserialize_high_scores() {
			using namespace stdc::literals;
			
			assert(high_scores.empty());
			
			auto save_folder = stdc::fs::path{"saves"s};
			auto high_scores_path = save_folder / "high_scores";

			if (!stdc::fs::exists(high_scores_path)) {
				//TODO
				high_scores = std::vector(levels.size(), stdc::nullid);
				return;
			}
			
			auto is = std::ifstream{high_scores_path};
			auto ia = boost::archive::text_iarchive{is};
			
			ia >> high_scores;
			assert(high_scores.size() == levels.size()); 

		}

		void serialize_high_scores() const {
			using namespace stdc::literals;
			
			assert(high_scores.size() == levels.size());
			
			auto save_folder = stdc::fs::path{"saves"s};
			auto high_scores_path = save_folder / "high_scores";

			auto os = std::ofstream{high_scores_path};
			auto oa = boost::archive::text_oarchive{os};
			
			oa << high_scores;
		}

		void maybe_revert_to_previous_save() {
			using namespace stdc::literals;
			
			if (!maybe_path_previous_save) {
				return;
			}

			if (cached_saves_for_redo.empty()) {
				cached_saves_for_redo.push_back(serialize_level_state());
			}
			cached_saves_for_redo.push_back(*maybe_path_previous_save);

			auto [saved_level_id, saved_turns, saved_maybe_path_previous_save] = deserialize_save<World>(*maybe_path_previous_save);

			load_level(saved_level_id);
			maybe_path_previous_save = saved_maybe_path_previous_save;
			assert(turns.empty());
			turns = std::move(saved_turns);
			
			for (auto i = 0_z; i < turns.size(); ++i) {
				//TODO: VERY BAD! THIS CHECKS GOAL CONDITION! SHOULD ALSO RETURN IF IT DID SOMETHING
				maybe_do_next_turn();
				assert(!turns.empty()); //for now
			}
			assert(next_turn_id == turns.size());
		}

		
		void maybe_forward_to_next_save() {
			using namespace stdc::literals;
			
			if (cached_saves_for_redo.size() < 2) {
				return;
			}
			
			[[maybe_unused]] auto previous_path = cached_saves_for_redo.back();
			cached_saves_for_redo.pop_back();
			const auto &save_to_load = cached_saves_for_redo.back();

			auto [saved_level_id, saved_turns, saved_maybe_path_previous_save] = deserialize_save<World>(save_to_load);

			load_level(saved_level_id);
			maybe_path_previous_save = saved_maybe_path_previous_save;
			assert(previous_path == maybe_path_previous_save);
			assert(turns.empty());
			turns = std::move(saved_turns);
		}

		World() :
			controlled_pos{},
			shader{"shader.vs", "shader.fs"},
			text_shader{"font_shader.vs", "font_shader.fs"},
			loaded_level_id{},
			number_of_steps{},
			maybe_path_previous_save{},
			high_scores{},
			next_turn_id{}
		{

			WATCH("box").reset();
			WATCH("box").start();
			maybe_models.try_emplace(Entity::Box, "resources/objects/box/box.obj");
			WATCH("box").stop();
			std::cout << "box: " << WATCH("box").elapsed<std::chrono::milliseconds>() << " ms\n\n";
			
			WATCH("backpack").reset();
			WATCH("backpack").start();
			maybe_models.try_emplace(Entity::Player, "resources/objects/backpack/backpack.obj");
			WATCH("backpack").stop();
			std::cout << "backpack: " << WATCH("backpack").elapsed<std::chrono::milliseconds>() << " ms\n\n";
			
			WATCH("block").reset();
			WATCH("block").start();
			maybe_models.try_emplace(Entity::Ground, "resources/objects/block/Grass_Block.obj");
			WATCH("block").stop();
			std::cout << "block: " << WATCH("block").elapsed<std::chrono::milliseconds>() << " ms\n\n";
						
			WATCH("plate").reset();
			WATCH("plate").start();
			maybe_models.try_emplace(Entity::Goal, "resources/objects/plate/Grass_Block.obj");
			WATCH("plate").stop();
			std::cout << "plate: " << WATCH("plate").elapsed<std::chrono::milliseconds>() << " ms\n\n";
			
			WATCH("wall").reset();
			WATCH("wall").start();
			maybe_models.try_emplace(Entity::Wall, "resources/objects/wall/wall.obj");
			WATCH("wall").stop();
			std::cout << "wall: " << WATCH("wall").elapsed<std::chrono::milliseconds>() << " ms\n\n";
			
			maybe_models.try_emplace(Entity::Nothing);
			
			levels = parse_collection("/home/jgr/Downloads/level/Homz _Challenge/Homz Challenge.txt");
			std::cout << "Parsed levels: " << levels.size() << ".\n";

			deserialize_high_scores();

			assert(!levels.empty());
			load_level(0);
		}



		constexpr World(const This &) = delete;
		constexpr auto operator=(const This &) & -> World & = delete;
		constexpr World(This &&) noexcept = delete;
		constexpr auto operator=(This &&) &noexcept-> World & = delete;
		~World() {
			serialize_high_scores();
		}

		[[nodiscard]] auto is_in_bounds(const glm::ivec3 &pos) const -> bool {
			return 0 <= pos.x && pos.x < static_cast<ptrdiff_t>(grid.size()) &&
				0 <= pos.y && pos.y < static_cast<ptrdiff_t>(grid[static_cast<size_t>(pos.x)].size()) &&
				0 <= pos.z && pos.z < static_cast<ptrdiff_t>(grid[static_cast<size_t>(pos.x)][static_cast<size_t>(pos.y)].size());	
		}

		void check_goals() {
			if (satisfies_goal_condition()) {
				//TODO
				if (high_scores[loaded_level_id] > next_turn_id) {
					std::cout << "New high score! " << next_turn_id << " instead of " << high_scores[loaded_level_id] << ".\n";
				}
				stdc::minimize(high_scores[loaded_level_id], next_turn_id);
				--next_turn_id;
				load_next_level();
			}
		}

		class Change {
		public:
			glm::ivec3 pos;
		private:
			Entity before;
			Entity after;
		public:
			[[nodiscard]] constexpr auto get_before() const {
				return before;
			}
	
			[[nodiscard]] constexpr auto get_after() const {
				return after;
			}

			//TODO
			Change() = default;

			template<class Archive>
			void serialize(Archive &ar, const unsigned int) {
				ar & pos;
				ar & before;
				ar & after;
			}

			Change(glm::ivec3 _pos, Entity _before, Entity _after) :
				pos{_pos}, before{_before}, after{_after}
			{
				assert(before != after);
			}
		};

		void apply(const Change &change) {
			auto &entity = entity_at(change.pos);
			assert(entity == change.get_before());
			entity = change.get_after();
		}

		void revert(Change change) {
			auto reverse_change = Change{change.pos, change.get_after(), change.get_before()};
			apply(reverse_change);
		}

		class Turn {
		private:
			std::vector<Change> changes{};
			glm::ivec3 controlled_pos_before{};
			glm::ivec3 controlled_pos_after{};

		public:
			template<class Archive>
			void serialize(Archive &ar, const unsigned int) {
				ar & changes;
				ar & controlled_pos_before;
				ar & controlled_pos_after;
			}
		
			Turn() = default; //TODO: Serialization needs this? whä
			
			Turn(
				std::vector<Change> _changes,
				glm::ivec3 _controlled_pos_before,
				glm::ivec3 _controlled_pos_after
			) :
				changes{std::move(_changes)},
				controlled_pos_before{_controlled_pos_before},
				controlled_pos_after{_controlled_pos_after}
			{
				[[maybe_unused]] auto change_to_pos = [](const auto &change) -> const auto & {
					return change.pos;
				};
				assert(stdc::contains_no_duplicates(
					stdc::transform_iterator{changes.begin(), change_to_pos},
					stdc::transform_iterator{changes.end(), change_to_pos}
				));

				assert(controlled_pos_before != controlled_pos_after);
			}

			using const_iterator = decltype(changes)::const_iterator;
			[[nodiscard]] auto begin() const noexcept -> const_iterator { return changes.begin(); }
			[[nodiscard]] auto end() const noexcept -> const_iterator { return changes.end(); }

			[[nodiscard]] constexpr auto get_controlled_pos_before() const {
				return controlled_pos_before;
			}
	
			[[nodiscard]] constexpr auto get_controlled_pos_after() const {
				return controlled_pos_after;
			}

		};

		void maybe_do_next_turn() {
			if (next_turn_id == turns.size()) {
				maybe_forward_to_next_save();
				return;
			}

			auto turn = turns[next_turn_id];
			++next_turn_id;
			
			for (const auto &change : turn) {
				apply(change);
			}

			assert(controlled_pos == turn.get_controlled_pos_before());
			controlled_pos = turn.get_controlled_pos_after();	
			check_goals();
		}

		void maybe_undo_previous_turn() {
			if (!next_turn_id) {
				maybe_revert_to_previous_save();
				return;
			}

			auto turn = turns[next_turn_id - 1];
			--next_turn_id;

			for (const auto &change : turn) {
				revert(change);
			}

			assert(controlled_pos == turn.get_controlled_pos_after());
			controlled_pos = turn.get_controlled_pos_before();	
			// check_goals();
		}

		//TODO: High risk that I call this with a turn in turns, which would be very wrong!
		void apply(const Turn &turn) {
			//TODO: Auslagern? Sonst auf mehreren Ebenen nötig, je nachdem, ob man bei Leveln oder bei Turns weitergeht.
			turns.erase(stdc::to_it(turns, next_turn_id), turns.end());
			cached_saves_for_redo.clear();
			turns.push_back(turn);
			maybe_do_next_turn();
		}
		
		void move(const glm::ivec3 &translation) {
			assert(is_in_bounds(controlled_pos));
			auto target_pos = controlled_pos + translation;
			
			if (!is_in_bounds(target_pos)) {
				return;
			}

			if (entity_at(target_pos) == Entity::Box) {
				auto box_target = target_pos + translation;

				if (!is_in_bounds(box_target)) {
					return;
				}

				if (entity_at(box_target) == Entity::Nothing) {
					auto changes = std::vector<Change>{};
					changes.emplace_back(target_pos, Entity::Box, entity_at(controlled_pos));
					changes.emplace_back(controlled_pos, entity_at(controlled_pos), Entity::Nothing);
					changes.emplace_back(box_target, Entity::Nothing, Entity::Box);
					apply(Turn{std::move(changes), controlled_pos, target_pos});
				}
				return;
			}

			if (entity_at(target_pos) == Entity::Nothing) {
				auto changes = std::vector<Change>{};
				changes.emplace_back(target_pos, Entity::Nothing, entity_at(controlled_pos));
				changes.emplace_back(controlled_pos, entity_at(controlled_pos), Entity::Nothing);
				apply(Turn{std::move(changes), controlled_pos, target_pos});
			}
		}

		//TODO: easy to call load_level when transition is what we want.
		void transition_to_level(size_t level_id) {
			turns.erase(stdc::to_it(turns, next_turn_id), turns.end());
			cached_saves_for_redo.clear();
			maybe_path_previous_save = serialize_level_state();

			load_level(level_id);
		}

	private:
		std::vector<Turn> turns;
	public: //TODO
		size_t next_turn_id;
	private:

	public:
		void reload_level() {
			//TODO: Eventuell hier später auch returnen, wenn der Levelstate initial ist?
			if (!next_turn_id) {
				return;
			}

			transition_to_level(loaded_level_id);
		}
		
		void load_next_level() {
			if (loaded_level_id + 1 == levels.size()) {
				return;
			}

			transition_to_level(loaded_level_id + 1);
		}

		void load_previous_level() {
			if (!loaded_level_id) {
				return;
			}

			transition_to_level(loaded_level_id - 1);
		}
	}; //World


	template<typename World>
	[[nodiscard]] auto deserialize_save(const stdc::fs::path &load_path) -> std::tuple<size_t, std::vector<typename World::Turn>, std::optional<stdc::fs::path>> {
	using namespace stdc::literals;
	
	//TODO: Robustness agains users changing files etc
	assert(stdc::fs::exists(load_path));
	auto is = std::ifstream{load_path};
	auto ia = boost::archive::text_iarchive{is};
	//Todo auslagern für serialize
	
	auto result = std::tuple<size_t, std::vector<typename World::Turn>, std::optional<stdc::fs::path>>{};
	auto string_path = stdc::fs::path{};

	ia >> std::get<0>(result);
	ia >> std::get<1>(result);
	ia >> std::get<2>(result);

	return result;
}



} //namespace sstm