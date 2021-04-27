/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017 Michael Fabian Dirks
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

#include "plugin.hpp"
#include <fstream>
#include <stdexcept>
#include "configuration.hpp"
#include "obs/gs/gs-vertexbuffer.hpp"
#include "obs/obs-source-tracker.hpp"

#ifdef ENABLE_ENCODER_FFMPEG
#include "encoders/encoder-ffmpeg.hpp"
#endif

#ifdef ENABLE_FILTER_AUTOFRAMING
#include "filters/filter-autoframing.hpp"
#endif
#ifdef ENABLE_FILTER_BLUR
#include "filters/filter-blur.hpp"
#endif
#ifdef ENABLE_FILTER_COLOR_GRADE
#include "filters/filter-color-grade.hpp"
#endif
#ifdef ENABLE_FILTER_DISPLACEMENT
#include "filters/filter-displacement.hpp"
#endif
#ifdef ENABLE_FILTER_DYNAMIC_MASK
#include "filters/filter-dynamic-mask.hpp"
#endif
#ifdef ENABLE_FILTER_SDF_EFFECTS
#include "filters/filter-sdf-effects.hpp"
#endif
#ifdef ENABLE_FILTER_SHADER
#include "filters/filter-shader.hpp"
#endif
#ifdef ENABLE_FILTER_TRANSFORM
#include "filters/filter-transform.hpp"
#endif

#ifdef ENABLE_SOURCE_MIRROR
#include "sources/source-mirror.hpp"
#endif
#ifdef ENABLE_SOURCE_SHADER
#include "sources/source-shader.hpp"
#endif

#ifdef ENABLE_TRANSITION_SHADER
#include "transitions/transition-shader.hpp"
#endif

#ifdef ENABLE_FRONTEND
#include "ui/ui.hpp"
#endif

#ifdef ENABLE_UPDATER
#include "updater.hpp"
//static std::shared_ptr<streamfx::updater> _updater;
#endif

static std::shared_ptr<util::threadpool>  _threadpool;
static std::shared_ptr<gs::vertex_buffer> _gs_fstri_vb;

MODULE_EXPORT bool obs_module_load(void)
try {
	DLOG_INFO("Loading Version %s", STREAMFX_VERSION_STRING);

	// Initialize global configuration.
	streamfx::configuration::initialize();

	// Initialize global Thread Pool.
	_threadpool = std::make_shared<util::threadpool>();

	// Initialize Source Tracker
	obs::source_tracker::initialize();

	// GS Stuff
	{
		_gs_fstri_vb = std::make_shared<gs::vertex_buffer>(uint32_t(3), uint8_t(1));
		{
			auto vtx = _gs_fstri_vb->at(0);
			vec3_set(vtx.position, 0, 0, 0);
			vec4_set(vtx.uv[0], 0, 0, 0, 0);
		}
		{
			auto vtx = _gs_fstri_vb->at(1);
			vec3_set(vtx.position, 2, 0, 0);
			vec4_set(vtx.uv[0], 2, 0, 0, 0);
		}
		{
			auto vtx = _gs_fstri_vb->at(2);
			vec3_set(vtx.position, 0, 2, 0);
			vec4_set(vtx.uv[0], 0, 2, 0, 0);
		}
		_gs_fstri_vb->update();
	}

	// Encoders
	{
#ifdef ENABLE_ENCODER_FFMPEG
		using namespace streamfx::encoder::ffmpeg;
		ffmpeg_manager::initialize();
#endif
	}

	// Filters
	{
#ifdef ENABLE_FILTER_AUTOFRAMING
		streamfx::filter::autoframing::autoframing_factory::initialize();
#endif
#ifdef ENABLE_FILTER_BLUR
		streamfx::filter::blur::blur_factory::initialize();
#endif
#ifdef ENABLE_FILTER_COLOR_GRADE
		streamfx::filter::color_grade::color_grade_factory::initialize();
#endif
#ifdef ENABLE_FILTER_DISPLACEMENT
		streamfx::filter::displacement::displacement_factory::initialize();
#endif
#ifdef ENABLE_FILTER_DYNAMIC_MASK
		streamfx::filter::dynamic_mask::dynamic_mask_factory::initialize();
#endif
#ifdef ENABLE_FILTER_SDF_EFFECTS
		streamfx::filter::sdf_effects::sdf_effects_factory::initialize();
#endif
#ifdef ENABLE_FILTER_SHADER
		streamfx::filter::shader::shader_factory::initialize();
#endif
#ifdef ENABLE_FILTER_TRANSFORM
		streamfx::filter::transform::transform_factory::initialize();
#endif
	}

	// Sources
	{
#ifdef ENABLE_SOURCE_MIRROR
		streamfx::source::mirror::mirror_factory::initialize();
#endif
#ifdef ENABLE_SOURCE_SHADER
		streamfx::source::shader::shader_factory::initialize();
#endif
	}

	// Transitions
	{
#ifdef ENABLE_TRANSITION_SHADER
		streamfx::transition::shader::shader_factory::initialize();
#endif
	}

// Frontend
#ifdef ENABLE_FRONTEND
	streamfx::ui::handler::initialize();
#endif

	DLOG_INFO("Loaded Version %s", STREAMFX_VERSION_STRING);
	return true;
} catch (...) {
	DLOG_ERROR("Unexpected exception in function '%s'.", __FUNCTION_NAME__);
	return false;
}

