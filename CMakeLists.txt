# StreamFX - The premier VFX plugin for OBS Studio.
# Copyright (C) 2017 - 2020 Michael Fabian Dirks
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# CMake Setup
cmake_minimum_required(VERSION 3.8...4.0)

################################################################################
# Configure Type
################################################################################

# Detect if we are building by ourselves or as part of something else.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
	set(GROUPED OFF)
	set(PREFIX "")
else()
	set(GROUPED ON)
	set(PREFIX "StreamFX_")
endif()
set(LOGPREFIX "StreamFX:")

################################################################################
# Versioning
################################################################################

set(VERSION_MAJOR 0)
set(VERSION_MINOR 10)
set(VERSION_PATCH 0)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
set(VERSION_COMMIT "00000000")

# Check if we are in a git repository.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	# Try and figure out where git is.
	find_program(GIT git
		PATHS
			/bin
			/sbin
			/usr/bin
			/usr/local/bin
	)

	if(GIT)
		set(GIT_RESULT)
		set(GIT_OUTPUT)
		set(GIT_ERROR)

		execute_process(
			COMMAND "${GIT}" describe --tags --long --match "[0-9]*.[0-9]*.[0-9]*" --abbrev=8 HEAD
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE GIT_OUTPUT
			ERROR_VARIABLE GIT_ERROR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)

		if(GIT_RESULT EQUAL 0)
			string(REPLACE "-" "." GIT_OUTPUT "${GIT_OUTPUT}")
			string(REPLACE "." ";" GIT_OUTPUT "${GIT_OUTPUT}")

			# Parse Version
			list(GET GIT_OUTPUT 0 VERSION_MAJOR)
			list(GET GIT_OUTPUT 1 VERSION_MINOR)
			list(GET GIT_OUTPUT 2 VERSION_PATCH)
			list(GET GIT_OUTPUT 3 VERSION_TWEAK)
			list(GET GIT_OUTPUT 4 VERSION_COMMIT)

			# Patch needs additional parsing.
			# This may be a [0-9]*[a-z]*[0-9]+ string.
			string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
			set(VERSION_PATCH "${CMAKE_MATCH_1}")
			if(CMAKE_MATCH_2)
				set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
			else()
				set(VERSION_SUFFIX "")
			endif()
		else()
			message(WARNING "${LOGPREFIX} Failed to detect version, using default instead.")
		endif()
	endif()
else()
	message(STATUS "${LOGPREFIX} Not a git repository, automatic version detection disabled.")
endif()

# Allow manual overrides of the detected version.
set(${PREFIX}VERSION "" CACHE STRING "Override StreamFX version with this string. Format: Major.Minor.Patch[Suffix][-Tweak[-Commit8c]]")
if(NOT (${PREFIX}VERSION STREQUAL ""))
	string(REPLACE "-" "." T_VERSION "${${PREFIX}VERSION}")
	string(REPLACE "." ";" T_VERSION "${${PREFIX}VERSION}")

	list(LENGTH T_VERSION T_VERSIONLEN)
	list(GET T_VERSION 0 VERSION_MAJOR)
	list(GET T_VERSION 1 VERSION_MINOR)
	list(GET T_VERSION 2 VERSION_PATCH)
	if (T_VERSIONLEN GREATER_EQUAL 3)
		list(GET T_VERSION 3 VERSION_TWEAK)
	else()
		set(VERSION_BUILD 0)
	endif()
	if (T_VERSIONLEN GREATER_EQUAL 4)
		list(GET T_VERSION 4 VERSION_COMMIT)
	else()
		set(VERSION_COMMIT "")
	endif()

	# Patch needs additional parsing.
	# This may be a [0-9]*[a-z]*[0-9]+ string.
	string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
	set(VERSION_PATCH "${CMAKE_MATCH_1}")
	if(CMAKE_MATCH_2)
		set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
	else()
		set(VERSION_SUFFIX "")
	endif()
endif()

# Generate Version String
if(NOT (VERSION_COMMIT STREQUAL ""))
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}-${VERSION_COMMIT}")
else()
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()

# Log the detected version.
message(STATUS "${LOGPREFIX} Version ${VERSION_STRING}")

################################################################################
# Project
################################################################################
project(
	StreamFX
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
	DESCRIPTION "Additional sources, filters, transitions and encoders for OBS Studio."
	HOMEPAGE_URL "https://streamfx.xaymar.com/"
)

# Full Project Name
set(PROJECT_FULL_NAME "StreamFX (for OBS Studio)")

# Description
set(PROJECT_DESCRIPTION "Better Production Quality, for free.")

# Authors (TODO: Generate this from AUTHORS)
set(PROJECT_AUTHORS "Michael Fabian 'Xaymar' Dirks <info@xaymar.com>")

# Copyright Years (TODO: Update every year)
set(PROJECT_COPYRIGHT_YEARS "2018 - 2020")

# Versioning
set(PROJECT_VERSION_STRING ${VERSION_STRING})

################################################################################
# Modules
################################################################################

