#pragma once

#include <optional>
#include <vector>
#include <string>

#include <cool/filesystem.h>

#include <boost/serialization/split_free.hpp>
#include <glm/vec3.hpp>

BOOST_SERIALIZATION_SPLIT_FREE(stdc::fs::path)

namespace boost {
	namespace serialization {
		
		// BOOST_SERIALIZATION_SPLIT_FREE(std::optional<T>)		
		template<class Archive, typename T>
		inline void serialize( 
			Archive & ar,      
			std::optional<T> & t,             
			const unsigned int file_version
		){
			split_free(ar, t, file_version);       
		} 

		template<class Archive>
		void serialize(Archive &ar, glm::ivec3 &vec, const unsigned int) {
			ar & vec.x;
			ar & vec.y;
			ar & vec.z;
		}

		template<class Archive>
		void save(Archive & ar, const stdc::fs::path &path, const unsigned int) {
			ar & path.string();
		}
		template<class Archive>
		void load(Archive & ar, stdc::fs::path &path, const unsigned int) {
			auto str = std::string{};
			ar & str;
			path = stdc::fs::path{std::move(str)};
		}

		template<class Archive, typename T>
		void save(Archive & ar, const std::optional<T> &opt, const unsigned int) {
			if (!opt) {
				ar & false;
				return;
			}
		
			ar & true;
			ar & *opt;
		}

		template<class Archive, typename T>
		void load(Archive & ar, std::optional<T> &opt, const unsigned int) {
			auto has_value = bool{};
			ar & has_value;
			if (!has_value) {
				opt = std::nullopt;
				return;
			}

			auto value = T{};
			ar & value;
			opt = std::move(value);
		}
	} // namespace serialization
} // namespace boost