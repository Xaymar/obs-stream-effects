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

#include "filter-autoframing.hpp"
#include "obs/gs/gs-helper.hpp"
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<Auto-Framing (Filter)> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

// Tracking
// - Mode
//   - "Solo": Track only a single person, but drastically improve temporal coherency.
//   - "Group": Track up to 8 people, but make no guarantee about which boundary is which face.
//     - May need to fix the random aspect of it in the future.
// - Stability: Slider from 0% to 100%.
//   - 0%: Smoothing, with over and undershoot.
//   - 100%: Pinpoint accuracy, always on the face.

// Framing
// - Modes
//   - Mode 0: Combine all tracked frames into a single large one.
//   - Mode 1: Keep individual frames per person, place them in some sort of predefined pattern (selectable).
//   - Mode 2: Frame each one individually, and if frames would overlap, merge their cells in the selectable pattern into one large one.
//     - Animate the merge, or having it be instant?
// - Padding in X and Y, either as % of total, or as px.

/* options
Tracking
- Mode
  - Solo: Track a single person only.
  - Group: Track up to 8 people.
- Stability: 0 - 100% Slider
  - 0%: Predict and smooth changes as much as possible.
  - 100%: No smoothing, always spot on tracking.


Advanced
- Engine
  - Automatic (whichever is available)
  - NVIDIA BROADCAST

*/

#define ST_I18N "Filter.AutoFraming"

#define ST_KEY_TRACKING "Tracking"
#define ST_I18N_TRACKING ST_I18N ".Tracking"
#define ST_KEY_TRACKING_MODE ST_KEY_TRACKING ".Mode"
#define ST_I18N_TRACKING_MODE ST_I18N_TRACKING ".Mode"
#define ST_I18N_TRACKING_MODE_SOLO ST_I18N_TRACKING_MODE ".Solo"
#define ST_I18N_TRACKING_MODE_GROUP ST_I18N_TRACKING_MODE ".Group"
#define ST_KEY_TRACKING_STABILITY ST_KEY_TRACKING ".Stability"
#define ST_I18N_TRACKING_STABILITY ST_I18N_TRACKING ".Stability"

#define ST_KEY_LAYOUT "Layout"
#define ST_I18N_LAYOUT ST_I18N ".Layout"
#define ST_KEY_LAYOUT_PADDING ST_KEY_LAYOUT ".Padding"
#define ST_I18N_LAYOUT_PADDING ST_I18N_LAYOUT ".Padding"
#define ST_KEY_LAYOUT_PADDING_X ST_KEY_LAYOUT_PADDING ".X"
#define ST_I18N_LAYOUT_PADDING_X ST_I18N_LAYOUT_PADDING ".X"
#define ST_KEY_LAYOUT_PADDING_Y ST_KEY_LAYOUT_PADDING ".Y"
#define ST_I18N_LAYOUT_PADDING_Y ST_I18N_LAYOUT_PADDING ".Y"
#define ST_KEY_LAYOUT_OFFSET ST_KEY_LAYOUT ".Offset"
#define ST_I18N_LAYOUT_OFFSET ST_I18N_LAYOUT ".Offset"
#define ST_KEY_LAYOUT_OFFSET_X ST_KEY_LAYOUT_OFFSET ".X"
#define ST_I18N_LAYOUT_OFFSET_X ST_I18N_LAYOUT_OFFSET ".X"
#define ST_KEY_LAYOUT_OFFSET_Y ST_KEY_LAYOUT_OFFSET ".Y"
#define ST_I18N_LAYOUT_OFFSET_Y ST_I18N_LAYOUT_OFFSET ".Y"

#define ST_KEY_ADVANCED "Advanced"
#define ST_I18N_ADVANCED ST_I18N ".Advanced"
#define ST_KEY_ADVANCED_ENGINE ST_KEY_ADVANCED ".Engine"
#define ST_I18N_ADVANCED_ENGINE ST_I18N_ADVANCED ".Engine"
#define ST_I18N_ADVANCED_ENGINE_AUTOMATIC D_TRANSLATE(S_STATE_AUTOMATIC)
#define ST_I18N_ADVANCED_ENGINE_NVIDIA "NVIDIA Broadcast"

