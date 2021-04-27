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

#include "nvidia-ar.hpp"
#include <mutex>
#include <stdexcept>
#include <vector>
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<nvidia::ar::ar> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

#ifdef WIN32
#include <KnownFolders.h>
#include <ShlObj.h>
#include <Windows.h>

#define LIBRARY_NAME "nvARPose.dll"
#else
#define LIBRARY_NAME "libnvARPose.so"
#endif

#define LOAD_SYMBOL(NAME)                                                                      \
	{                                                                                          \
		NAME = static_cast<decltype(NAME)>(_library->load_symbol("Nv" #NAME));                 \
		if (!NAME)                                                                             \
			throw std::runtime_error("Failed to load 'Nv" #NAME "' from '" LIBRARY_NAME "'."); \
	}

nvidia::ar::ar::~ar()
{
	D_LOG_DEBUG("Finalizing... (Addr: 0x%" PRIuPTR ")", this);
}

nvidia::ar::ar::ar() : _sdk_path(""), _library()
{
	D_LOG_DEBUG("Initializating... (Addr: 0x%" PRIuPTR ")", this);

	{   // Find out where the SDK is located at.
		// NV_AR_SDK_PATH is undefined in current installer.
		// NVAR_MODEL_PATH is defined to point to the models themselves.
#ifdef WIN32
		DWORD env_size = GetEnvironmentVariableW(L"NV_AR_SDK_PATH", nullptr, 0);
		if (env_size > 0) {
			std::vector<wchar_t> wide_buffer;
			wide_buffer.resize(static_cast<size_t>(env_size) + 1);
			env_size  = GetEnvironmentVariableW(L"NV_AR_SDK_PATH", wide_buffer.data(), wide_buffer.size());
			_sdk_path = std::wstring(wide_buffer.data(), wide_buffer.size());
		} else {
			PWSTR   str = nullptr;
			HRESULT res = SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &str);
			if (res == S_OK) {
				_sdk_path = std::wstring(str);
				_sdk_path /= "NVIDIA Corporation";
				_sdk_path /= "NVIDIA AR SDK";
				CoTaskMemFree(str);
			}
		}
#else
		char* env = getenv("NV_AR_SDK_PATH");
		if (env != nullptr) {
			_sdk_path = env;
		}
#endif
	}

	{   // Find out where the models are located at.
		// NVAR_MODEL_PATH is defined to point to the models themselves.
#ifdef WIN32
		DWORD env_size = GetEnvironmentVariableW(L"NVAR_MODEL_PATH", nullptr, 0);
		if (env_size > 0) {
			std::vector<wchar_t> wide_buffer;
			wide_buffer.resize(static_cast<size_t>(env_size) + 1);
			env_size    = GetEnvironmentVariableW(L"NVAR_MODEL_PATH", wide_buffer.data(), wide_buffer.size());
			_model_path = std::wstring(wide_buffer.data(), wide_buffer.size());
		} else {
			_model_path = std::filesystem::path(_sdk_path).append("models");
		}
#else
		char* env = getenv("NVAR_MODEL_PATH");
		if (env != nullptr) {
			_sdk_path = std::string(env);
		} else {
			_model_path = std::filesystem::path(_sdk_path).append("models");
		}
#endif
	}

	{ // Attempt to load the library.
		std::string library_name;
#ifdef WIN32
		library_name = "nvARPose.dll";
#else
		library_name = "libnvARPose.so";
#endif
		_library = util::library::load(std::filesystem::path(_sdk_path).append(library_name));
	}

	// Load Symbols
	LOAD_SYMBOL(AR_GetVersion);
	LOAD_SYMBOL(AR_Create);
	LOAD_SYMBOL(AR_Destroy);
	LOAD_SYMBOL(AR_Run);
	LOAD_SYMBOL(AR_Load);
	LOAD_SYMBOL(AR_GetS32);
	LOAD_SYMBOL(AR_SetS32);
	LOAD_SYMBOL(AR_GetU32);
	LOAD_SYMBOL(AR_SetU32);
	LOAD_SYMBOL(AR_GetU64);
	LOAD_SYMBOL(AR_SetU64);
	LOAD_SYMBOL(AR_GetF32);
	LOAD_SYMBOL(AR_SetF32);
	LOAD_SYMBOL(AR_GetF64);
	LOAD_SYMBOL(AR_SetF64);
	LOAD_SYMBOL(AR_GetString);
	LOAD_SYMBOL(AR_SetString);
	LOAD_SYMBOL(AR_GetCudaStream);
	LOAD_SYMBOL(AR_SetCudaStream);
	LOAD_SYMBOL(AR_GetObject);
	LOAD_SYMBOL(AR_SetObject);
	LOAD_SYMBOL(AR_GetF32Array);
	LOAD_SYMBOL(AR_SetF32Array);
	LOAD_SYMBOL(AR_CudaStreamCreate);
	LOAD_SYMBOL(AR_CudaStreamDestroy);
	LOAD_SYMBOL(CV_GetErrorStringFromCode);
	LOAD_SYMBOL(CVImage_Create);
	LOAD_SYMBOL(CVImage_Destroy);
	LOAD_SYMBOL(CVImage_Init);
	LOAD_SYMBOL(CVImage_InitView);
	LOAD_SYMBOL(CVImage_Alloc);
	LOAD_SYMBOL(CVImage_Realloc);
	LOAD_SYMBOL(CVImage_Dealloc);
	LOAD_SYMBOL(CVImage_ComponentOffsets);
	LOAD_SYMBOL(CVImage_Transfer);
	LOAD_SYMBOL(CVImage_Composite);
	LOAD_SYMBOL(CVImage_CompositeOverConstant);
	LOAD_SYMBOL(CVImage_FlipY);
}

std::filesystem::path nvidia::ar::ar::get_sdk_path()
{
	return _sdk_path;
}

std::filesystem::path nvidia::ar::ar::get_model_path()
{
	return _model_path;
}

std::shared_ptr<nvidia::ar::ar> nvidia::ar::ar::get()
{
	static std::shared_ptr<nvidia::ar::ar> instance;
	static std::mutex                      lock;

	std::unique_lock<std::mutex> ul(lock);
	if (!instance) {
		instance = std::make_shared<nvidia::ar::ar>();
	}
	return instance;
}

nvidia::ar::ar_error::ar_error(result code) : std::exception(ar::get()->CV_GetErrorStringFromCode(code)), _code(code) {}