# Search Paths
set(CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# Include
include("Architecture")					# Architecture Detection
include("util")							# CacheClear, CacheSet
include("DownloadProject")				# DownloadProject

################################################################################
# Platform Setup
################################################################################

# Operating System
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(D_PLATFORM_OS "windows")
	set(D_PLATFORM_WINDOWS 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(D_PLATFORM_OS "linux")
	set(D_PLATFORM_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(D_PLATFORM_OS "macos")
	set(D_PLATFORM_MAC 1)
else()
	set(D_PLATFORM_OS "unknown")
	set(D_PLATFORM_UNKNOWN 1)
	message(WARNING "${LOGPREFIX} The operating system '${CMAKE_SYSTEM_NAME}' is unknown to to this script, continue at your own risk.")
endif()

# Architecture
set(D_PLATFORM_INSTR ${ARCH_INST})
if(ARCH_INST STREQUAL "x86")
	set(D_PLATFORM_INSTR_X86 ON)
	set(D_PLATFORM_ARCH_X86 ON)
elseif(ARCH_INST STREQUAL "ARM")
	set(D_PLATFORM_INSTR_ARM ON)
	set(D_PLATFORM_ARCH_ARM ON)
elseif(ARCH_INST STREQUAL "IA64")
	set(D_PLATFORM_INSTR_ITANIUM ON)
	set(D_PLATFORM_ARCH_ITANIUM ON)
endif()
set(D_PLATFORM_ARCH ${ARCH_INST})

# Bitness
set(D_PLATFORM_BITS ${ARCH_BITS})
set(D_PLATFORM_BITS_PTR ${ARCH_BITS_POINTER})

################################################################################
# C/C++ Compiler Adjustments
################################################################################
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	message(STATUS "Applying custom flags for MSVC style build.")

	# MSVC/ClangCL
	# - Dynamically link Microsoft C/C++ Redistributable.
	# - Enable /W3 and disable useless warnings.
	# - Enable C++ exceptions with SEH exceptions.
	# - Enable multi-processor compiling.

	# Build with dynamic MSVC linkage.
    add_compile_options(
        $<$<CONFIG:>:/MD>
        $<$<CONFIG:Debug>:/MDd>
        $<$<CONFIG:Release>:/MD>
        $<$<CONFIG:RelWithDebInfo>:/MD>
        $<$<CONFIG:MinSizeRel>:/MD>
    )

	# Enable most useful warnings.
	set(DISABLED_WARNINGS
		"/wd4061" "/wd4100" "/wd4180" "/wd4201" "/wd4464" "/wd4505" "/wd4514"
		"/wd4571" "/wd4623" "/wd4625" "/wd4626" "/wd4668" "/wd4710" "/wd4774"
		"/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd26812"
	)
	add_compile_options("/W3")
	foreach(WARN ${DISABLED_WARNINGS})
		add_compile_options("${WARN}")
	endforeach()

	# C++ Exceptions & SEH
	add_compile_options("/EHa")

	# Multiprocessor compiling
	add_compile_options("/MP")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message(STATUS "Applying custom flags for GNU style build.")

	# Clang/AppleClang/GNU
	# - Don't export by default. (Temporarily disabled)
	# - Enable all and extra warnings.

	add_compile_options("-Wall")
	add_compile_options("-Wextra")
	# add_compile_options("-fvisibility=hidden")
endif()

################################################################################
# Detect if we are building with OBS Studio (different from Grouped builds)
################################################################################

set(STANDALONE ON)
if(GROUPED AND (TARGET libobs))
	set(STANDALONE OFF)
endif()
if(STANDALONE)
	message(STATUS "${LOGPREFIX} This is a standalone build, please make sure you've followed the instructions.")
	set(${PREFIX}OBS_NATIVE OFF)
else()
	message(STATUS "${LOGPREFIX} This is a combined build.")
	set(${PREFIX}OBS_NATIVE ON)
endif()

################################################################################
# Options
################################################################################

# Features
## Encoders
set(${PREFIX}ENABLE_ENCODER_FFMPEG ON CACHE BOOL "Enable FFmpeg Encoder integration.")
set(${PREFIX}ENABLE_ENCODER_FFMPEG_AMF ON CACHE BOOL "Enable AMF Encoder in FFmpeg.")
set(${PREFIX}ENABLE_ENCODER_FFMPEG_NVENC ON CACHE BOOL "Enable NVENC Encoder in FFmpeg.")
set(${PREFIX}ENABLE_ENCODER_FFMPEG_PRORES ON CACHE BOOL "Enable ProRes Encoder in FFmpeg.")

## Filters
set(${PREFIX}ENABLE_FILTER_AUTOFRAMING ON CACHE BOOL "Enable Auto-Framing Filter")
set(${PREFIX}ENABLE_FILTER_BLUR ON CACHE BOOL "Enable Blur Filter")
set(${PREFIX}ENABLE_FILTER_COLOR_GRADE ON CACHE BOOL "Enable Color Grade Filter")
set(${PREFIX}ENABLE_FILTER_DISPLACEMENT ON CACHE BOOL "Enable Displacement Filter")
set(${PREFIX}ENABLE_FILTER_DYNAMIC_MASK ON CACHE BOOL "Enable Dynamic Mask Filter")
set(${PREFIX}ENABLE_FILTER_SDF_EFFECTS ON CACHE BOOL "Enable SDF Effects Filter")
set(${PREFIX}ENABLE_FILTER_SHADER ON CACHE BOOL "Enable Shader Filter")
set(${PREFIX}ENABLE_FILTER_TRANSFORM ON CACHE BOOL "Enable Transform Filter")

## Sources
set(${PREFIX}ENABLE_SOURCE_MIRROR ON CACHE BOOL "Enable Mirror Source")
set(${PREFIX}ENABLE_SOURCE_SHADER ON CACHE BOOL "Enable Shader Source")

## Transitions
set(${PREFIX}ENABLE_TRANSITION_SHADER ON CACHE BOOL "Enable Shader Transition")

## FrontEnd & UI
set(${PREFIX}ENABLE_FRONTEND ON CACHE BOOL "Enable Frontend code.")
set(${PREFIX}ENABLE_UPDATER ON CACHE BOOL "Enable automatic update checks.")

## Code Related
set(${PREFIX}ENABLE_CLANG ON CACHE BOOL "Enable Clang integration for supported compilers.")
set(${PREFIX}ENABLE_PROFILING OFF CACHE BOOL "Enable CPU and GPU performance tracking, which has a non-zero overhead at all times. Do not enable this for release builds.")

# Installation / Packaging
if(STANDALONE)
	set(STRUCTURE_UNIFIED CACHE BOOL "Install for use in a Plugin Manager")
	if(D_PLATFORM_LINUX)
		set(STRUCTURE_PACKAGEMANAGER CACHE BOOL "Install for use in a Package Manager (system-wide installation)")
	endif()

	set(PACKAGE_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Where to place the packages?")
	set(PACKAGE_NAME "${PROJECT_NAME}" CACHE STRING "What should the package be called?")
	set(PACKAGE_SUFFIX "" CACHE STRING "Any suffix for the package name? (Defaults to the current version string)")
endif()

################################################################################
# Clang
################################################################################

if(${PREFIX}ENABLE_CLANG AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang/Clang.cmake"))
	include("Clang")
	set(HAVE_CLANG ON)
endif()

################################################################################
# Standalone Build: OBS Studio
################################################################################

if(NOT ${PREFIX}OBS_NATIVE)
	# Options
	set(${PREFIX}DOWNLOAD_OBS_URL "" CACHE STRING "(Optional) URL of prebuilt libOBS archive to download.")
	set(${PREFIX}DOWNLOAD_OBS_HASH "" CACHE STRING "(Optional) The hash for the libOBS archive.")
	mark_as_advanced(
		${PREFIX}DOWNLOAD_OBS_URL
		${PREFIX}DOWNLOAD_OBS_HASH
	)

	# Allow overriding what version we build against.
	if(${PREFIX}DOWNLOAD_OBS_URL)
		set(_DOWNLOAD_OBS_URL "${${PREFIX}DOWNLOAD_OBS_URL}")
		set(_DOWNLOAD_OBS_HASH "${${PREFIX}DOWNLOAD_OBS_HASH}")
	else()
		set(_DOWNLOAD_OBS_VERSION "26.1.0-ci")
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-windows-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=EA9EE4CCEC0A19605C0768F1F4C21391A919CF0886F7A7CC94D63A95402CFD9B")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_LINUX)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-ubuntu-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=2B2793CDD0C3F5940924665022E31A76C10E34174073BF7B221325C11E764DF5")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_MAC)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-macos-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=34ABD3E574C62DA4E026E5DE271B93AB5151927E564F77264A59456F6ABCBDD0")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		else()
			message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
			return()
		endif()
	endif()

	# Download libOBS
	download_project(
		PROJ libobs
		URL "${_DOWNLOAD_OBS_URL}"
		URL_HASH "${_DOWNLOAD_OBS_HASH}"
		DOWNLOAD_NO_PROGRESS OFF
		UPDATE_DISCONNECTED OFF
	)

	include("${libobs_SOURCE_DIR}/cmake/LibObs/LibObsConfig.cmake")
endif()

################################################################################
# Standalone Build: OBS Studio Dependencies
################################################################################

if(STANDALONE AND NOT D_PLATFORM_LINUX)
	# Options
	set(${PREFIX}DOWNLOAD_OBSDEPS_URL "" CACHE STRING "(Optional) URL of prebuilt libOBS archive to download.")
	set(${PREFIX}DOWNLOAD_OBSDEPS_HASH "" CACHE STRING "(Optional) The hash for the libOBS archive.")
	mark_as_advanced(
		${PREFIX}DOWNLOAD_OBSDEPS_URL
		${PREFIX}DOWNLOAD_OBSDEPS_HASH
	)

	# Allow overriding what version we build against.
	if(${PREFIX}DOWNLOAD_OBSDEPS_URL)
		set(_DOWNLOAD_OBSDEPS_URL "${${PREFIX}DOWNLOAD_OBSDEPS_URL}")
		set(_DOWNLOAD_OBSDEPS_HASH "${${PREFIX}DOWNLOAD_OBSDEPS_HASH}")
	else()
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBSDEPS_URL "https://github.com/Xaymar/obs-studio/releases/download/26.1.0/deps_windows_x86.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=F5F8F5A53FDB26FF8F4D24811CA0FEA891B3A0C8477515B886129669A81D37C2")
				elseif (D_PLATFORM_BITS EQUAL 32)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=F5F8F5A53FDB26FF8F4D24811CA0FEA891B3A0C8477515B886129669A81D37C2")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_MAC)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBSDEPS_URL "https://github.com/Xaymar/obs-studio/releases/download/26.1.0/deps_macos_x86.tar.gz")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=B316DD7802E0DEAE2BD43F52D4C8BD52EF893D203B90148C2AB851A00AA733FF")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		else()
			message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
			return()
		endif()
	endif()

	# Download libOBS
	download_project(
		PROJ obsdeps
		URL "${_DOWNLOAD_OBSDEPS_URL}"
		URL_HASH "${_DOWNLOAD_OBSDEPS_HASH}"
		DOWNLOAD_NO_PROGRESS OFF
		UPDATE_DISCONNECTED OFF
	)

	if (D_PLATFORM_WINDOWS)
		set(_OBSDEPS_PATH "${obsdeps_SOURCE_DIR}/win${D_PLATFORM_BITS}")
	elseif(D_PLATFORM_MAC)
		set(_OBSDEPS_PATH "${obsdeps_SOURCE_DIR}/obsdeps")
	endif()
endif()

################################################################################
# Standalone Build: Qt v5.x
################################################################################

if(STANDALONE AND NOT D_PLATFORM_LINUX)
	set(${PREFIX}DOWNLOAD_QT OFF CACHE BOOL "Download Qt?")

	if(${PREFIX}DOWNLOAD_QT)
		set(${PREFIX}DOWNLOAD_QT_URL "" CACHE STRING "")
		set(${PREFIX}DOWNLOAD_QT_HASH "" CACHE STRING "")
		mark_as_advanced(
			${PREFIX}DOWNLOAD_QT_URL
			${PREFIX}DOWNLOAD_QT_HASH
		)

		# Allow overriding what version we build against.
		if(${PREFIX}DOWNLOAD_QT_URL)
			set(_DOWNLOAD_QT_URL "${${PREFIX}DOWNLOAD_QT_URL}")
			set(_DOWNLOAD_QT_HASH "${${PREFIX}DOWNLOAD_QT_HASH}")
		else()
			if (D_PLATFORM_WINDOWS)
				if (D_PLATFORM_ARCH_X86)
					if (D_PLATFORM_BITS EQUAL 64)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/26.1.0/qt_5.15.2_windows_64.7z")
						set(_DOWNLOAD_QT_HASH "SHA256=109B9C21EF165B0C46DFAA9AD23124F2070ED4D74207C4AFB308183CB8D43BDD")
					elseif (D_PLATFORM_BITS EQUAL 32)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/26.1.0/qt_5.15.2_windows_32.7z")
						set(_DOWNLOAD_QT_HASH "SHA256=372E4FBF2A15DD4FDA955A07334D8B8AC6802990148C9CB4E766C90205F8F570")
					else()
						message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
						return()
					endif()
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			elseif(D_PLATFORM_MAC)
				if (D_PLATFORM_ARCH_X86)
					set(_DOWNLOAD_QT_URL "https://github.com/obsproject/obs-deps/releases/download/2020-12-22/macos-qt-5.15.2-2020-12-22.tar.gz")
					if (D_PLATFORM_BITS EQUAL 64)
						set(_DOWNLOAD_QT_HASH "SHA256=5BA0BC722F54391FE27508D6DB57EC63BDA22DAD890F5960145C731C1BB837ED")
					else()
						message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
						return()
					endif()
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		endif()

		download_project(
			PROJ qt
			URL "${_DOWNLOAD_QT_URL}"
			URL_HASH "${_DOWNLOAD_QT_HASH}"
			DOWNLOAD_NO_PROGRESS OFF
			UPDATE_DISCONNECTED OFF
		)

		set(Qt5_DIR "${qt_SOURCE_DIR}" CACHE STRING "Path to Qt5")
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				if (D_PLATFORM_BITS EQUAL 64)
					CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5")
				elseif (D_PLATFORM_BITS EQUAL 32)
					CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5")
				endif()
			endif()
		elseif(D_PLATFORM_MAC)
			CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5")
		endif()
	endif()
endif()

################################################################################
# Components
################################################################################

# Component resolving:
# 1. Check which features are enabled. For each feature, set what they require to ON.
# 2. Try and find required items.
# 3. Again check which features are enabled, if their requirements are missing, warn about it and disable them.
# TODO: Consider making this an error instead.

function(is_feature_enabled FEATURE OUTPUT)
	set(T_ENABLED ${${PREFIX}ENABLE_${FEATURE}})
	set(T_DISABLED ${${PREFIX}DISABLE_${FEATURE}})
	if(T_ENABLED AND NOT T_DISABLED)
		set(${OUTPUT} ON PARENT_SCOPE)
	else()
		set(${PREFIX}DISABLE_${FEATURE} ON PARENT_SCOPE)
		set(${OUTPUT} OFF PARENT_SCOPE)
	endif()
endfunction()

macro(set_feature_disabled FEATURE DISABLED)
	set(${PREFIX}DISABLE_${FEATURE} ${DISABLED} PARENT_SCOPE)
endmacro()

# Features
function(feature_encoder_ffmpeg RESOLVE)
	is_feature_enabled(ENCODER_FFMPEG T_CHECK)
	if(RESOLVE AND T_CHECK)
		if(NOT HAVE_FFMPEG)
			message(WARNING "${LOGPREFIX}: FFmpeg Encoder requires FFmpeg. Disabling...")
			set_feature_disabled(ENCODER_FFMPEG ON)
		else()
			# AMF
			is_feature_enabled(ENCODER_FFMPEG_AMF T_CHECK)
			if(T_CHECK AND D_PLATFORM_MAC)
				message(WARNING "${LOGPREFIX}: FFmpeg Encoder 'AMF' requires Windows or Linux. Disabling...")
				set_feature_disabled(ENCODER_FFMPEG_AMF ON)
			endif()

			# NVENC
			is_feature_enabled(ENCODER_FFMPEG_NVENC T_CHECK)
			if(T_CHECK AND D_PLATFORM_MAC)
				message(WARNING "${LOGPREFIX}: FFmpeg Encoder 'NVENC' requires Windows or Linux. Disabling...")
				set_feature_disabled(ENCODER_FFMPEG_NVENC ON)
			endif()

			# ProRes
			is_feature_enabled(ENCODER_FFMPEG_PRORES T_CHECK)
		endif()
	elseif(T_CHECK)
		set(REQUIRE_FFMPEG ON PARENT_SCOPE)
	endif()
endfunction()

function(feature_filter_blur RESOLVE)
	is_feature_enabled(FILTER_BLUR T_CHECK)
endfunction()

function(feature_filter_color_grade RESOLVE)
	is_feature_enabled(FILTER_COLOR_GRADE T_CHECK)
endfunction()

function(feature_filter_displacement RESOLVE)
	is_feature_enabled(FILTER_DISPLACEMENT T_CHECK)
endfunction()

function(feature_filter_dynamic_mask RESOLVE)
	is_feature_enabled(FILTER_DYNAMIC_MASK T_CHECK)
endfunction()

function(feature_filter_autoframing RESOLVE)
	is_feature_enabled(FILTER_AUTOFRAMING T_CHECK)
	if(RESOLVE AND T_CHECK)
		if(NOT D_PLATFORM_WINDOWS)
			message(WARNING "${LOGPREFIX}: Auto-Framing requires Windows. Disabling...")
			set_feature_disabled(FILTER_AUTOFRAMING ON)
		elseif(NOT HAVE_NVIDIA_ARSDK)
			message(WARNING "${LOGPREFIX}: Auto-Framing requires NVIDIA AR SDK. Disabling...")
			set_feature_disabled(FILTER_AUTOFRAMING ON)
		elseif(NOT HAVE_NVIDIA_CUDA)
			message(WARNING "${LOGPREFIX}: Auto-Framing requires NVIDIA CUDA. Disabling...")
			set_feature_disabled(FILTER_AUTOFRAMING ON)
		endif()
	elseif(T_CHECK)
		set(REQUIRE_NVIDIA_ARSDK ON PARENT_SCOPE)
		set(REQUIRE_NVIDIA_CUDA ON PARENT_SCOPE)
	endif()
endfunction()

function(feature_filter_sdf_effects RESOLVE)
	is_feature_enabled(FILTER_SDF_EFFECTS T_CHECK)
endfunction()

function(feature_filter_shader RESOLVE)
	is_feature_enabled(FILTER_SHADER T_CHECK)
endfunction()

function(feature_filter_transform RESOLVE)
	is_feature_enabled(FILTER_TRANSFORM T_CHECK)
endfunction()

function(feature_source_mirror RESOLVE)
	is_feature_enabled(SOURCE_MIRROR T_CHECK)
endfunction()

function(feature_source_shader RESOLVE)
	is_feature_enabled(SOURCE_SHADER T_CHECK)
endfunction()

function(feature_transition_shader RESOLVE)
	is_feature_enabled(TRANSITION_SHADER T_CHECK)
endfunction()

function(feature_frontend RESOLVE)
	is_feature_enabled(FRONTEND T_CHECK)
	if(RESOLVE AND T_CHECK)
		if(NOT HAVE_QT)
			message(WARNING "${LOGPREFIX}: Front-End requires Qt. Disabling...")
			set_feature_disabled(FRONTEND ON)
		elseif(NOT HAVE_OBSFE)
			message(WARNING "${LOGPREFIX}: Front-End requires OBS FrontEnd API. Disabling...")
			set_feature_disabled(FRONTEND ON)
		endif()
	elseif(T_CHECK)
		set(REQUIRE_QT ON PARENT_SCOPE)
		set(REQUIRE_OBSFE ON PARENT_SCOPE)
	endif()
endfunction()

function(feature_updater RESOLVE)
	is_feature_enabled(UPDATER T_CHECK)
	if(RESOLVE AND T_CHECK)
		if(NOT HAVE_CURL)
			message(WARNING "${LOGPREFIX}: Updater requires CURL. Disabling...")
			set_feature_disabled(UPDATER ON)
		elseif(NOT HAVE_JSON)
			message(WARNING "${LOGPREFIX}: Updater requires nlohmann::json. Disabling...")
			set_feature_disabled(UPDATER ON)
		endif()
	elseif(T_CHECK)
		set(REQUIRE_CURL ON PARENT_SCOPE)
		set(REQUIRE_JSON ON PARENT_SCOPE)
	endif()
endfunction()

# Set Requirements
feature_encoder_ffmpeg(OFF)
feature_filter_blur(OFF)
feature_filter_color_grade(OFF)
feature_filter_displacement(OFF)
feature_filter_dynamic_mask(OFF)
feature_filter_autoframing(OFF)
feature_filter_sdf_effects(OFF)
feature_filter_shader(OFF)
feature_filter_transform(OFF)
feature_source_mirror(OFF)
feature_source_shader(OFF)
feature_transition_shader(OFF)
feature_frontend(OFF)
feature_updater(OFF)

# Fulfill Requirements
#- CURL
set(HAVE_CURL OFF)
if(REQUIRE_CURL)
	if(D_PLATFORM_WINDOWS)
		if(${PREFIX}OBS_NATIVE) # Already defined by OBS
			set(CURL_LIBRARIES "${CURL_LIB}")
			set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIR}")
		else()
			set(CURL_LIBRARIES "${_OBSDEPS_PATH}/bin/libcurl.lib")
			set(CURL_INCLUDE_DIRS "${_OBSDEPS_PATH}/include")
		endif()
		set(CURL_LIBRARY_DEBUG ${CURL_LIBRARIES})
		set(CURL_LIBRARY_RELEASE ${CURL_LIBRARIES})
		set(CURL_INCLUDE_DIR ${CURL_INCLUDE_DIRS})
		set(CURL_FOUND ON)
	else()
		find_package(CURL)
	endif()

	set(HAVE_CURL ${CURL_FOUND})
endif()

#- FFmpeg
set(HAVE_FFMPEG OFF)
if(REQUIRE_FFMPEG)
	if(D_PLATFORM_WINDOWS AND NOT ${PREFIX}OBS_NATIVE)
		find_path(
			FFmpegPath "libavcodec/avcodec.h"
			HINTS
				${OBS_DEPENDENCIES_DIR}
				${_OBSDEPS_PATH}
				${DepsPath}
				${DepsPath32}
				${DepsPath64}

			PATHS
				/usr/include
				/usr/local/include
				/opt/local/include
				/sw/include
			PATH_SUFFIXES
				win${D_PLATFORM_BITS}
				win${D_PLATFORM_BITS}/bin
				win${D_PLATFORM_BITS}/include
				win${D_PLATFORM_INSTR}
				win${D_PLATFORM_INSTR}/bin
				win${D_PLATFORM_INSTR}/include
				bin
				include
		)
	endif()

	find_package(FFmpeg COMPONENTS avutil avcodec swscale)
	set(HAVE_FFMPEG ${FFmpeg_FOUND})
endif()

#- JSON
set(HAVE_JSON OFF)
if(REQUIRE_JSON)
	if(NOT EXISTS "${PROJECT_SOURCE_DIR}/third-party/nlohmann-json/single_include")
		message(FATAL_ERROR "${LOGPREFIX} Please make sure to update git submodules to their latest supported version.")
		return()
	else()
		set(JSON_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third-party/nlohmann-json/single_include")
		set(HAVE_JSON ON)
	endif()
endif()

#- NVIDIA Augmented Reality SDK (Windows)
set(HAVE_NVIDIA_ARSDK OFF)
if(REQUIRE_NVIDIA_ARSDK AND D_PLATFORM_WINDOWS)
	if(EXISTS "${PROJECT_SOURCE_DIR}/third-party/nvidia-arsdk/version.h")
		set(NVAR_ROOT "${PROJECT_SOURCE_DIR}/third-party/nvidia-arsdk")
	endif()

	find_package(NVAR)

	set(HAVE_NVIDIA_ARSDK ${NVAR_FOUND})
endif()

#- NVIDIA CUDA (Windows)
set(HAVE_NVIDIA_CUDA OFF)
if(REQUIRE_NVIDIA_CUDA AND D_PLATFORM_WINDOWS)
	set(HAVE_NVIDIA_CUDA ON)
endif()

#- OBS Front-End API
set(HAVE_OBSFE OFF)
if(REQUIRE_OBSFE)
	if(${PREFIX}OBS_NATIVE)
		if(TARGET obs-frontend-api)
			set(HAVE_OBSFE ON)
		endif()
	else()
		if (EXISTS "${libobs_SOURCE_DIR}/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake")
			include("${libobs_SOURCE_DIR}/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake")
			set(HAVE_OBSFE ON)
		endif()
	endif()
endif()

#- Qt5
set(HAVE_QT OFF)
if(REQUIRE_QT)
	find_package(Qt5
		COMPONENTS Widgets Core REQUIRED
	)
	set(HAVE_QT ${Qt5_FOUND})
endif()

# Verify Requirements
feature_encoder_ffmpeg(ON)
feature_filter_blur(ON)
feature_filter_color_grade(ON)
feature_filter_displacement(ON)
feature_filter_dynamic_mask(ON)
feature_filter_autoframing(ON)
feature_filter_sdf_effects(ON)
feature_filter_shader(ON)
feature_filter_transform(ON)
feature_source_mirror(ON)
feature_source_shader(ON)
feature_transition_shader(ON)
feature_frontend(ON)
feature_updater(ON)

################################################################################
# Code
################################################################################
set(PROJECT_DATA_LOCALE )
set(PROJECT_DATA_EFFECTS )
set(PROJECT_DATA_SHADERS )
set(PROJECT_LIBRARIES )
set(PROJECT_LIBRARIES_DELAYED )
set(PROJECT_INCLUDE_DIRS )
set(PROJECT_TEMPLATES )
set(PROJECT_PRIVATE_GENERATED )
set(PROJECT_PRIVATE_SOURCE )
set(PROJECT_UI )
set(PROJECT_UI_SOURCE )
set(PROJECT_DEFINITIONS )

# Configure Files
configure_file(
	"templates/config.hpp.in"
	"generated/config.hpp"
)
LIST(APPEND PROJECT_TEMPLATES "templates/config.hpp.in")
LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/config.hpp")

configure_file(
	"templates/version.hpp.in"
	"generated/version.hpp"
)
LIST(APPEND PROJECT_TEMPLATES "templates/version.hpp.in")
LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/version.hpp")

configure_file(
	"templates/module.cpp.in"
	"generated/module.cpp"
)
LIST(APPEND PROJECT_TEMPLATES "templates/module.cpp.in")
LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/module.cpp")

if(D_PLATFORM_WINDOWS) # Windows Support
	set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME}")
	set(PROJECT_COMPANY_NAME "${PROJECT_AUTHORS}")
	set(PROJECT_COPYRIGHT "${PROJECT_AUTHORS} © ${PROJECT_COPYRIGHT_YEARS}")
	set(PROJECT_LEGAL_TRADEMARKS_1 "")
	set(PROJECT_LEGAL_TRADEMARKS_2 "")

	configure_file(
		"templates/version.rc.in"
		"generated/version.rc"
		@ONLY
	)
	LIST(APPEND PROJECT_TEMPLATES "templates/version.rc.in")
	LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/version.rc")
endif()

# Minimum Dependencies
list(APPEND PROJECT_LIBRARIES libobs)

# Components
if(HAVE_CURL)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/util/util-curl.hpp"
		"source/util/util-curl.cpp"
	)
	list(APPEND PROJECT_LIBRARIES ${CURL_LIBRARY_RELEASE})
	list(APPEND PROJECT_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
endif()

if(HAVE_FFMPEG)
	list(APPEND PROJECT_LIBRARIES
		${FFMPEG_LIBRARIES}
	)
	list(APPEND PROJECT_INCLUDE_DIRS
		${FFMPEG_INCLUDE_DIRS}
	)
endif()

if(HAVE_JSON)
	list(APPEND PROJECT_INCLUDE_DIRS ${JSON_INCLUDE_DIR})
endif()

if(HAVE_NVIDIA_ARSDK)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/nvidia/ar/nvidia-ar.hpp"
		"source/nvidia/ar/nvidia-ar.cpp"
		"source/nvidia/ar/nvidia-ar-feature.hpp"
		"source/nvidia/ar/nvidia-ar-feature.cpp"
		"source/nvidia/ar/nvidia-ar-facedetection.hpp"
		"source/nvidia/ar/nvidia-ar-facedetection.cpp"
		"source/nvidia/ar/nvidia-ar-image.hpp"
		"source/nvidia/ar/nvidia-ar-image.cpp"
		"source/nvidia/ar/nvidia-ar-gs-texture.hpp"
		"source/nvidia/ar/nvidia-ar-gs-texture.cpp"
	)
	list(APPEND PROJECT_LIBRARIES
	)
