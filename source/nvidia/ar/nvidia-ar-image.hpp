/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2020 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once
#include <memory>
#include "nvidia-ar.hpp"
#include "nvidia/cuda/nvidia-cuda-stream.hpp"

namespace nvidia::ar {
	class image {
		protected:
		std::shared_ptr<::nvidia::ar::ar> _ar;
		cv_image                          _image;

		public:
		~image();

		protected:
		image();

		public:
		/** Allocate a new image.
		 * 
		 */
		image(uint32_t width, uint32_t height, cv_pixel_format pixfmt = cv_pixel_format::RGBA,
			  cv_component_type cmptyp = cv_component_type::UINT8, cv_planar planar = cv_planar::CHUNKY,
			  cv_memory memtype = cv_memory::GPU, uint32_t alignment = 1);

		::nvidia::ar::cv_image& get();

		public /* Functions */:
		static void composite(std::shared_ptr<::nvidia::ar::image> source, std::shared_ptr<::nvidia::ar::image> mask,
							  std::shared_ptr<::nvidia::ar::image> destination);
		static void composite_over_constant(std::shared_ptr<::nvidia::ar::image> source,
											std::shared_ptr<::nvidia::ar::image> mask, const uint8_t color[3],
											std::shared_ptr<::nvidia::ar::image> destination);

		/** Transfer (and convert) the source to the destination image.
		 *
		 * @param buffer Temporary buffer for conversion and/or transfer. If left blank will automatically allocate a buffer.
		 */
		static void transfer(std::shared_ptr<::nvidia::ar::image> source,
							 std::shared_ptr<::nvidia::ar::image> destination,
							 std::shared_ptr<::nvidia::ar::image> buffer = nullptr, float scale = 1.0,
							 std::shared_ptr<::nvidia::cuda::stream> stream = nullptr);
	};
} // namespace nvidia::ar
