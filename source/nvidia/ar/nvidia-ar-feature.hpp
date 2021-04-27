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
#include <string_view>
#include "nvidia-ar.hpp"
#include "nvidia/cuda/nvidia-cuda-stream.hpp"

namespace nvidia::ar {
	class feature {
		protected:
		std::shared_ptr<::nvidia::ar::ar> _ar;
		::nvidia::ar::feature_t           _feature;

		public:
		~feature();
		feature(feature_id_t feature);

		::nvidia::ar::feature_t get()
		{
			return _feature;
		}

		inline void load()
		{
			if (auto res = _ar->AR_Load(_feature); res != result::SUCCESS)
				throw ar_error(res);
		}

		inline void run()
		{
			if (auto res = _ar->AR_Run(_feature); res != result::SUCCESS)
				throw ar_error(res);
		}

		public:
		inline void set(std::string_view name, int32_t value)
		{
			if (auto res = _ar->AR_SetS32(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void get(std::string_view name, int32_t* value)
		{
			if (auto res = _ar->AR_GetS32(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void set(std::string_view name, uint32_t value)
		{
			if (auto res = _ar->AR_SetU32(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void get(std::string_view name, uint32_t* value)
		{
			if (auto res = _ar->AR_GetU32(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void set(std::string_view name, uint64_t value)
		{
			if (auto res = _ar->AR_SetU64(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void get(std::string_view name, uint64_t* value)
		{
			if (auto res = _ar->AR_GetU64(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void set(std::string_view name, float value)
		{
			if (auto res = _ar->AR_SetF32(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void get(std::string_view name, float* value)
		{
			if (auto res = _ar->AR_GetF32(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void set(std::string_view name, double value)
		{
			if (auto res = _ar->AR_SetF64(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void get(std::string_view name, double* value)
		{
			if (auto res = _ar->AR_GetF64(_feature, name.data(), value); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void set(std::string_view name, std::vector<float>& value)
		{
			if (auto res = _ar->AR_SetF32Array(_feature, name.data(), value.data(), static_cast<int32_t>(value.size()));
				res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void get(std::string_view name, std::vector<float>& value)
		{
			const float* data;
			int32_t      size;

			if (auto res = _ar->AR_GetF32Array(_feature, name.data(), &data, &size); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);

			value.resize(size);
			memcpy(value.data(), data, size * sizeof(float));
		}

		inline void set(std::string_view name, std::string_view value)
		{
			if (auto res = _ar->AR_SetString(_feature, name.data(), value.data()); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}

		inline void get(std::string_view name, std::string& value)
		{
			const char* data;
			if (auto res = _ar->AR_GetString(_feature, name.data(), &data); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);

			if (data != nullptr)
				value = std::string(data);
			else
				value.clear();
		}

		inline void set(std::string_view name, std::shared_ptr<::nvidia::cuda::stream> value)
		{
			if (auto res = _ar->AR_SetCudaStream(_feature, name.data(), value->get()); res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}
		//void get(...);

		inline void set(std::string_view name, void* data, size_t data_size)
		{
			if (auto res = _ar->AR_SetObject(_feature, name.data(), data, static_cast<uint32_t>(data_size));
				res != result::SUCCESS)
				throw nvidia::ar::ar_error(res);
		}
		//void get(std::string_view name, void** data, size_t data_size);
	};
} // namespace nvidia::ar