endif()

if(HAVE_NVIDIA_CUDA)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/nvidia/cuda/nvidia-cuda.hpp"
		"source/nvidia/cuda/nvidia-cuda.cpp"
		"source/nvidia/cuda/nvidia-cuda-obs.hpp"
		"source/nvidia/cuda/nvidia-cuda-obs.cpp"
		"source/nvidia/cuda/nvidia-cuda-context.hpp"
		"source/nvidia/cuda/nvidia-cuda-context.cpp"
		"source/nvidia/cuda/nvidia-cuda-gs-texture.hpp"
		"source/nvidia/cuda/nvidia-cuda-gs-texture.cpp"
		"source/nvidia/cuda/nvidia-cuda-memory.hpp"
		"source/nvidia/cuda/nvidia-cuda-memory.cpp"
		"source/nvidia/cuda/nvidia-cuda-stream.hpp"
		"source/nvidia/cuda/nvidia-cuda-stream.cpp"
	)
endif()

if(REQUIRE_OBSFE AND HAVE_OBSFE)
	list(APPEND PROJECT_LIBRARIES obs-frontend-api)
endif()

if(REQUIRE_QT AND HAVE_QT)
	list(APPEND PROJECT_LIBRARIES Qt5::Core Qt5::Widgets)
endif()

