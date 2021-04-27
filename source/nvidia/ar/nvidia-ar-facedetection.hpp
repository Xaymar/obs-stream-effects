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
#include "nvidia-ar-feature.hpp"
#include "nvidia-ar-gs-texture.hpp"
#include "nvidia-ar-image.hpp"
#include "nvidia/cuda/nvidia-cuda-gs-texture.hpp"
#include "nvidia/cuda/nvidia-cuda-obs.hpp"

namespace nvidia::ar {
	class facedetection {
		std::shared_ptr<nvidia::ar::feature> _feature;
		std::shared_ptr<nvidia::cuda::obs>   _cobs;
		std::shared_ptr<nvidia::ar::ar>      _ar;
		bool                                 _loaded;

		// Output
		std::vector<rect_t> _rects;
		std::vector<float>  _rects_confidence;
		boundaries          _bboxes;

		// Input
		std::shared_ptr<gs::texture>           _input_texture;
		std::shared_ptr<nvidia::ar::gstexture> _input_mapped;
		std::shared_ptr<nvidia::ar::image>     _input_buffer;
		std::shared_ptr<nvidia::ar::image>     _input;

		public:
		~facedetection();

		/** Create a new face detection feature.
		 *
		 * Must be in a graphics and CUDA context when calling.
		 */
		facedetection();

		/** Enable Temporal tracking, which is more stable but only tracks a single face.
		 *
		 * Normally faces are detected with a best effort approach, which is temporally
		 * unstable, but allows more than a single face to be detected and tracked. By
		 * enabling temporal tracking we limit ourselves to a single face with insane
		 * tracking precision and accuracy.
		 */
		void enable_temporal_tracking(bool value);

		void set_limit(size_t limit);

		/** Load the actual effect into memory.
		 *
		 * This is an expensive operation and should not be done asynchronously.
		 *
		 * Must be in a graphics and CUDA context when calling.
		 */
		void load();

		/** Track any faces in the given texture.
		 *
		 * This is a partially expensive operation which will automatically copy
		 * and allocate memory if necessary.
		 *
		 * Must be in a graphics and CUDA context when calling.
		 */
		void track(std::shared_ptr<gs::texture> texture);

		size_t count();

		rect_t const& at(size_t index);

		rect_t const& at(size_t index, float& confidence);
	};
} // namespace nvidia::ar
