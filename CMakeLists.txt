#
# Copyright 2016 Mattia Basaglia
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0009 NEW)
cmake_policy(SET CMP0048 NEW)
set(NICE_NAME HttPony)
project(${NICE_NAME} VERSION 0.1 LANGUAGES CXX)
set(PROJECT_DESCRIPTION "A C++ HTTP Server library")
string(TOLOWER ${NICE_NAME} LIBRARY_NAME)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(misc)

# C++ Standard
use_cxx_standard(14)

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Werror --coverage")

# Enable Debug by default, can be changed with -D CMAKE_BUILD_TYPE=Release
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE Debug)

    execute_process(COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(GIT_VERSION)
        set(PROJECT_VERSION_LONG "${PROJECT_VERSION}-git-${GIT_VERSION}")
        add_custom_target(version_git ALL
            COMMAND [ "`git rev-parse --short HEAD`" != "${GIT_VERSION}" ] && ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR}
            VERBATIM
        )
    endif()
endif()


include_directories("lib/Melanolib/include/")
include_directories(include)
add_subdirectory(src lib)

if(NOT TARGET melano)
    message(STATUS "Including Melanolib")
    add_subdirectory(lib/Melanolib/src EXCLUDE_FROM_ALL)
endif()

add_subdirectory(example EXCLUDE_FROM_ALL)

# Find all sources for documentation and stuff
set(ALL_SOURCE_DIRECTORIES src include)

find_sources(ALL_SOURCES *.cpp *.hpp)
set(DOXYGEN_FOUND ON)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    create_doxygen_target(doc)
endif(DOXYGEN_FOUND)

# Tests
set(LCOV_SOURCE_BASE_DIR "${CMAKE_SOURCE_DIR}")
set(LCOV_REMOVE_PATTERN "lib/*.cpp" "lib/*.hpp" "test_*.cpp")
include(testing)
add_subdirectory(test)