################################################################################
# Features
################################################################################

# Core
list(APPEND PROJECT_PRIVATE_SOURCE
	"source/configuration.hpp"
	"source/configuration.cpp"
	"source/common.hpp"
	"source/strings.hpp"
	"source/plugin.hpp"
	"source/plugin.cpp"
	"source/util/utility.hpp"
	"source/util/utility.cpp"
	"source/util/util-bitmask.hpp"
	"source/util/util-event.hpp"
	"source/util/util-library.cpp"
	"source/util/util-library.hpp"
	"source/util/util-logging.cpp"
	"source/util/util-logging.hpp"
	"source/util/util-threadpool.cpp"
	"source/util/util-threadpool.hpp"
	"source/gfx/gfx-source-texture.hpp"
	"source/gfx/gfx-source-texture.cpp"
	"source/obs/gs/gs-helper.hpp"
	"source/obs/gs/gs-helper.cpp"
	"source/obs/gs/gs-effect.hpp"
	"source/obs/gs/gs-effect.cpp"
	"source/obs/gs/gs-effect-parameter.hpp"
	"source/obs/gs/gs-effect-parameter.cpp"
	"source/obs/gs/gs-effect-pass.hpp"
	"source/obs/gs/gs-effect-pass.cpp"
	"source/obs/gs/gs-effect-technique.hpp"
	"source/obs/gs/gs-effect-technique.cpp"
	"source/obs/gs/gs-indexbuffer.hpp"
	"source/obs/gs/gs-indexbuffer.cpp"
	"source/obs/gs/gs-limits.hpp"
	"source/obs/gs/gs-mipmapper.hpp"
	"source/obs/gs/gs-mipmapper.cpp"
	"source/obs/gs/gs-rendertarget.hpp"
	"source/obs/gs/gs-rendertarget.cpp"
	"source/obs/gs/gs-sampler.hpp"
	"source/obs/gs/gs-sampler.cpp"
	"source/obs/gs/gs-texture.hpp"
	"source/obs/gs/gs-texture.cpp"
	"source/obs/gs/gs-vertex.hpp"
	"source/obs/gs/gs-vertex.cpp"
	"source/obs/gs/gs-vertexbuffer.hpp"
	"source/obs/gs/gs-vertexbuffer.cpp"
	"source/obs/obs-encoder-factory.hpp"
	"source/obs/obs-encoder-factory.cpp"
	"source/obs/obs-signal-handler.hpp"
	"source/obs/obs-signal-handler.cpp"
	"source/obs/obs-source.hpp"
	"source/obs/obs-source.cpp"
	"source/obs/obs-source-factory.hpp"
	"source/obs/obs-source-factory.cpp"
	"source/obs/obs-source-tracker.hpp"
	"source/obs/obs-source-tracker.cpp"
	"source/obs/obs-tools.hpp"
	"source/obs/obs-tools.cpp"
)
list(APPEND PROJECT_DATA
	"data/effects/color_conversion_rgb_hsl.effect"
	"data/effects/color_conversion_rgb_hsv.effect"
	"data/effects/color_conversion_rgb_yuv.effect"
	"data/effects/mipgen.effect"
	"data/effects/pack-unpack.effect"
	"data/effects/shared.effect"
	"data/locale/en-US.ini"
)
list(APPEND PROJECT_INCLUDE_DIRS
	"${PROJECT_BINARY_DIR}/generated"
	"${PROJECT_SOURCE_DIR}/source"
)
file(GLOB_RECURSE PROJECT_EXAMPLES "data/examples/*")
list(APPEND PROJECT_DATA ${PROJECT_EXAMPLES})