MODULE_EXPORT void obs_module_unload(void)
try {
	DLOG_INFO("Unloading Version %s", STREAMFX_VERSION_STRING);

	// Frontend
#ifdef ENABLE_FRONTEND
	streamfx::ui::handler::finalize();
#endif

	// Transitions
	{
#ifdef ENABLE_TRANSITION_SHADER
		streamfx::transition::shader::shader_factory::finalize();
#endif
	}

	// Sources
	{
#ifdef ENABLE_SOURCE_MIRROR
		streamfx::source::mirror::mirror_factory::finalize();
#endif
#ifdef ENABLE_SOURCE_SHADER
		streamfx::source::shader::shader_factory::finalize();
#endif
	}

	// Filters
	{
#ifdef ENABLE_FILTER_BLUR
		streamfx::filter::blur::blur_factory::finalize();
#endif
#ifdef ENABLE_FILTER_COLOR_GRADE
		streamfx::filter::color_grade::color_grade_factory::finalize();
#endif
#ifdef ENABLE_FILTER_DISPLACEMENT
		streamfx::filter::displacement::displacement_factory::finalize();
#endif
#ifdef ENABLE_FILTER_DYNAMIC_MASK
		streamfx::filter::dynamic_mask::dynamic_mask_factory::finalize();
#endif
#ifdef ENABLE_FILTER_SDF_EFFECTS
		streamfx::filter::sdf_effects::sdf_effects_factory::finalize();
#endif
#ifdef ENABLE_FILTER_SHADER
		streamfx::filter::shader::shader_factory::finalize();
#endif
#ifdef ENABLE_FILTER_TRANSFORM
		streamfx::filter::transform::transform_factory::finalize();
#endif
#ifdef ENABLE_FILTER_AUTOFRAMING
		streamfx::filter::autoframing::autoframing_factory::finalize();
#endif
	}

	// Encoders
	{
#ifdef ENABLE_ENCODER_FFMPEG
		streamfx::encoder::ffmpeg::ffmpeg_manager::finalize();
#endif
	}

	// GS Stuff
	{
		_gs_fstri_vb.reset();
	}

	// Finalize Source Tracker
	obs::source_tracker::finalize();

	//	// Auto-Updater
	//#ifdef ENABLE_UPDATER
	//	_updater.reset();
	//#endif

	// Finalize Thread Pool
	_threadpool.reset();

	// Finalize Configuration
	streamfx::configuration::finalize();

	DLOG_INFO("Unloaded Version %s", STREAMFX_VERSION_STRING);
} catch (...) {
	DLOG_ERROR("Unexpected exception in function '%s'.", __FUNCTION_NAME__);
}

std::shared_ptr<util::threadpool> streamfx::threadpool()
{
	return _threadpool;
}

void streamfx::gs_draw_fullscreen_tri()
{
	gs_load_vertexbuffer(_gs_fstri_vb->update(false));
	gs_draw(GS_TRIS, 0, 3); //_gs_fstri_vb->size());
}

std::filesystem::path streamfx::data_file_path(std::string_view file)
{
	const char* root_path = obs_get_module_data_path(obs_current_module());
	if (root_path) {
		auto ret = std::filesystem::u8path(root_path);
		ret.append(file.data());
		return ret;
	} else {
		throw std::runtime_error("obs_get_module_data_path returned nullptr");
	}
}

std::filesystem::path streamfx::config_file_path(std::string_view file)
{
	char* root_path = obs_module_get_config_path(obs_current_module(), file.data());
	if (root_path) {
		auto ret = std::filesystem::u8path(root_path);
		bfree(root_path);
		return ret;
	} else {
		throw std::runtime_error("obs_module_get_config_path returned nullptr");
	}
}

#ifdef ENABLE_FRONTEND
bool streamfx::open_url(std::string_view url)
{
	QUrl qurl = QString::fromUtf8(url.data());
	return QDesktopServices::openUrl(qurl);
}
#endif