#define ST_MAXIMUM_REGIONS 8

using namespace streamfx::filter::autoframing;

autoframing_instance::~autoframing_instance()
{
	D_LOG_DEBUG("Finalizing... (Addr: 0x%" PRIuPTR ")", this);

	auto gctx = gs::context();

	// Unload Engines
	nvidia_unload();

	_capture.reset();
}

autoframing_instance::autoframing_instance(obs_data_t* settings, obs_source_t* self)
	: source_instance(settings, self), _size(), _vb(), _capture(),

	  _force_reinit(true), _track_groups(false), _padding(), _offset(), _engine(engine::AUTOMATIC),

	  _last_engine(engine::AUTOMATIC), _nvidia(),

	  _regions(ST_MAXIMUM_REGIONS, rect()), _frame()
{
	D_LOG_DEBUG("Initializating... (Addr: 0x%" PRIuPTR ")", this);

	auto gctx = gs::context();
	_capture  = std::make_shared<::gs::rendertarget>(GS_RGBA, GS_ZS_NONE);
	_vb       = std::make_shared<gs::vertex_buffer>(4, 1);

	{
		vec3_set(_vb->at(0).position, 0, 0, 0);
		vec3_set(_vb->at(1).position, 1, 0, 0);
		vec3_set(_vb->at(2).position, 0, 1, 0);
		vec3_set(_vb->at(3).position, 1, 1, 0);
		_vb->update(true);
	}

	// Load Engines
	nvidia_load();

	// Update from passed data.
	update(settings);
}

void autoframing_instance::load(obs_data_t* data)
{
	// Update from passed data.
	update(data);
}

void autoframing_instance::migrate(obs_data_t* data, uint64_t version)
{
	switch (version) {
	case STREAMFX_MAKE_VERSION(0, 10, 0, 0): {
		{ // Change engine setting back to automatic if loading of a specified engine failed.
			if (!autoframing_factory::get()->have_nvidia()
				&& (static_cast<engine>(obs_data_get_int(data, ST_KEY_ADVANCED_ENGINE)) == engine::NVIDIA)) {
				obs_data_set_int(data, ST_KEY_ADVANCED_ENGINE, static_cast<int64_t>(engine::AUTOMATIC));
			}
		}
	}
	}
}

inline std::pair<bool, double_t> parse_text_as_size(const char* text)
{
	double_t v = 0;
	if (sscanf(text, "%lf", &v) == 1) {
		const char* prc_chr = strrchr(text, '%');
		if (prc_chr && (*prc_chr == '%')) {
			return {true, v / 100.0};
		} else {
			return {false, v};
		}
	} else {
		return {true, 1.0};
	}
}

void autoframing_instance::update(obs_data_t* data)
{
	bool reinit = _force_reinit;

	{ // Tracking > Mode
		bool v        = (obs_data_get_int(data, ST_KEY_TRACKING_MODE) != 0);
		reinit        = reinit || (_track_groups != v);
		_track_groups = v;
	}

	{ // Layout > Padding
		auto x = parse_text_as_size(obs_data_get_string(data, ST_KEY_LAYOUT_PADDING_X));
		auto y = parse_text_as_size(obs_data_get_string(data, ST_KEY_LAYOUT_PADDING_Y));
		if (x.first) {
			_padding.first = x.second * _size.first;
		} else {
			_padding.first = x.second;
		}
		if (y.first) {
			_padding.second = y.second * _size.second;
		} else {
			_padding.second = y.second;
		}
	}
	{ // Layout > Offset
		auto x = parse_text_as_size(obs_data_get_string(data, ST_KEY_LAYOUT_OFFSET_X));
		auto y = parse_text_as_size(obs_data_get_string(data, ST_KEY_LAYOUT_OFFSET_Y));
		if (x.first) {
			_offset.first = x.second * _size.first;
		} else {
			_offset.first = x.second;
		}
		if (y.first) {
			_offset.second = y.second * _size.second;
		} else {
			_offset.second = y.second;
		}
	}

	{ // Advanced > Engine
		engine v = static_cast<engine>(obs_data_get_int(data, ST_KEY_ADVANCED_ENGINE));
		reinit   = reinit || (_engine != v);
		_engine  = v;
	}

	if (reinit) {
		reinitialize();
		_force_reinit = false;
	}
}