# Encoder/FFmpeg
is_feature_enabled(ENCODER_FFMPEG T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_PRIVATE_SOURCE
		# FFmpeg
		"source/ffmpeg/avframe-queue.cpp"
		"source/ffmpeg/avframe-queue.hpp"
		"source/ffmpeg/swscale.hpp"
		"source/ffmpeg/swscale.cpp"
		"source/ffmpeg/tools.hpp"
		"source/ffmpeg/tools.cpp"
		"source/ffmpeg/hwapi/base.hpp"
		"source/ffmpeg/hwapi/base.cpp"
		"source/ffmpeg/hwapi/d3d11.hpp"
		"source/ffmpeg/hwapi/d3d11.cpp"

		# Encoders
		"source/encoders/encoder-ffmpeg.hpp"
		"source/encoders/encoder-ffmpeg.cpp"

		# Encoders/Codecs
		"source/encoders/codecs/hevc.hpp"
		"source/encoders/codecs/hevc.cpp"
		"source/encoders/codecs/h264.hpp"
		"source/encoders/codecs/h264.cpp"
		"source/encoders/codecs/prores.hpp"
		"source/encoders/codecs/prores.cpp"

		# Encoders/Handlers
		"source/encoders/handlers/handler.hpp"
		"source/encoders/handlers/handler.cpp"
		"source/encoders/handlers/debug_handler.hpp"
		"source/encoders/handlers/debug_handler.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_ENCODER_FFMPEG
	)

	# AMF
	is_feature_enabled(ENCODER_FFMPEG_AMF T_CHECK)
	if(T_CHECK)
		list(APPEND PROJECT_PRIVATE_SOURCE
			"source/encoders/handlers/amf_shared.hpp"
			"source/encoders/handlers/amf_shared.cpp"
			"source/encoders/handlers/amf_h264_handler.hpp"
			"source/encoders/handlers/amf_h264_handler.cpp"
			"source/encoders/handlers/amf_hevc_handler.hpp"
			"source/encoders/handlers/amf_hevc_handler.cpp"
		)
		list(APPEND PROJECT_DEFINITIONS
			ENABLE_ENCODER_FFMPEG_AMF
		)
	endif()

	# NVENC
	is_feature_enabled(ENCODER_FFMPEG_NVENC T_CHECK)
	if(T_CHECK)
		list(APPEND PROJECT_PRIVATE_SOURCE
			"source/encoders/handlers/nvenc_shared.hpp"
			"source/encoders/handlers/nvenc_shared.cpp"
			"source/encoders/handlers/nvenc_h264_handler.hpp"
			"source/encoders/handlers/nvenc_h264_handler.cpp"
			"source/encoders/handlers/nvenc_hevc_handler.hpp"
			"source/encoders/handlers/nvenc_hevc_handler.cpp"
		)
		list(APPEND PROJECT_DEFINITIONS
			ENABLE_ENCODER_FFMPEG_NVENC
		)
	endif()

	# ProRES
	is_feature_enabled(ENCODER_FFMPEG_PRORES T_CHECK)
	if(T_CHECK)
		list(APPEND PROJECT_PRIVATE_SOURCE
			"source/encoders/handlers/prores_aw_handler.hpp"
			"source/encoders/handlers/prores_aw_handler.cpp"
		)
		list(APPEND PROJECT_DEFINITIONS
			ENABLE_ENCODER_FFMPEG_PRORES
		)
	endif()
