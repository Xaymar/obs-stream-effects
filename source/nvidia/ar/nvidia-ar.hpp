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
#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include "nvidia/cuda/nvidia-cuda.hpp"
#include "util/util-library.hpp"

#define NVAR_DEFINE_FUNCTION(name, ...)                   \
	typedef ::nvidia::ar::result (*t##name)(__VA_ARGS__); \
	t##name name = nullptr;

#define NVAR_DEFINE_FUNCTION2(name, type, ...) \
	typedef type (*t##name)(__VA_ARGS__);      \
	t##name name = nullptr;

#define NVAR_FEATURE_FACEDETECTION "FaceDetection"
#define NVAR_FEATURE_LANDMARKDETECTION "LandmarkDetection"
#define NVAR_FEATURE_FACE3DRECONSTRUCTION "Face3DReconstruction"

#define NVAR_PARAMETER_INPUT(name) "NvAR_Parameter_Input_"##name
#define NVAR_PARAMETER_OUTPUT(name) "NvAR_Parameter_Output_"##name
#define NVAR_PARAMETER_CONFIG(name) "NvAR_Parameter_Config_"##name

namespace nvidia::ar {
	enum class result : int32_t {
		SUCCESS,
		ERROR_GENERIC              = -1,
		ERROR_NOT_IMPLEMENTED      = -2,
		ERROR_MEMORY               = -3,
		ERROR_EFFECT               = -4,
		ERROR_SELECTOR             = -5,
		ERROR_BUFFER               = -6,
		ERROR_PARAMETER            = -7,
		ERROR_MISMATCH             = -8,
		ERROR_PIXELFORMAT          = -9,
		ERROR_MODEL                = -10,
		ERROR_LIBRARY              = -11,
		ERROR_INITIALIZATION       = -12,
		ERROR_FILE                 = -13,
		ERROR_FEATURENOTFOUND      = -14,
		ERROR_MISSINGINPUT         = -15,
		ERROR_RESOLUTION           = -16,
		ERROR_UNSUPPORTEDGPU       = -17,
		ERROR_WRONGGPU             = -18,
		ERROR_UNSUPPORTEDDRIVER    = -19,
		ERROR_CUDA_MEMORY          = -20,
		ERROR_CUDA_VALUE           = -21,
		ERROR_CUDA_PITCH           = -22,
		ERROR_CUDA_INIT            = -23,
		ERROR_CUDE_LAUNCH          = -24,
		ERROR_CUDA_KERNEL          = -25,
		ERROR_CUDA_DRIVER          = -26,
		ERROR_CUDA_UNSUPPORTED     = -27,
		ERROR_CUDA_ILLEGAL_ADDRESS = -28,
		ERROR_CUDA                 = -30,
	};

	typedef void*       feature_t;
	typedef const char* feature_id_t;

	template<typename T>
	struct vec2 {
		T x;
		T y;
	};

	template<typename T>
	struct vec3 : public vec2<T> {
		T z;
	};

	template<typename T>
	struct vec4 : public vec3<T> {
		T w;
	};

	typedef vec2<float> point_t;
	typedef vec4<float> frustum_t;
	typedef vec4<float> quaternion_t;
	typedef vec4<float> rect_t;

	struct boundaries {
		rect_t* rects;
		uint8_t current;
		uint8_t maximum;
	};

	struct facemesh {
		vec3<float>*   vertices;
		size_t         num_vertices;
		vec3<uint16_t> indices;
		size_t         num_indices;
	};

	struct rendering_params {
		frustum_t    frustum;
		quaternion_t rotation;
		vec3<float>  translation;
	};

	enum class cv_pixel_format : uint32_t {
		UNKNOWN,
		Y,
		A,
		YA,
		RGB,
		BGR,
		RGBA,
		BGRA,
		YUV420,
		YUV422,
	};

	enum class cv_component_type : uint32_t {
		UNKNOWN,
		UINT8,
		UINT16,
		INT16,
		FLOAT16,
		UINT32,
		INT32,
		FLOAT32,
		UINT64,
		INT64,
		FLOAT64,
		DOUBLE = FLOAT64,
	};

	// [] denotes a single plane, each character being one component.
	enum class cv_planar : uint32_t {
		INTERLEAVED = 0, // [YUV]/[RGB]/[BGR]/[RGBA]/... 4:4:4
		PLANAR      = 1, // [Y][U][V]/[R][G][B]/... 4:4:4
		UYVY        = 2, // [UYVY] 4:2:2
		YUV         = 3, // [Y][U][V] 4:2:2/4:2:0
		VYUY        = 4, // [VYUY] 4:2:2
		YVU         = 5, // [Y][V][U] 4:2:2 or 4:2:0
		YUYV        = 6, // [YUYV] 4:2:2
		YCUV        = 7, // [Y][UV] 4:2:2 or 4:2:0
		YVYU        = 8, // [YVYU] 4:2:2
		YCVU        = 9, // [Y][VU] 4:2:2 or 4:2:0
		CHUNKY      = INTERLEAVED,
		YUY2        = YUYV,
		I420        = YUV,
		IYUV        = YUV,
		YV12        = YVU,
		NV12        = YCUV,
		NV21        = YCVU,
	};

	enum class cv_memory : uint32_t {
		CPU        = 0, // CPU Memory
		GPU        = 1, // GPU Memory
		CPU_PINNED = 2, // CPU Memory (Non-pageable, always mapped to the GPU)
	};

	struct cv_image {
		uint32_t          width;                  // 0,3
		uint32_t          height;                 // 4,7
		uint32_t          pitch;                  // 8,11
		cv_pixel_format   pixel_format;           // 12,15
		cv_component_type component_type;         // 16,19
		uint8_t           pixel_bytes;            // 20,20
		uint8_t           component_bytes;        // 21,21
		uint8_t           num_components;         // 22,22
		uint8_t           planar;                 // 23,23
		uint8_t           memory;                 // 24,24
		uint8_t           colorspace;             // 25,25
		uint8_t           batch;                  // 26,26
		void*             pixels;                 // 32,39
		void*             private_data;           // 40,47
		void (*private_data_deleter)(void* data); // 48,55
		uint64_t pixels_size;                     // 56,63
	};

	class ar_error : public std::exception {
		result _code;

		public:
		ar_error(result code);

		public:
		inline result code()
		{
			return _code;
		};
	};

	class ar {
		std::filesystem::path            _sdk_path;
		std::filesystem::path            _model_path;
		std::shared_ptr<::util::library> _library;

		public:
		~ar();
		ar();

		std::filesystem::path get_sdk_path();
		std::filesystem::path get_model_path();

		public:
		NVAR_DEFINE_FUNCTION(AR_GetVersion, uint32_t* version);

		NVAR_DEFINE_FUNCTION(AR_Create, feature_id_t feature_id, feature_t* ptr);
		NVAR_DEFINE_FUNCTION(AR_Destroy, feature_t ptr);
		NVAR_DEFINE_FUNCTION(AR_Run, feature_t ptr);
		NVAR_DEFINE_FUNCTION(AR_Load, feature_t ptr);

		NVAR_DEFINE_FUNCTION(AR_GetS32, feature_t ptr, const char* key, int32_t* value);
		NVAR_DEFINE_FUNCTION(AR_SetS32, feature_t ptr, const char* key, int32_t value);
		NVAR_DEFINE_FUNCTION(AR_GetU32, feature_t ptr, const char* key, uint32_t* value);
		NVAR_DEFINE_FUNCTION(AR_SetU32, feature_t ptr, const char* key, uint32_t value);
		NVAR_DEFINE_FUNCTION(AR_GetU64, feature_t ptr, const char* key, uint64_t* value);
		NVAR_DEFINE_FUNCTION(AR_SetU64, feature_t ptr, const char* key, uint64_t value);
		NVAR_DEFINE_FUNCTION(AR_GetF32, feature_t ptr, const char* key, float* value);
		NVAR_DEFINE_FUNCTION(AR_SetF32, feature_t ptr, const char* key, float value);
		NVAR_DEFINE_FUNCTION(AR_GetF64, feature_t ptr, const char* key, double* value);
		NVAR_DEFINE_FUNCTION(AR_SetF64, feature_t ptr, const char* key, double value);
		NVAR_DEFINE_FUNCTION(AR_GetString, feature_t ptr, const char* key, const char** value);
		NVAR_DEFINE_FUNCTION(AR_SetString, feature_t ptr, const char* key, const char* value);
		NVAR_DEFINE_FUNCTION(AR_GetCudaStream, feature_t ptr, const char* key, nvidia::cuda::stream_t* value);
		NVAR_DEFINE_FUNCTION(AR_SetCudaStream, feature_t ptr, const char* key, nvidia::cuda::stream_t value);
		NVAR_DEFINE_FUNCTION(AR_GetObject, feature_t ptr, const char* key, void** value, uint32_t size);
		NVAR_DEFINE_FUNCTION(AR_SetObject, feature_t ptr, const char* key, void* value, uint32_t size);
		NVAR_DEFINE_FUNCTION(AR_GetF32Array, feature_t ptr, const char* key, const float** values, int32_t* size);
		NVAR_DEFINE_FUNCTION(AR_SetF32Array, feature_t ptr, const char* key, const float* values, int32_t size);

		NVAR_DEFINE_FUNCTION(AR_CudaStreamCreate, nvidia::cuda::stream_t* ptr);
		NVAR_DEFINE_FUNCTION(AR_CudaStreamDestroy, nvidia::cuda::stream_t ptr);

		NVAR_DEFINE_FUNCTION2(CV_GetErrorStringFromCode, const char*, result code);

		NVAR_DEFINE_FUNCTION(CVImage_Create, uint32_t width, uint32_t height, cv_pixel_format pixel_format,
							 cv_component_type component_type, cv_planar is_planar, cv_memory on_gpu,
							 uint32_t alignment, cv_image* image);
		NVAR_DEFINE_FUNCTION2(CVImage_Destroy, void, cv_image* image);
		NVAR_DEFINE_FUNCTION(CVImage_Init, cv_image* image, uint32_t width, uint32_t height, int32_t pitch, void* data,
							 cv_pixel_format pixel_format, cv_component_type component_type, cv_planar is_planar,
							 cv_memory on_gpu);
		NVAR_DEFINE_FUNCTION2(CVImage_InitView, void, cv_image* sub_image, cv_image* main_image, int32_t x, int32_t y,
							  uint32_t width, uint32_t height);
		NVAR_DEFINE_FUNCTION(CVImage_Alloc, cv_image* image, uint32_t width, uint32_t height,
							 cv_pixel_format pixel_format, cv_component_type component_type, cv_planar is_planar,
							 cv_memory memory_type, uint32_t alignment);
		NVAR_DEFINE_FUNCTION(CVImage_Realloc, cv_image* image, uint32_t width, uint32_t height,
							 cv_pixel_format pixel_format, cv_component_type component_type, cv_planar is_planar,
							 cv_memory memory_type, uint32_t alignment);
		NVAR_DEFINE_FUNCTION2(CVImage_Dealloc, void, cv_image* image);
		NVAR_DEFINE_FUNCTION2(CVImage_ComponentOffsets, void, cv_pixel_format pixel_format, int32_t* r_offset,
							  int32_t* g_offset, int32_t* b_offset, int32_t* a_offset, int32_t* y_offset);
		NVAR_DEFINE_FUNCTION(CVImage_Transfer, const cv_image* src, cv_image* dst, float scale,
							 nvidia::cuda::stream_t stream, cv_image* tmp);
		NVAR_DEFINE_FUNCTION(CVImage_Composite, const cv_image* src, const cv_image* mat, cv_image* dst);
		NVAR_DEFINE_FUNCTION(CVImage_CompositeOverConstant, const cv_image* src, const cv_image* mat,
							 const uint8_t color[3], cv_image* dst);
		NVAR_DEFINE_FUNCTION(CVImage_FlipY, const cv_image* src, cv_image* dst);

		public:
		static std::shared_ptr<::nvidia::ar::ar> get();
	};
} // namespace nvidia::ar

#undef NVAR_DEFINE_FUNCTION
#undef NVAR_DEFINE_FUNCTION2