void autoframing_instance::video_tick(float_t seconds)
{
	// Update the input size.
	if (obs_source_t* src = obs_filter_get_target(_self); src != nullptr) {
		_size.first  = obs_source_get_width(src);
		_size.second = obs_source_get_height(src);
	} else {
		_size.first = _size.second = 1;
	}

	_capture_fresh = false;
}

void autoframing_instance::video_render(gs_effect_t* effect)
{ // Already in a graphics context here.
	gs_effect_t* default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_effect_t* input_effect   = effect ? effect : default_effect;

	if (!_capture_fresh) { // Only re-capture if we ticked before.
		if (obs_source_process_filter_begin(_self, _capture->get_color_format(), OBS_ALLOW_DIRECT_RENDERING)) {
			auto op  = _capture->render(_size.first, _size.second);
			vec4 clr = {0., 0., 0., 0.};

			gs_ortho(0., _size.first, 0., _size.second, -1., 1.);
			gs_clear(GS_CLEAR_COLOR, &clr, 0., 0);
			gs_enable_color(true, true, true, true);
			gs_enable_blending(false);

			obs_source_process_filter_tech_end(_self, input_effect, _size.first, _size.second, "Draw");
		} else {
			obs_source_skip_video_filter(_self);
			return;
		}

		// Update Tracking
		perform();

		_capture_fresh = true;
	}

	{ // Adjust UVs
		{
			auto v     = _vb->at(0);
			v.uv[0]->x = _frame.x / _size.first;
			v.uv[0]->y = _frame.y / _size.second;
		}
		{
			auto v     = _vb->at(1);
			v.uv[0]->x = (_frame.x + _frame.w) / _size.first;
			v.uv[0]->y = _frame.y / _size.second;
		}
		{
			auto v     = _vb->at(2);
			v.uv[0]->x = _frame.x / _size.first;
			v.uv[0]->y = (_frame.y + _frame.h) / _size.second;
		}
		{
			auto v     = _vb->at(3);
			v.uv[0]->x = (_frame.x + _frame.w) / _size.first;
			v.uv[0]->y = (_frame.y + _frame.h) / _size.second;
		}
	}
	gs_load_vertexbuffer(_vb->update(true));

	gs_matrix_push();
	gs_matrix_scale3f(_size.first, _size.second, 0);

	gs_effect_set_texture(gs_effect_get_param_by_name(default_effect, "image"), _capture->get_texture()->get_object());
	while (gs_effect_loop(default_effect, "Draw")) {
		gs_draw(GS_TRISTRIP, 0, _vb->size());
	}

	gs_matrix_pop();
	gs_load_vertexbuffer(nullptr);
}

void autoframing_instance::reinitialize()
{
	// This function takes care of the following:
	// 1. Changing which engine is active and destroy instances of inactive ones.
	// 2. Reinitializing the currently active engine for tracking in a different way.
	auto gctx = gs::context();

	// If we switched engines, destroy any possibly existing instances.
	if (_last_engine != _engine) {
		nvidia_destroy();
	}

	// If the engine is still the same, just reinitialize it.
	std::vector<engine> engines;
	if (_nvidia.available && ((_engine == engine::AUTOMATIC) || (_engine == engine::NVIDIA)))
		engines.push_back(engine::NVIDIA);

	bool have_engine = false;
	for (engine e : engines) {
		if (e == engine::NVIDIA) {
			try {
				nvidia_create();
				have_engine = true;
				break;
			} catch (std::exception const& ex) {
				DLOG_WARNING(
					"Failed to reinitialize NVIDIA engine with error: %s. Falling back to another engine if available.",
					ex.what());
				nvidia_destroy();
			}
		}
	}
	if (!have_engine) {
		DLOG_ERROR("No engines available to perform tracking.");
	}

	_last_engine = _engine;
}

