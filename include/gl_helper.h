#pragma once

namespace sstm {

	[[nodiscard]] inline auto to_gl_offset(size_t index) -> const void * {
		return reinterpret_cast<const void *>(index); //NOLINT
	}

} //namespace sstm