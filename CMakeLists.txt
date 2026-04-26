#=============================================================================
# MedVoxel3D - Root CMakeLists.txt
#=============================================================================
#
# WHAT THIS FILE DOES?
# 1. Sets project-wide rules (C++ version, compiler warnings)
# 2. Downloads external dependencies (GLFW, GLM) automatically
# 3. Finds system-installed OpenGL.
# 4. Collects all our source files.
# 5. Builds a single executable called MedVoxel3D
# 6. Copies shader/assets to the build directory so the exe can find them
# 

#------------------------------------------------------------------------------
#cmake_minimum_required: Tells CMake "dont run if you're older than 3.20"
# 
# WHY 3.20?
# - FetchContent improvements (FETCONTENT_UPDATES_DISCONNECTED support)
# - cmake_path() command for cross-platfrom path handling
# - Better presets support (CMakePresets.json)
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(MI_3D
		VERSION 0.1.0
		LANGUAGES C CXX
)

#------------------------------------------------------------------------------
# C++ STANDARD: C++17, required, no extensions,
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#------------------------------------------------------------------------------
# Output directories: Put all build outputs in predictable locations.
#
# Without this, on multi-config generators (Visual Studio, Xcode), the exe
# ends up in build/Debug/ or build/Release/. On Makefiles, it's just build/.
# That inconsistency means shaders at "../shaders/" resolve differently
# depending on the generator. This forces everything to build/ root.
#
# WHY does this matter? When our Shader class loads "shaders/volume.frag",
# it uses a path relative to the exe. If the exe moves around per-config,
# the relative path breaks. Pinning the output location prevents that.
# --------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#------------------------------------------------------------------------------
# Dependencies: FetchContent downloads at configure time.
include(FetchContent)

# --------- GLFW 3.4 ---------
FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG      3.4
	GIT_SHALLOW  ON
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# --------- GLM 1.0.1 ---------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    ON
)

# --------- FETCHCONTENT AVAILABLE -------------
FetchContent_MakeAvailable(glfw glm)

# --------- OpenGL ---------
# for UBUNTU if this fails: sudo apt install libg11-mesa-dev
# for Windows, GPU drivers include OpenGL. It should be always be found
find_package(OpenGL REQUIRED)

#------------------------------------------------------------------------------
set(MI_SOURCES
	# --------- Entry Point ---------
	src/main.cpp

	# --------- Core files ---------
	 src/core/Application.cpp
	src/core/Window.cpp
	 src/core/Camera.cpp

	# --------- Renderer files ---------
	 src/renderer/shader.cpp
	# src/renderer/Renderer.cpp
	# src/renderer/VolumeRenderer.cpp
	# src/renderer/SliceRenderer.cpp

	# --------- Medical files ---------
	src/medical/DicomParser.cpp
	src/medical/VolumeData.cpp
	# src/medical/TransferFunctionEditor.cpp

	# --------- UI files ---------
	src/ui/UIManager.cpp
	# src/ui/TransferFunctionEditor.cpp

	# --------- External ---------
	external/glad/src/glad.c
	external/imgui/imgui.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/imgui_widgets.cpp
	external/imgui/backends/imgui_impl_glfw.cpp
	external/imgui/backends/imgui_impl_opengl3.cpp

)

# --------------------------------------------------------------------------
# add_executable: Creates the build target.
add_executable(${PROJECT_NAME} ${MI_SOURCES} "src/core/Camera.h")

# --------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/imgui
	${CMAKE_SOURCE_DIR}/external/glad/include

)

# --------------------------------------------------------------------------
# Link libraries: Connect our exe to the dependency targets.
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm::glm
    OpenGL::GL
)

# --------------------------------------------------------------------------
# Compiler Warnings: Catch Bugs at a compile time.

# we will use for GCC/Clang (-Wall -Wextra -Wpendatic) or MSVC (/W4)

# since we are building cross platform application we are using platform specific issues early

target_compile_options(${PROJECT_NAME} PRIVATE
$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
$<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# -------------------- PASSING ASSET AS PART OF BUILD -------------------------
# POST BUILD COMMAND
# AGENDA - TO MAKE CROSS PLATFORM COMPATIBLE
# using cmake -E copy_directory
# every time we build it evaluate if there is any thing different

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/shaders
	${CMAKE_BINARY_DIR}/shaders
    COMMENT "COPYING SHADERS TO BUILD DIRECTORY"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/assets
	${CMAKE_BINARY_DIR}/assets
    COMMENT "COPYING SHADERS TO BUILD DIRECTORY"
)

# --------------------------------------------------------------------------
# Print a configuration summary so you can verify everything is right.
# This prints once when you run cmake -B build.
# --------------------------------------------------------------------------

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "  C++ Standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform:      ${CMAKE_SYSTEM_NAME}")
message(STATUS "========================================")
message(STATUS "")



