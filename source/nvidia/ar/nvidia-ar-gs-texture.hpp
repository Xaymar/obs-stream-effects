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
#include "nvidia-ar-image.hpp"
#include "nvidia-ar.hpp"
#include "nvidia/cuda/nvidia-cuda-gs-texture.hpp"
#include "nvidia/cuda/nvidia-cuda-memory.hpp"
#include "nvidia/cuda/nvidia-cuda-obs.hpp"
#include "nvidia/cuda/nvidia-cuda.hpp"
#include "obs/gs/gs-texture.hpp"

namespace nvidia::ar {
	class gstexture : public image {
		std::shared_ptr<nvidia::cuda::obs> _cobs;

		std::shared_ptr<gs::texture>             _texture;
		std::shared_ptr<nvidia::cuda::gstexture> _texture_cuda;
		std::shared_ptr<nvidia::cuda::memory>    _memory;

		public:
		/** Destructor.
		 * 
		 * Must be in a Graphics and CUDA context at the time of calling.
		 */
		~gstexture();

		/** Create a new image from a graphics resource.
		 * 
		 * Must be in a Graphics and CUDA context at the time of calling.
		 */
		gstexture(std::shared_ptr<gs::texture> texture);

		/** Update the image with new data from the gs::texture object.
		 *
		 * Must be called on every update, as there is no way to directly map a
		 * graphics resource into the SDK for now. This will hopefully be 
		 * addressed by NVIDIA in a future update.
		 *
		 * Must be in a Graphics and CUDA context at the time of calling.
		 */
		void update();

		public /* Direct Access*/:
		std::shared_ptr<gs::texture>             get_texture();
		std::shared_ptr<nvidia::cuda::gstexture> get_cuda_texture();
		std::shared_ptr<nvidia::cuda::memory>    get_cuda_memory();
		nvidia::ar::cv_image&                    get();
	};
} // namespace nvidia::ar
