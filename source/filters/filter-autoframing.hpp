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
#include "common.hpp"
#include "nvidia/ar/nvidia-ar-facedetection.hpp"
#include "nvidia/ar/nvidia-ar.hpp"
#include "nvidia/cuda/nvidia-cuda-obs.hpp"
#include "obs/gs/gs-rendertarget.hpp"
#include "obs/gs/gs-vertexbuffer.hpp"
#include "obs/obs-source-factory.hpp"

namespace streamfx::filter::autoframing {
	typedef std::chrono::high_resolution_clock::time_point hrc_tp;

	enum class engine {
		AUTOMATIC,
		NVIDIA,
	};

	struct rect {
		union {
			float v[4];
			struct {
				float x;
				float y;
				float w;
				float h;
			};
			struct {
				union {
					float r;
					float x;
				};
				union {
					float g;
					float y;
				};
				union {
					float b;
					float z;
				};
				union {
					float a;
					float w;
				};
			} vec4;
			struct {
				float left;
				float top;
				float right;
				float bottom;
			};
		};
	};

	class autoframing_instance : public obs::source_instance {
		std::pair<uint32_t, uint32_t>       _size;
		std::shared_ptr<gs::vertex_buffer>  _vb;
		std::shared_ptr<::gs::rendertarget> _capture;
		bool                                _capture_fresh;

		// Configuration
		bool                    _force_reinit;
		bool                    _track_groups;
		std::pair<float, float> _padding;
		std::pair<float, float> _offset;
		engine                  _engine;

		// Engines
		engine _last_engine;
		struct tnvidia {
			bool available;

			std::shared_ptr<::nvidia::cuda::obs>         _cobs;
			std::shared_ptr<::nvidia::ar::ar>            _ar;
			std::shared_ptr<::nvidia::ar::facedetection> _detector;
		} _nvidia;

		// Tracking, Framing
		std::vector<rect> _regions;
		rect              _frame;

		public:
		~autoframing_instance();
		autoframing_instance(obs_data_t* settings, obs_source_t* self);

		virtual void load(obs_data_t* data) override;

		virtual void migrate(obs_data_t* data, uint64_t version) override;

		virtual void update(obs_data_t* data) override;

		virtual void video_tick(float_t seconds) override;

		virtual void video_render(gs_effect_t* effect) override;

		private:
		/** Reinitialize the entire tracking and framing process.
		 *
		 */
		void reinitialize();

		void perform();

		void nvidia_load();
		void nvidia_create();
		void nvidia_track();
		void nvidia_destroy();
		void nvidia_unload();
	};

	class autoframing_factory : public obs::source_factory<streamfx::filter::autoframing::autoframing_factory,
														   streamfx::filter::autoframing::autoframing_instance> {
		struct nvidia {
			std::shared_ptr<::nvidia::cuda::cuda> _cuda;
			std::shared_ptr<::nvidia::ar::ar>     _ar;
		} _nvidia;

		public:
		autoframing_factory();
		virtual ~autoframing_factory() override;

		virtual const char* get_name() override;

		virtual void get_defaults2(obs_data_t* data) override;

		virtual obs_properties_t* get_properties2(streamfx::filter::autoframing::autoframing_instance* data) override;

		inline bool have_nvidia()
		{
			return _nvidia._cuda != nullptr;
		}

		inline std::shared_ptr<::nvidia::cuda::cuda> get_cuda()
		{
			return _nvidia._cuda;
		}

		inline std::shared_ptr<::nvidia::ar::ar> get_ar()
		{
			return _nvidia._ar;
		}

		private:
		void load_nvidia();

		public: // Singleton
		static void initialize();

		static void finalize();

		static std::shared_ptr<autoframing_factory> get();
	};
} // namespace streamfx::filter::autoframing
