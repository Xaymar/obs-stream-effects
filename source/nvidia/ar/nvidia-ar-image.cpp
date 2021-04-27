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

#include "nvidia-ar-image.hpp"
#include "nvidia/cuda/nvidia-cuda-obs.hpp"
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<nvidia::ar::image> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

nvidia::ar::image::~image()
{
	D_LOG_DEBUG("Finalizing... (Addr: 0x%" PRIuPTR ")", this);
	if (_image.width || _image.height)
		_ar->CVImage_Dealloc(&_image);
}

nvidia::ar::image::image() : _ar(::nvidia::ar::ar::get()), _image()
{
	D_LOG_DEBUG("Initializating... (Addr: 0x%" PRIuPTR ")", this);
}

nvidia::ar::image::image(uint32_t width, uint32_t height, cv_pixel_format pixfmt, cv_component_type cmptyp,
						 cv_planar planar, cv_memory memtype, uint32_t alignment)
	: image()
{
	auto res = _ar->CVImage_Alloc(&_image, width, height, pixfmt, cmptyp, planar, memtype, alignment);
	if (res != result::SUCCESS) {
		throw ::nvidia::ar::ar_error(res);
	}
}

nvidia::ar::cv_image& nvidia::ar::image::get()
{
	return _image;
}

void nvidia::ar::image::composite(std::shared_ptr<::nvidia::ar::image> source,
								  std::shared_ptr<::nvidia::ar::image> mask,
								  std::shared_ptr<::nvidia::ar::image> destination)
{
	auto res = ::nvidia::ar::ar::get()->CVImage_Composite(&source->get(), &mask->get(), &destination->get());
	if (res != result::SUCCESS) {
		throw ::nvidia::ar::ar_error(res);
	}
}

void nvidia::ar::image::composite_over_constant(std::shared_ptr<::nvidia::ar::image> source,
												std::shared_ptr<::nvidia::ar::image> mask, const uint8_t color[3],
												std::shared_ptr<::nvidia::ar::image> destination)
{
	auto res = ::nvidia::ar::ar::get()->CVImage_CompositeOverConstant(&source->get(), &mask->get(), color,
																	  &destination->get());
	if (res != result::SUCCESS) {
		throw ::nvidia::ar::ar_error(res);
	}
}

void nvidia::ar::image::transfer(std::shared_ptr<::nvidia::ar::image> source,
								 std::shared_ptr<::nvidia::ar::image> destination,
								 std::shared_ptr<::nvidia::ar::image> buffer, float scale,
								 std::shared_ptr<::nvidia::cuda::stream> stream)
{
	if (!stream)
		stream = ::nvidia::cuda::obs::get()->get_stream();

	auto res = ::nvidia::ar::ar::get()->CVImage_Transfer(&source->get(), &destination->get(), scale, stream->get(),
														 buffer ? &buffer->get() : nullptr);
	if (res != result::SUCCESS) {
		throw ::nvidia::ar::ar_error(res);
	}
}
