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

#include "nvidia-ar-facedetection.hpp"
#include <stdexcept>
#include "obs/gs/gs-helper.hpp"
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<nvidia::ar::facedetection> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

nvidia::ar::facedetection::~facedetection()
{
	D_LOG_DEBUG("Finalizing... (Addr: 0x%" PRIuPTR ")", this);
	//_feature->set(NVAR_PARAMETER_INPUT("Image"), nullptr, sizeof(nvidia::ar::cv_image));
	_feature.reset();
}

nvidia::ar::facedetection::facedetection()
	: _cobs(::nvidia::cuda::obs::get()), _ar(::nvidia::ar::ar::get()), _loaded(false)
{
	D_LOG_DEBUG("Initializing... (Addr: 0x%" PRIuPTR ")", this);

	_feature = std::make_shared<::nvidia::ar::feature>(NVAR_FEATURE_FACEDETECTION);

	// Set up initial configuration
	set_limit(1);
	enable_temporal_tracking(true);
}

void nvidia::ar::facedetection::enable_temporal_tracking(bool value)
{
	if (_loaded)
		throw std::logic_error("Can't change configuration after initialization.");

	_feature->set(NVAR_PARAMETER_CONFIG("Temporal"), value ? uint32_t(1) : uint32_t(0));
}

void nvidia::ar::facedetection::set_limit(size_t limit)
{
	if (_loaded) {
		throw std::logic_error("Can't change configuration after initialization.");
	}
	if (limit == 0) {
		throw std::invalid_argument("limit must be greater or equal to 1");
	}

	// Resize Data
	_rects            = std::vector<rect_t>(limit);
	_rects_confidence = std::vector<float>(limit, 0);

	// Update Structure
	_bboxes.rects   = _rects.data();
	_bboxes.maximum = limit;
	_bboxes.current = 0;

	// Update Object
	_feature->set(NVAR_PARAMETER_OUTPUT("BoundingBoxes"), reinterpret_cast<void*>(&_bboxes), sizeof(boundaries));
	_feature->set(NVAR_PARAMETER_OUTPUT("BoundingBoxesConfidence"), _rects_confidence);
}

void nvidia::ar::facedetection::track(std::shared_ptr<gs::texture> ptexture)
{
	if (!ptexture)
		throw std::invalid_argument("texture is nullptr");

	// Tracking must be done on a copy, we can't be sure that the original texture is still valid.
	// This is especially problematic due to gs::rendertarget.

	// Check if our input is still valid, and if it is not, update it.
	if ((!_input_texture) //
		|| (_input_texture->get_width() != ptexture->get_width())
		|| (_input_texture->get_height() != ptexture->get_height())
		|| (_input_texture->get_color_format() != ptexture->get_color_format())) {
		try {
			uint32_t width  = ptexture->get_width();
			uint32_t height = ptexture->get_height();

			auto  texture = std::make_shared<gs::texture>(width, height, ptexture->get_color_format(), 1, nullptr,
                                                         gs::texture::flags::Dynamic);
			auto  mapped  = std::make_shared<nvidia::ar::gstexture>(texture);
			auto& image   = mapped->get();

			auto buffer = std::make_shared<nvidia::ar::image>(width, height, image.pixel_format, image.component_type,
															  cv_planar::INTERLEAVED, cv_memory::GPU, 0);
			auto input =
				std::make_shared<nvidia::ar::image>(width, height, cv_pixel_format::BGR, cv_component_type::UINT8,
													cv_planar::INTERLEAVED, cv_memory::GPU, 0);

			_feature->set(NVAR_PARAMETER_INPUT("Image"), &input->get(), sizeof(nvidia::ar::cv_image));

			{ // Update stored values.
				// Unregister the texture from CUDA & AR ...
				_input_mapped.reset();
				// ... then actually replace the original texture, ...
				_input_texture = texture;
				// ... to finally store the new mapped texture.
				_input_mapped = mapped;
				// The rest is just assigning other values.
				_input        = input;
				_input_buffer = buffer;
			}
		} catch (...) {
			_input_mapped.reset();
			_input_texture.reset();
			_input_buffer.reset();
			_input.reset();
			throw;
		}
	}

	// Update Buffers.
	gs_copy_texture(_input_texture->get_object(), ptexture->get_object());
	_input_mapped->update();
	_input_mapped->transfer(_input_mapped, _input);

	// Track
	_feature->run();
}

size_t nvidia::ar::facedetection::count()
{
	return _bboxes.current;
}

nvidia::ar::rect_t const& nvidia::ar::facedetection::at(size_t index)
{
	float v;
	return at(index, v);
}

nvidia::ar::rect_t const& nvidia::ar::facedetection::at(size_t index, float& confidence)
{
	if (_bboxes.current == 0)
		throw std::runtime_error("no tracked faces");
	if (index > _bboxes.current)
		throw std::out_of_range("index too large");
	auto& ref  = _rects.at(index);
	confidence = _rects_confidence.at(index);
	return ref;
}

void nvidia::ar::facedetection::load()
{
	std::string models_path = _ar->get_model_path().string();
	_feature->set(NVAR_PARAMETER_CONFIG("CUDAStream"), _cobs->get_stream());
	_feature->set(NVAR_PARAMETER_CONFIG("ModelDir"), models_path.c_str());
	_feature->load();
}
