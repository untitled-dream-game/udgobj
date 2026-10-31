cmake_minimum_required(VERSION 3.13.5)

project(udgObj VERSION 0.1 LANGUAGES C)

option(UDGOBJ_BUILD_EXAMPLES "Build the example project" ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(udgObj
	src/udgobj.c
)

target_include_directories(udgObj
	PUBLIC
	"${PROJECT_SOURCE_DIR}/include"
)

if (UDGOBJ_BUILD_EXAMPLES)
	add_executable(udgObjExample
		examples/src/main.c
	)

	target_link_libraries(udgObjExample PRIVATE udgObj)
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/examples/resources 
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
	)

	install(TARGETS udgObjExample udgObj
		DESTINATION bin
	)
endif()