endif()

# Filter/Blur
is_feature_enabled(FILTER_BLUR T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_DATA
		"data/effects/mask.effect"
		"data/effects/blur/box.effect"
		"data/effects/blur/box-linear.effect"
		"data/effects/blur/dual-filtering.effect"
		"data/effects/blur/gaussian.effect"
		"data/effects/blur/gaussian-linear.effect"
	)
	list (APPEND PROJECT_PRIVATE_SOURCE
		"source/gfx/blur/gfx-blur-base.hpp"
		"source/gfx/blur/gfx-blur-base.cpp"
		"source/gfx/blur/gfx-blur-box.hpp"
		"source/gfx/blur/gfx-blur-box.cpp"
		"source/gfx/blur/gfx-blur-box-linear.hpp"
		"source/gfx/blur/gfx-blur-box-linear.cpp"
		"source/gfx/blur/gfx-blur-dual-filtering.hpp"
		"source/gfx/blur/gfx-blur-dual-filtering.cpp"
		"source/gfx/blur/gfx-blur-gaussian.hpp"
		"source/gfx/blur/gfx-blur-gaussian.cpp"
		"source/gfx/blur/gfx-blur-gaussian-linear.hpp"
		"source/gfx/blur/gfx-blur-gaussian-linear.cpp"
		"source/filters/filter-blur.hpp"
		"source/filters/filter-blur.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_BLUR
	)
endif()

# Filter/Color Grade
is_feature_enabled(FILTER_COLOR_GRADE T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_DATA
		"data/effects/color-grade.effect"
	)
	list (APPEND PROJECT_PRIVATE_SOURCE
		"source/filters/filter-color-grade.hpp"
		"source/filters/filter-color-grade.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_COLOR_GRADE
	)
	set(REQUIRE_LUT ON)
endif()

# Filter/Displacement
is_feature_enabled(FILTER_DISPLACEMENT T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_DATA
		"data/effects/displace.effect"
	)
	list (APPEND PROJECT_PRIVATE_SOURCE
		"source/filters/filter-displacement.hpp"
		"source/filters/filter-displacement.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_DISPLACEMENT
	)
endif()

# Filter/Dynamic Mask
is_feature_enabled(FILTER_DYNAMIC_MASK T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_DATA
		"data/effects/channel-mask.effect"
	)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/filters/filter-dynamic-mask.hpp"
		"source/filters/filter-dynamic-mask.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_DYNAMIC_MASK
	)
endif()

# Filter/NVIDIA Face Tracking
is_feature_enabled(FILTER_AUTOFRAMING T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/filters/filter-autoframing.hpp"
		"source/filters/filter-autoframing.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_AUTOFRAMING
	)
endif()

# Filter/SDF Effects
is_feature_enabled(FILTER_SDF_EFFECTS T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_DATA
		"data/effects/sdf/sdf-producer.effect"
		"data/effects/sdf/sdf-consumer.effect"
	)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/filters/filter-sdf-effects.hpp"
		"source/filters/filter-sdf-effects.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_SDF_EFFECTS
	)
endif()

# Filter/Shader
is_feature_enabled(FILTER_SHADER T_CHECK)
if(T_CHECK)
	set(REQUIRE_PART_SHADER ON)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/filters/filter-shader.hpp"
		"source/filters/filter-shader.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_SHADER
	)
endif()

# Filter/Transform
is_feature_enabled(FILTER_TRANSFORM T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/filters/filter-transform.hpp"
		"source/filters/filter-transform.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FILTER_TRANSFORM
	)
endif()

# Source/Mirror
is_feature_enabled(SOURCE_MIRROR T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/sources/source-mirror.hpp"
		"source/sources/source-mirror.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_SOURCE_MIRROR
	)
endif()

# Source/Shader
is_feature_enabled(SOURCE_SHADER T_CHECK)
if(T_CHECK)
	set(REQUIRE_PART_SHADER ON)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/sources/source-shader.hpp"
		"source/sources/source-shader.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_SOURCE_SHADER
	)
endif()

