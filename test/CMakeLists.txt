set(SOURCES
	../src/qasm.cpp
	../src/qsim.cpp
)

set(TEST_SOURCES
	test_qasm.cpp
	test_qsim.cpp
)

add_executable(test_fqcsim ${SOURCES} ${TEST_SOURCES})

target_compile_features(test_fqcsim PRIVATE cxx_std_17)

target_include_directories(test_fqcsim
	PUBLIC
		../inc
		../external/Catch2/src
		${CMAKE_BINARY_DIR}/external/Catch2/generated-includes
)

target_link_libraries(test_fqcsim 
	PRIVATE
		Catch2::Catch2WithMain
)