void autoframing_instance::perform()
{
	{ // Track with any available engine.
		if (_nvidia._detector)
			nvidia_track();
	}

	if (_regions.size() > 0) { // Framing - Find the largest frame that encompasses all regions
		//
		rect boundary;
		boundary.left   = std::numeric_limits<float>::infinity();
		boundary.top    = std::numeric_limits<float>::infinity();
		boundary.right  = std::numeric_limits<float>::infinity();
		boundary.bottom = std::numeric_limits<float>::infinity();

		for (rect& region : _regions) {
			if (region.left < boundary.left)
				boundary.left = region.left;
			if (region.top < boundary.top)
				boundary.top = region.top;
			if (auto v = _size.first - (region.left + region.w); v < boundary.right)
				boundary.right = v;
			if (auto v = _size.second - (region.top + region.h); v < boundary.bottom)
				boundary.bottom = v;
		}

		// Padding
		boundary.left -= _padding.first;
		boundary.right -= _padding.first;
		boundary.top -= _padding.second;
		boundary.bottom -= _padding.second;

		// Convert to frame.
		rect frame;
		frame.x = boundary.left;
		frame.y = boundary.top;
		frame.w = (_size.first - boundary.right) - boundary.left;
		frame.h = (_size.second - boundary.bottom) - boundary.top;

		// Try and fit more of the actual face into the frame by moving it up 1/30th of the
		// detected frame height. This should place the center of it between the eyes, instead
		// of below it.
		frame.y -= frame.h * (1. / 30.);

		// Offset
		frame.x += _offset.first;
		frame.y -= _offset.first;

		// Fix Aspect Ratio
		float aspect = static_cast<float>(_size.first) / static_cast<float>(_size.second);
		float cx     = frame.x + frame.w / 2.;
		float tw     = frame.h * aspect;
		frame.x      = cx - tw / 2.;
		frame.w      = tw;

		_frame = frame;
	} else {
		_frame.x = 0;
		_frame.y = 0;
		_frame.w = _size.first;
		_frame.h = _size.second;
	}
}

void autoframing_instance::nvidia_load()
{
	if (!autoframing_factory::get()->have_nvidia()) {
		_nvidia.available = false;
		return;
	}

	try {
		if (!_nvidia._cobs)
			_nvidia._cobs = nvidia::cuda::obs::get();
		if (!_nvidia._ar)
			_nvidia._ar = nvidia::ar::ar::get();

		_nvidia.available = true;
	} catch (...) {
		nvidia_unload();
	}
}

void autoframing_instance::nvidia_create()
{
	if (!_nvidia.available)
		throw std::runtime_error("NVIDIA is not available");

	auto cctx         = _nvidia._cobs->get_context()->enter();
	_nvidia._detector = std::make_shared<::nvidia::ar::facedetection>();
	_nvidia._detector->enable_temporal_tracking(!_track_groups);
	_nvidia._detector->set_limit(_regions.capacity());
	_nvidia._detector->load();
}