# Transition/Shader
is_feature_enabled(TRANSITION_SHADER T_CHECK)
if(T_CHECK)
	set(REQUIRE_PART_SHADER ON)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/transitions/transition-shader.hpp"
		"source/transitions/transition-shader.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_TRANSITION_SHADER
	)
endif()

# Profiling
is_feature_enabled(PROFILING T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/util/util-profiler.cpp"
		"source/util/util-profiler.hpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_PROFILING
	)
endif()

# Updater
is_feature_enabled(UPDATER T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/updater.hpp"
		"source/updater.cpp"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_UPDATER
	)
endif()

# Frontend
is_feature_enabled(FRONTEND T_CHECK)
if(T_CHECK)
	list(APPEND PROJECT_UI
		"ui/streamfx.qrc"
		"ui/about.ui"
		"ui/about-entry.ui"
	)
	list(APPEND PROJECT_UI_SOURCE
		"source/ui/ui-common.hpp"
		"source/ui/ui.hpp"
		"source/ui/ui.cpp"
		"source/ui/ui-about.hpp"
		"source/ui/ui-about.cpp"
		"source/ui/ui-about-entry.hpp"
		"source/ui/ui-about-entry.cpp"
	)
	list(APPEND PROJECT_INCLUDE_DIRS
		"source/ui"
	)
	list(APPEND PROJECT_DEFINITIONS
		ENABLE_FRONTEND
	)

	is_feature_enabled(UPDATER T_CHECK)
	if(T_CHECK)
		list(APPEND PROJECT_UI_SOURCE
			"source/ui/ui-updater.hpp"
			"source/ui/ui-updater.cpp"
		)
		list(APPEND PROJECT_UI
			"ui/updater.ui"
		)
	endif()
endif()

################################################################################
# Parts
################################################################################

# Shaders
if(REQUIRE_PART_SHADER)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/gfx/shader/gfx-shader.hpp"
		"source/gfx/shader/gfx-shader.cpp"
		"source/gfx/shader/gfx-shader-param.hpp"
		"source/gfx/shader/gfx-shader-param.cpp"
		"source/gfx/shader/gfx-shader-param-audio.hpp"
		"source/gfx/shader/gfx-shader-param-audio.cpp"
		"source/gfx/shader/gfx-shader-param-basic.hpp"
		"source/gfx/shader/gfx-shader-param-basic.cpp"
		"source/gfx/shader/gfx-shader-param-matrix.hpp"
		"source/gfx/shader/gfx-shader-param-matrix.cpp"
		"source/gfx/shader/gfx-shader-param-texture.hpp"
		"source/gfx/shader/gfx-shader-param-texture.cpp"
	)
endif()

# LUT
if(REQUIRE_LUT)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/gfx/lut/gfx-lut.hpp"
		"source/gfx/lut/gfx-lut.cpp"
		"source/gfx/lut/gfx-lut-consumer.hpp"
		"source/gfx/lut/gfx-lut-consumer.cpp"
		"source/gfx/lut/gfx-lut-producer.hpp"
		"source/gfx/lut/gfx-lut-producer.cpp"
	)
	list(APPEND PROJECT_DATA
		"data/effects/lut.effect"
		"data/effects/lut-consumer.effect"
		"data/effects/lut-producer.effect"
	)
endif()

# Windows
if(D_PLATFORM_WINDOWS)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/windll.cpp"
	)
	list(APPEND PROJECT_LIBRARIES
		Delayimp.lib
	)
	# Disable/Enable a ton of things.
	list(APPEND PROJECT_DEFINITIONS
		# Microsoft Visual C++
		_CRT_SECURE_NO_WARNINGS
		_ENABLE_EXTENDED_ALIGNED_STORAGE
		# windows.h
		WIN32_LEAN_AND_MEAN
		NOGPICAPMASKS
		NOVIRTUALKEYCODES
		#NOWINMESSAGES
		NOWINSTYLES
		NOSYSMETRICS
		NOMENUS
		NOICONS
		NOKEYSTATES
		NOSYSCOMMANDS
		NORASTEROPS
		NOSHOWWINDOW
		NOATOM
		NOCLIPBOARD
		NOCOLOR
		#NOCTLMGR
		NODRAWTEXT
		#NOGDI
		NOKERNEL
		#NOUSER
		#NONLS
		NOMB
		NOMEMMGR
		NOMETAFILE
		NOMINMAX
		#NOMSG
		NOOPENFILE
		NOSCROLL
		NOSERVICE
		NOSOUND
		#NOTEXTMETRIC
		NOWH
		NOWINOFFSETS
		NOCOMM
		NOKANJI
		#NOHELP
		NOPROFILER
		NODEFERWINDOWPOS
		NOMCX
		NOIME
		NOMDI
		NOINOUT
	)
endif()

# GCC before 9.0, Clang before 9.0
if((CMAKE_C_COMPILER_ID STREQUAL "GNU")
	OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	OR (CMAKE_C_COMPILER_ID STREQUAL "Clang")
	OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
		list(APPEND PROJECT_LIBRARIES
			"stdc++fs"
		)
	endif()
endif()

################################################################################
# Register Library
################################################################################

# Combine all variables that matter.
set(PROJECT_FILES
	# Always exist
	${PROJECT_DATA}
	${PROJECT_PRIVATE_GENERATED}
	${PROJECT_PRIVATE_SOURCE}
	${PROJECT_TEMPLATES}
	# UI-only (empty if not enabled)
	${PROJECT_UI}
	${PROJECT_UI_SOURCE}
)

# Set source groups for IDE generators.
source_group(TREE "${PROJECT_SOURCE_DIR}/data" PREFIX "Data" FILES ${PROJECT_DATA})
source_group(TREE "${PROJECT_SOURCE_DIR}/source" PREFIX "Source" FILES ${PROJECT_PRIVATE_SOURCE} ${PROJECT_UI_SOURCE})
source_group(TREE "${PROJECT_BINARY_DIR}/generated" PREFIX "Source" FILES ${PROJECT_PRIVATE_GENERATED})
source_group(TREE "${PROJECT_SOURCE_DIR}/templates" PREFIX "Templates" FILES ${PROJECT_TEMPLATES})
source_group(TREE "${PROJECT_SOURCE_DIR}/ui" PREFIX "User Interface" FILES ${PROJECT_UI})

# Prevent unwanted files from being built as source.
set_source_files_properties(${PROJECT_DATA} ${PROJECT_TEMPLATES} ${PROJECT_UI} PROPERTIES
	HEADER_FILE_ONLY ON
)

# Prevent non-UI files from being Qt'd
if(HAVE_QT)
	set_source_files_properties(${PROJECT_DATA} ${PROJECT_TEMPLATES} ${PROJECT_PRIVATE_GENERATED} ${PROJECT_PRIVATE_SOURCE} PROPERTIES
		SKIP_AUTOGEN ON
		SKIP_AUTOMOC ON
		SKIP_AUTORCC ON
		SKIP_AUTOUIC ON
	)
endif()

# Register the library
add_library(${PROJECT_NAME} MODULE ${PROJECT_FILES}) # We are a module for libOBS.
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE_DIRS})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_DEFINITIONS})
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARIES})

# Set C++ Standard and Extensions
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

# Remove prefix on other platforms.
set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	IMPORT_PREFIX ""
)

# Set file version (on anything but MacOS)
if(NOT D_PLATFORM_MAC)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
endif()

# Delay-Loading on Microsoft Visual C++
if(D_PLATFORM_WINDOWS)
	foreach(DELAYLOAD ${PROJECT_LIBRARIES_DELAYED})
		get_target_property(_lf ${PROJECT_NAME} LINK_FLAGS)
		if (NOT _lf)
			set(_lf "")
		endif()
		set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${_lf} /DELAYLOAD:${DELAYLOAD}")
		add_link_options("/DELAYLOAD:${DELAYLOAD}")
	endforeach()
endif()

