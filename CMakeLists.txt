cmake_minimum_required(VERSION 3.14)

project(Honey LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(INSTALL_DEVEL_DOCS "Install development documentation" OFF)
option(INSTALL_DEVEL_HEADERS "Install development headers" OFF)
option(INSTALL_USAGE_DOCS "Install usage-related documentation (manpages)" ON)

# Honey sources related variables
set(CONFIG_HNY_EXTRACTION_BUFFERSIZE_DEFAULT 4096 CACHE STRING "Honey extraction default internal buffer size")
set(CONFIG_HNY_EXTRACTION_BUFFERSIZE_MIN 512 CACHE STRING "Honey extraction minimal internal buffer size")
set(CONFIG_HNY_EXTRACTION_DICTIONARYMAX_DEFAULT UINT32_MAX CACHE STRING "Honey LZMA2 dictionary max size default")
set(CONFIG_HNY_EXTRACTION_CPIO_FILENAME_DEFAULT_CAPACITY 32 CACHE STRING "Honey default filename buffer capacity when extracting cpio")
set(CONFIG_HNY_REMOVE_DIRSTACK_DEFAULT_CAPACITY 10 CACHE STRING "Honey remove directory stack default capacity")
set(CONFIG_HNY_STATUS_BUFFER_DEFAULT_CAPACITY 120 CACHE STRING "Honey status readlink buffer default capacity")

# Configurations
configure_file(include/config.h.in include/config.h @ONLY)
include_directories(${PROJECT_BINARY_DIR}/include include)

# Target definitions and properties

file(GLOB_RECURSE HNY_SOURCES CONFIGURE_DEPENDS src/hny/*.c)
add_executable(hny ${HNY_SOURCES})

file(GLOB_RECURSE LIBHNY_SOURCES CONFIGURE_DEPENDS src/libhny/*.c)
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

install(TARGETS hny libhny RUNTIME LIBRARY ARCHIVE)

if(INSTALL_DEVEL_DOCS AND TARGET doc)
	install(DIRECTORY ${PROJECT_BINARY_DIR}/html TYPE DOC)
endif()

if(INSTALL_DEVEL_HEADERS)
	install(TARGETS libhny PUBLIC_HEADER)
endif()

find_program(HVN_MAN hvn-man)
# hvn-man is required to build manpages
if(INSTALL_USAGE_DOCS AND HVN_MAN)
	add_custom_target(man1 ALL DEPENDS man1/hny.1)
	add_custom_command(OUTPUT man1/hny.1 COMMAND mkdir -p man1 COMMAND ${HVN_MAN} -o man1/hny.1 -i ${PROJECT_SOURCE_DIR}/docs/hny.1.md)

	install(DIRECTORY ${PROJECT_BINARY_DIR}/man1 TYPE MAN)
endif()