void autoframing_instance::nvidia_track()
{
	static const float threshold = 0.5;

	auto cctx = _nvidia._cobs->get_context()->enter();
	_nvidia._detector->track(_capture->get_texture());

	// Copy any tracked regions to the internal state of the filter.
	float  confidence = 0;
	size_t ridx       = 0;
	_regions.resize(ST_MAXIMUM_REGIONS);
	for (size_t idx = 0; idx < _nvidia._detector->count(); idx++) {
		auto& rc = _nvidia._detector->at(idx, confidence);
		// Only copy the detected region if we are confident that it is a face.
		if (confidence > threshold) {
			auto& ref = _regions.at(ridx);
			ref.x     = rc.x;
			ref.y     = rc.y;
			ref.w     = rc.z;
			ref.h     = rc.w;
			ridx++;
		}
	}
	_regions.resize(ridx);
}

void autoframing_instance::nvidia_destroy()
{
	if (!_nvidia._detector) {
		return;
	}

	auto cctx = _nvidia._cobs->get_context()->enter();
	_nvidia._cobs->get_context()->synchronize();
	_nvidia._cobs->get_stream()->synchronize();
	_nvidia._detector.reset();
}

void autoframing_instance::nvidia_unload()
{
	nvidia_destroy();
	_nvidia._ar.reset();
	_nvidia._cobs.reset();
	_nvidia.available = false;
}

autoframing_factory::autoframing_factory()
{
	// Load any available engines.
	load_nvidia();

	// Check if any of the engines is available.
	if (!have_nvidia()) {
		DLOG_ERROR("No available face tracking engines, feature unavailable.");
		return;
	}

	// Register initial source.
	_info.id           = PREFIX "filter-autoframing";
	_info.type         = OBS_SOURCE_TYPE_FILTER;
	_info.output_flags = OBS_SOURCE_VIDEO;
	set_resolution_enabled(false);
	finish_setup();

	// Register proxy identifiers.
	register_proxy("streamfx-filter-nvidia-face-tracking");
	register_proxy("streamfx-nvidia-face-tracking");
}

autoframing_factory::~autoframing_factory() {}

const char* autoframing_factory::get_name()
{
	return D_TRANSLATE(ST_I18N);
}

void autoframing_factory::get_defaults2(obs_data_t* data)
{
	// Tracking
	obs_data_set_default_int(data, ST_KEY_TRACKING_MODE, static_cast<int64_t>(0));

	// Layout
	obs_data_set_default_string(data, ST_KEY_LAYOUT_PADDING_X, "10.0 %");
	obs_data_set_default_string(data, ST_KEY_LAYOUT_PADDING_Y, "10.0 %");
	obs_data_set_default_string(data, ST_KEY_LAYOUT_OFFSET_X, "0.0 %");
	obs_data_set_default_string(data, ST_KEY_LAYOUT_OFFSET_Y, "-5.0 %");

	// Advanced
	obs_data_set_default_int(data, ST_KEY_ADVANCED_ENGINE, static_cast<int64_t>(engine::AUTOMATIC));
}

