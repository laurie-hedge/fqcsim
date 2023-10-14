cmake_minimum_required(VERSION 2.8.12)
cmake_policy(SET CMP0048 NEW)

project(fqcsim
	VERSION 2.8.12
)

set(SOURCES
	src/main.cpp
	src/qsim.cpp
	src/qasm.cpp
	src/qsim_gui.cpp
	external/imgui/imgui.cpp
	external/imgui/imgui_demo.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/imgui_widgets.cpp
	external/implot/implot.cpp
	external/implot/implot_demo.cpp
	external/implot/implot_items.cpp
	external/ImGui-Addons/FileBrowser/ImGuiFileBrowser.cpp
)

add_executable(fqcsim ${SOURCES})

target_compile_features(fqcsim PRIVATE cxx_std_20)

target_include_directories(fqcsim
	PRIVATE
		inc/
		external/imgui
		external/imgui/backends
		external/implot
		external/ImGui-Addons/FileBrowser
)

if (WIN32)
	target_include_directories(fqcsim
		PRIVATE
			${WindowsSdkDir}/Include/um
			${WindowsSdkDir}/Include/shared
			${DXSDK_DIR}/Include
	)

	target_sources(fqcsim
		PRIVATE
			src/platform_win32.cpp
			external/imgui/backends/imgui_impl_dx11.cpp
			external/imgui/backends/imgui_impl_win32.cpp
	)

	target_link_libraries(fqcsim
		PRIVATE
			comdlg32.lib
			d3d11.lib
			d3dcompiler.lib
	)
elseif (UNIX)
	find_package(SDL2 REQUIRED)

	target_include_directories(fqcsim
		PRIVATE
			${SDL2_INCLUDE_DIRS}
	)

	target_sources(fqcsim
		PRIVATE
			src/platform_linux.cpp
			external/imgui/backends/imgui_impl_opengl3.cpp
			external/imgui/backends/imgui_impl_sdl2.cpp
	)

	target_link_libraries(fqcsim
		PRIVATE
			${SDL2_LIBRARIES}
			-lGL
	)
endif ()

add_subdirectory(external/Catch2)
add_subdirectory(test)