# Enable Qt if needed
if(HAVE_QT)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		AUTOUIC ON
		AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR};${PROJECT_SOURCE_DIR}/ui"
		AUTOMOC ON
		AUTORCC ON
		AUTOGEN_BUILD_DIR "${PROJECT_BINARY_DIR}/generated"
	)
endif()

# MacOS: Disable automatic Code Signing in Xcode
if(D_PLATFORM_MAC)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
		XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
	)
endif()

################################################################################
# Extra Tools
################################################################################

# Clang
is_feature_enabled(CLANG T_CHECK)
if(T_CHECK AND HAVE_CLANG)
	generate_compile_commands_json(
		TARGETS ${PROJECT_NAME}
	)
	clang_tidy(
		TARGETS ${PROJECT_NAME}
		VERSION 9.0.0
	)
	clang_format(
		TARGETS ${PROJECT_NAME}
		DEPENDENCY
		VERSION 9.0.0
	)
endif()

# Apple otool
if(D_PLATFORM_MAC)
	# OBS
	mac_get_linker_id(TARGET libobs OUTPUT T_OBS_LINK)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND install_name_tool ARGS -change "${T_OBS_LINK}" "@executable_path/../Frameworks/libobs.0.dylib" $<TARGET_FILE:${PROJECT_NAME}>
	)
	message(STATUS "${LOGPREFIX} Added post-build step for adjusting libobs linking path.")

	# OBS Front-End API
	if(REQUIRE_OBSFE AND HAVE_OBSFE)
		mac_get_linker_id(TARGET obs-frontend-api OUTPUT T_OBSFE_LINK)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND install_name_tool ARGS -change "${T_OBSFE_LINK}" "@executable_path/../Frameworks/libobs-frontend-api.dylib" $<TARGET_FILE:${PROJECT_NAME}>
		)
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting libobs-frontend-api linking path.")
	endif()

	# Qt5
	if(REQUIRE_QT AND HAVE_QT)
		# Figure out the linker location for Qt5::Core
		mac_get_linker_id(TARGET Qt5::Core OUTPUT T_QT5CORE_LINK)

		# Figure out the linker location for Qt5::Gui
		mac_get_linker_id(TARGET Qt5::Gui OUTPUT T_QT5GUI_LINK)

		# Figure out the linker location for Qt5::Widsgets
		mac_get_linker_id(TARGET Qt5::Widgets OUTPUT T_QT5WIDGETS_LINK)

		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		# - QtCore
			COMMAND install_name_tool ARGS -change "${T_QT5CORE_LINK}" "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore" $<TARGET_FILE:${PROJECT_NAME}>
		# - QtGui
			COMMAND install_name_tool ARGS -change "${T_QT5GUI_LINK}" "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui" $<TARGET_FILE:${PROJECT_NAME}>
		# - QtWidgets
			COMMAND install_name_tool ARGS -change "${T_QT5WIDGETS_LINK}" "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets" $<TARGET_FILE:${PROJECT_NAME}>
		)
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting Qt5::Core linking path (Found: ${Qt5_DIR} resolved to ${T_QT5CORE_LINK}).")
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting Qt5::Gui linking path (Found: ${Qt5_DIR} resolved to ${T_QT5GUI_LINK}).")
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting Qt5::Widgets linking path (Found: ${Qt5_DIR} resolved to ${T_QT5WIDGETS_LINK}).")
	endif()
endif()

################################################################################
# Installation
################################################################################

if(${PREFIX}OBS_NATIVE)
	# Grouped builds don't offer standalone services.
	install_obs_plugin_with_data(${PROJECT_NAME} data)
else()
	if(STRUCTURE_UNIFIED)
		install(
			DIRECTORY "data/"
			DESTINATION "data/"
			FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)

		if(D_PLATFORM_WINDOWS)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "bin/windows-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamFX
				LIBRARY DESTINATION "bin/windows-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamFX
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			if(MSVC)
				install(
					FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
					DESTINATION "bin/windows-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/"
					COMPONENT StreamFX
					OPTIONAL
				)
			endif()
		elseif(D_PLATFORM_LINUX)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "bin/linux-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamFX
				LIBRARY DESTINATION "bin/linux-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamFX
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		elseif(D_PLATFORM_MAC)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "bin/mac-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamFX
				LIBRARY DESTINATION "bin/mac-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamFX
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		endif()

		install(
			FILES LICENSE
			DESTINATION "LICENSE"
			COMPONENT StreamFX
		)
		install(
			FILES icon.png
			DESTINATION "icon.png"
			COMPONENT StreamFX
		)
	elseif(D_PLATFORM_WINDOWS)
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/" COMPONENT StreamFX
			LIBRARY DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/" COMPONENT StreamFX
		)
		install(
			DIRECTORY "data/"
			DESTINATION "data/obs-plugins/${PROJECT_NAME}/"
		)
		if(MSVC)
			install(
				FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
				DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/"
				OPTIONAL
			)
		endif()
	elseif(D_PLATFORM_LINUX)
		if(STRUCTURE_PACKAGEMANAGER)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "lib/obs-plugins/" COMPONENT StreamFX
				LIBRARY DESTINATION "lib/obs-plugins/" COMPONENT StreamFX
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			install(
				DIRECTORY "data/"
				DESTINATION "share/obs/obs-plugins/${PROJECT_NAME}"
				COMPONENT StreamFX
				FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
				DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		else()
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "plugins/${PROJECT_NAME}/bin/${D_PLATFORM_BITS}bit/" COMPONENT StreamFX
				LIBRARY DESTINATION "plugins/${PROJECT_NAME}/bin/${D_PLATFORM_BITS}bit/" COMPONENT StreamFX
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			install(
				DIRECTORY "data/"
				DESTINATION "plugins/${PROJECT_NAME}/data/"
				COMPONENT StreamFX
				FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
				DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		endif()
	elseif(D_PLATFORM_MAC)
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "${PROJECT_NAME}/bin/" COMPONENT StreamFX
			LIBRARY DESTINATION "${PROJECT_NAME}/bin/" COMPONENT StreamFX
			PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)
		install(
			DIRECTORY "data/"
			DESTINATION "${PROJECT_NAME}/data/"
			COMPONENT StreamFX
			FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)
	endif()
endif()

################################################################################
# Packaging
################################################################################

if(NOT ${PREFIX}OBS_NATIVE)
	# Packaging
	if(NOT PACKAGE_SUFFIX)
		set(_PACKAGE_SUFFIX_OVERRIDE "${VERSION_STRING}")
	else()
		set(_PACKAGE_SUFFIX_OVERRIDE "${PACKAGE_SUFFIX}")
	endif()
	set(_PACKAGE_FULL_NAME "${PACKAGE_PREFIX}/${PACKAGE_NAME}-${_PACKAGE_SUFFIX_OVERRIDE}")

	if(STRUCTURE_UNIFIED)
		add_custom_target(
			PACKAGE_ZIP
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.obs" --format=zip --
				"."
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
	else()
		add_custom_target(
			PACKAGE_7Z
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
				"."
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
		add_custom_target(
			PACKAGE_ZIP
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
				"."
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)

		# Windows
		if(D_PLATFORM_WINDOWS)
			## Installer (InnoSetup)
			get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)

			get_filename_component(ISS_PACKAGE_DIR "${PACKAGE_PREFIX}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)

			get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)

			get_filename_component(ISS_MSVCHELPER_PATH "${msvc-redist-helper_BUILD_DIR}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_MSVCHELPER_PATH}" ISS_MSVCHELPER_PATH)

			configure_file(
				"templates/installer.iss.in"
				"installer.iss"
			)
		endif()

		# Apple MacOS
		if(D_PLATFORM_MAC)
			# .pkg Installer
			configure_file(
				"templates/installer.pkgproj.in"
				"installer.pkgproj"
				@ONLY
			)
		endif()
	endif()
endif()
