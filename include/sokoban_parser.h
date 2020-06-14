#pragma once

#include <cool/filesystem.h>
#include <cool/algorithm.h>

#include <fstream>
#include <string>
#include <string_view>
#include <optional>

namespace sstm {

	enum class SokobanPiece {Wall, Player, PlayerAndGoal, Box, BoxAndGoal, Goal, Floor, Nothing};

	using Level = std::vector<std::vector<SokobanPiece>>;

	inline auto maybe_sokoban_piece(auto c) -> std::optional<SokobanPiece> {
		switch (c) {
			case '#': return SokobanPiece::Wall;
			case '@': return SokobanPiece::Player;
			case '+': return SokobanPiece::PlayerAndGoal;
			case '$': return SokobanPiece::Box;
			case '*': return SokobanPiece::BoxAndGoal;
			case '.': return SokobanPiece::Goal;
			case ' ': return SokobanPiece::Floor;
			default: return std::nullopt;
		}
	}

	inline auto maybe_to_level_row(std::string_view line) -> std::optional<std::vector<SokobanPiece>> {
		auto row = std::vector<SokobanPiece>{};
		row.reserve(line.size());
		
		auto nothing_so_far = true;

		for (auto c : line) {
			if (auto maybe_piece = maybe_sokoban_piece(c)) {
				if (nothing_so_far && *maybe_piece != SokobanPiece::Wall) {
					row.push_back(SokobanPiece::Nothing);
					continue;
				}
				nothing_so_far = false;
				row.push_back(*maybe_piece);
			} else {
				return std::nullopt;
			}
		}

		if (row.empty()) {
			return std::nullopt;
		}
		

		return row;
	}

	inline auto get_line_portable(auto &fs) -> std::string {
		auto line = std::string{};
		std::getline(fs, line);
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		return line;
	} 

	inline auto parse_collection(const stdc::fs::path &file) -> std::vector<Level>{
		auto fs = std::fstream{file};

		auto levels = std::vector<Level>{};


		if (!fs.is_open()) {
			assert(false); //TODO
		}

		auto currently_in_a_row_streak = false;
		for (auto line = get_line_portable(fs); fs; line = get_line_portable(fs)) {
			if (auto maybe_row = maybe_to_level_row(line)) {
				if (!currently_in_a_row_streak) {
					//Found a new level!
					currently_in_a_row_streak = true;
					levels.emplace_back();
				}
				
				levels.back().emplace_back(*maybe_row);
			} else {
				currently_in_a_row_streak = false;
			}
		}

		return levels;
	}
} //namespace sstm