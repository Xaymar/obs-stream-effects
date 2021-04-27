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

#include "nvidia-ar-gs-texture.hpp"
#include "obs/gs/gs-helper.hpp"
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<nvidia::ar::gstexture> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

nvidia::ar::gstexture::~gstexture()
{
	D_LOG_DEBUG("Finalizing... (Addr: 0x%" PRIuPTR ")", this);

	memset(&_image, 0, sizeof(cv_image));
	_memory.reset();
	_texture.reset();
}

nvidia::ar::gstexture::gstexture(std::shared_ptr<gs::texture> texture)
	: image(), _cobs(::nvidia::cuda::obs::get()),

	  _texture(texture), _texture_cuda(), _memory()
{
	P_LOG_DEBUG("Initializating... (Addr: 0x%" PRIuPTR ")", this);

	cv_pixel_format   pixfmt;
	cv_component_type cmptyp;
	size_t            pitch_size;

	// Detect Pixel Format and Pitch size.
	switch (_texture->get_color_format()) {
	case GS_A8:
		pixfmt     = cv_pixel_format::A;
		pitch_size = 1;
		break;
	case GS_R8:
	case GS_R16:
	case GS_R16F:
	case GS_R32F:
		pixfmt     = cv_pixel_format::Y;
		pitch_size = 1;
		break;
	case GS_R8G8:
	case GS_RG16F:
	case GS_RG32F:
		pixfmt     = cv_pixel_format::YA;
		pitch_size = 2;
		break;
	case GS_RGBA:
	case GS_RGBA16:
	case GS_RGBA16F:
	case GS_RGBA32F:
		pixfmt     = cv_pixel_format::RGBA;
		pitch_size = 4;
		break;
	case GS_BGRX:
	case GS_BGRA:
		pixfmt     = cv_pixel_format::BGRA;
		pitch_size = 4;
		break;
	default:
		throw std::runtime_error("Texture Format not supported.");
	}

	// Detect Component Type
	switch (_texture->get_color_format()) {
	case GS_R8:
	case GS_A8:
	case GS_R8G8:
	case GS_RGBA:
	case GS_BGRX:
	case GS_BGRA:
		cmptyp = cv_component_type::UINT8;
		break;
	case GS_R16:
	case GS_RGBA16:
		cmptyp = cv_component_type::UINT16;
		break;
	case GS_R16F:
	case GS_RG16F:
	case GS_RGBA16F:
		cmptyp = cv_component_type::FLOAT16;
		break;
	case GS_R32F:
	case GS_RG32F:
	case GS_RGBA32F:
		cmptyp = cv_component_type::FLOAT32;
		break;
	default:
		throw std::runtime_error("Texture Format not supported.");
	}

	// Create buffers and image.
	size_t pitch  = pitch_size * _texture->get_width();
	_texture_cuda = std::make_shared<::nvidia::cuda::gstexture>(_texture);
	_memory       = std::make_shared<::nvidia::cuda::memory>(pitch * _texture->get_height());
	if (auto res = _ar->CVImage_Init(
			&_image, static_cast<uint32_t>(_texture->get_width()), static_cast<uint32_t>(_texture->get_height()), pitch,
			reinterpret_cast<void*>(_memory->get()), pixfmt, cmptyp, cv_planar::CHUNKY, cv_memory::GPU);
		res != result::SUCCESS) {
		throw ::nvidia::ar::ar_error(res);
	}
}

void nvidia::ar::gstexture::update()
{
	nvidia::cuda::memcpy2d_t mc = {0};
	mc.src_x_in_bytes           = 0;
	mc.src_y                    = 0;
	mc.src_memory_type          = ::nvidia::cuda::memory_type::ARRAY;
	mc.src_host                 = nullptr;
	mc.src_device               = 0;
	mc.src_array                = _texture_cuda->map(_cobs->get_stream());
	mc.src_pitch                = _image.pitch;
	mc.dst_x_in_bytes           = 0;
	mc.dst_y                    = 0;
	mc.dst_memory_type          = ::nvidia::cuda::memory_type::DEVICE;
	mc.dst_host                 = nullptr;
	mc.dst_device               = reinterpret_cast<::nvidia::cuda::device_ptr_t>(_image.pixels);
	mc.dst_array                = nullptr;
	mc.dst_pitch                = _image.pitch;
	mc.width_in_bytes           = static_cast<size_t>(_image.pitch);
	mc.height                   = static_cast<size_t>(_image.height);

	// TODO: Evaluate cuMemcpy2DAsync vs cuMemcpy2D
	if (auto res = _cobs->get_cuda()->cuMemcpy2D(&mc); res != ::nvidia::cuda::result::SUCCESS) {
		throw ::nvidia::cuda::cuda_error(res);
	}
}

std::shared_ptr<gs::texture> nvidia::ar::gstexture::get_texture()
{
	return _texture;
}

std::shared_ptr<nvidia::cuda::gstexture> nvidia::ar::gstexture::get_cuda_texture()
{
	return _texture_cuda;
}

std::shared_ptr<nvidia::cuda::memory> nvidia::ar::gstexture::get_cuda_memory()
{
	return _memory;
}

nvidia::ar::cv_image& nvidia::ar::gstexture::get()
{
	return _image;
}