obs_properties_t* autoframing_factory::get_properties2(autoframing_instance* data)
{
	obs_properties_t* props = obs_properties_create();

	{ // Tracking
		auto grp = obs_properties_create();
		obs_properties_add_group(props, ST_KEY_TRACKING, D_TRANSLATE(ST_I18N_TRACKING), OBS_GROUP_NORMAL, grp);

		{
			auto p = obs_properties_add_list(grp, ST_KEY_TRACKING_MODE, D_TRANSLATE(ST_I18N_TRACKING_MODE),
											 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, D_TRANSLATE(D_DESC(ST_I18N_TRACKING_MODE)));

			obs_property_list_add_int(p, D_TRANSLATE(ST_I18N_TRACKING_MODE_SOLO), 0);
			obs_property_list_add_int(p, D_TRANSLATE(ST_I18N_TRACKING_MODE_GROUP), 1);
		}
	}

	{ // Layout
		auto grp = obs_properties_create();
		obs_properties_add_group(props, ST_KEY_LAYOUT, D_TRANSLATE(ST_I18N_LAYOUT), OBS_GROUP_NORMAL, grp);

		{ // Padding
			auto grp2 = obs_properties_create();
			obs_properties_add_group(grp, ST_KEY_LAYOUT_PADDING, D_TRANSLATE(ST_I18N_LAYOUT_PADDING), OBS_GROUP_NORMAL,
									 grp2);

			{
				auto p = obs_properties_add_text(grp2, ST_KEY_LAYOUT_PADDING_X, D_TRANSLATE(ST_I18N_LAYOUT_PADDING_X),
												 OBS_TEXT_DEFAULT);
				obs_property_set_long_description(p, D_TRANSLATE(D_DESC(ST_I18N_LAYOUT_PADDING)));
			}
			{
				auto p = obs_properties_add_text(grp2, ST_KEY_LAYOUT_PADDING_Y, D_TRANSLATE(ST_I18N_LAYOUT_PADDING_Y),
												 OBS_TEXT_DEFAULT);
				obs_property_set_long_description(p, D_TRANSLATE(D_DESC(ST_I18N_LAYOUT_PADDING)));
			}
		}

		{ // Offset
			auto grp2 = obs_properties_create();
			obs_properties_add_group(grp, ST_KEY_LAYOUT_OFFSET, D_TRANSLATE(ST_I18N_LAYOUT_OFFSET), OBS_GROUP_NORMAL,
									 grp2);

			{
				auto p = obs_properties_add_text(grp2, ST_KEY_LAYOUT_OFFSET_X, D_TRANSLATE(ST_I18N_LAYOUT_OFFSET_X),
												 OBS_TEXT_DEFAULT);
				obs_property_set_long_description(p, D_TRANSLATE(D_DESC(ST_I18N_LAYOUT_OFFSET)));
			}
			{
				auto p = obs_properties_add_text(grp2, ST_KEY_LAYOUT_OFFSET_Y, D_TRANSLATE(ST_I18N_LAYOUT_OFFSET_Y),
												 OBS_TEXT_DEFAULT);
				obs_property_set_long_description(p, D_TRANSLATE(D_DESC(ST_I18N_LAYOUT_OFFSET)));
			}
		}
	}

	{ // Advanced
		auto grp = obs_properties_create();
		obs_properties_add_group(props, ST_KEY_ADVANCED, D_TRANSLATE(ST_I18N_ADVANCED), OBS_GROUP_NORMAL, grp);

		{
			auto p = obs_properties_add_list(grp, ST_KEY_ADVANCED_ENGINE, D_TRANSLATE(ST_I18N_ADVANCED_ENGINE),
											 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, D_TRANSLATE(D_DESC(ST_I18N_ADVANCED_ENGINE)));

			obs_property_list_add_int(p, ST_I18N_ADVANCED_ENGINE_AUTOMATIC, static_cast<int64_t>(engine::AUTOMATIC));
			if (have_nvidia())
				obs_property_list_add_int(p, ST_I18N_ADVANCED_ENGINE_NVIDIA, static_cast<int64_t>(engine::NVIDIA));
		}
	}

	return props;
}

void streamfx::filter::autoframing::autoframing_factory::load_nvidia()
{
	try {
		_nvidia._cuda = ::nvidia::cuda::cuda::get();
		_nvidia._ar   = ::nvidia::ar::ar::get();
	} catch (...) {
		_nvidia._ar.reset();
		_nvidia._cuda.reset();
	}
}

//----------------------------------------------------------------------------//
// Singleton
//----------------------------------------------------------------------------//
std::shared_ptr<autoframing_factory> _filter_nvidia_face_tracking_factory_instance = nullptr;

void autoframing_factory::initialize()
{
	try {
		_filter_nvidia_face_tracking_factory_instance = std::make_shared<autoframing_factory>();
	} catch (...) {
		// Silently eat exception and treat it as if loading failed.
	}
}

void autoframing_factory::finalize()
{
	_filter_nvidia_face_tracking_factory_instance.reset();
}

std::shared_ptr<autoframing_factory> autoframing_factory::get()
{
	return _filter_nvidia_face_tracking_factory_instance;
}
