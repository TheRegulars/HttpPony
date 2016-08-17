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
include(CTest)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address --coverage -O0 -fno-inline")

# Boost
find_package (Boost COMPONENTS unit_test_framework QUIET)
if(Boost_FOUND)
    include_directories (${Boost_INCLUDE_DIRS})
    add_definitions (-DBOOST_TEST_DYN_LINK)
    set(BUILD_TESTING ON)
else()
    set(BUILD_TESTING OFF)
endif()

#Tests
if (BUILD_TESTING)
    message(STATUS "Test targets enabled")
    enable_testing()

    # Example:
    # melanotest(test_foo
    #     ${CMAKE_SOURCE_DIR}/extra_file.cpp
    # )
    # target_link_libraries(test_foo some_library)
    function(melanotest test_name)
        add_executable(${test_name} EXCLUDE_FROM_ALL ${test_name}.cpp ${ARGN})
        target_link_libraries(${test_name} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
        add_test (NAME ${test_name} COMMAND ${test_name})
        add_dependencies(tests_compile ${test_name})
    endfunction(melanotest)

    set(COMMON_LIBRARIES httpony melano_stringutils melano_time)

    melanotest(test_uri)
    target_link_libraries(test_uri ${COMMON_LIBRARIES})

    melanotest(test_mime_type)
    target_link_libraries(test_mime_type ${COMMON_LIBRARIES})

    melanotest(test_base_encoding)
    target_link_libraries(test_base_encoding ${COMMON_LIBRARIES})

    melanotest(test_protocol)
    target_link_libraries(test_protocol ${COMMON_LIBRARIES})

    melanotest(test_quick_xml)

    melanotest(test_multipart)

    melanotest(test_streams)
    target_link_libraries(test_streams ${COMMON_LIBRARIES})

endif()
