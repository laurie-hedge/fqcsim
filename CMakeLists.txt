cmake_minimum_required(VERSION 2.8.11)

project(fqcsim)

set(SOURCES
	src/main.cpp
	src/qsim.cpp
	src/qasm.cpp
	src/qsim_gui.cpp
	external/imgui/backends/imgui_impl_dx11.cpp
	external/imgui/backends/imgui_impl_win32.cpp
	external/imgui/imgui.cpp
	external/imgui/imgui_demo.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/imgui_widgets.cpp
	external/implot/implot.cpp
	external/implot/implot_demo.cpp
	external/implot/implot_items.cpp
)

add_executable(fqcsim ${SOURCES})

target_compile_features(fqcsim PRIVATE cxx_std_17)

target_include_directories(fqcsim
	PRIVATE
		inc/
		external/imgui
		external/imgui/backends
		external/implot
		${WindowsSdkDir}/Include/um
		${WindowsSdkDir}/Include/shared
		${DXSDK_DIR}/Include
)

if (WIN32)
	target_sources(fqcsim
		PRIVATE src/platform_win32.cpp
	)

	target_link_libraries(fqcsim
		PRIVATE
			comdlg32.lib
			d3d11.lib
			d3dcompiler.lib
	)
endif()
