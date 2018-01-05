project( matchstick )
cmake_minimum_required( VERSION 3.7.2 )

include(CMakeToolsHelpers OPTIONAL)

set ( SOURCES
	src/main.cpp
)

add_executable( ${CMAKE_PROJECT_NAME} ${SOURCES} )
target_include_directories( ${CMAKE_PROJECT_NAME} PRIVATE src )
set_property( TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 17 )
set_property( TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED 17 )

