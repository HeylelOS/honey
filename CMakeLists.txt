cmake_minimum_required(VERSION 3.14)

project(Honey LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE HNY_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/hny/*.c)
add_executable(hny ${HNY_SOURCES})

file(GLOB_RECURSE LIBHNY_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/libhny/*.c)
add_library(libhny ${LIBHNY_SOURCES})

target_link_libraries(hny PUBLIC libhny)

set_target_properties(libhny PROPERTIES
	OUTPUT_NAME hny
	PUBLIC_HEADER include/hny.h
)

#################
# Documentation #
#################

find_package(Doxygen)

# We need doxygen with markdown support
if(DOXYGEN_FOUND AND DOXYGEN_VERSION VERSION_GREATER_EQUAL 1.8.0)
	set(DOXYGEN_EXTRACT_ALL YES)
	set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)

	doxygen_add_docs(doc
		include/hny.h
		docs/file-format.md
		docs/prefix-format.md
		docs/hny.1.md
	)
endif()

########
# Test #
########

enable_testing()
add_subdirectory(test)

###########
# Install #
###########

install(
	TARGETS hny libhny
	RUNTIME LIBRARY ARCHIVE PUBLIC_HEADER
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/man/man1 TYPE MAN)

if(TARGET doc)
	install(DIRECTORY ${PROJECT_BINARY_DIR}/html TYPE DOC)
endif()

