include(ExternalProject)
cmake_minimum_required (VERSION 2.8)
project (php_git)

set(CMAKE_BUILD_TYPE Debug)
set(php_git_VERSION_MAJOR 0)
set(php_git_VERSION_MINOR 1)

set(GLOBAL_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/install)
set (CMAKE_C_FLAGS -fPIC)

find_package(PkgConfig)

pkg_check_modules(
	LIBGIT2 REQUIRED
	libgit2
)

if(${LIBGIT2_VERSION} VERSION_LESS 0.22.0)
	message(FATAL_ERROR "${LIBGIT2_VERSION} is too old")
endif()

ExternalProject_Add(
	libdmp-build

	GIT_REPOSITORY "https://github.com/arrbee/diff-match-patch-c.git"

	SOURCE_DIR "${CMAKE_SOURCE_DIR}/depends/libdmp"

	PATCH_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make -f ${CMAKE_SOURCE_DIR}/depends/libdmp/Makefile
	UPDATE_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND ""
)


add_library(dmp STATIC IMPORTED)

set_target_properties(dmp PROPERTIES
	IMPORTED_LOCATION ./libdmp-build-prefix/src/libdmp-build-build/libdmp.a)

add_executable(main main.c)
add_dependencies(main libdmp-build dmp)
target_include_directories(main PUBLIC ${CMAKE_SOURCE_DIR}/depends/libdmp/include)
target_link_libraries(main git2 dmp)

add_custom_target(clean-all
	COMMAND rm Makefile
	COMMAND rm -r CMakeFiles
	COMMAND rm -rf depends
	COMMAND rm -r libdmp-build-prefix
	COMMAND rm CMakeCache.txt cmake_install.cmake
	COMMAND rm -r bin
)
