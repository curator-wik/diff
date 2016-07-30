cmake_minimum_required (VERSION 2.8)
project (php_git)

set (CMAKE_BUILD_TYPE Debug)
set (php_git_VERSION_MAJOR 0)
set (php_git_VERSION_MINOR 1)

set (CMAKE_C_FLAGS -fPIC)

find_package(PkgConfig)

pkg_check_modules(
	LIBGIT2 REQUIRED
	libgit2
)

if(${LIBGIT2_VERSION} VERSION_LESS 0.22.0)
	message(FATAL_ERROR "${LIBGIT2_VERSION} is too old")
endif()

link_directories(
	${LIBGIT2_LIBRARY_DIRS}
)

include_directories(
	${LIBGIT2_INCLUDE_DIRS}
)

add_executable(main main.c)
target_link_libraries(main git2)
