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
set(LIBRARY_NAME httpony) # repeated otherwise kdevelop messes up

configure_file(util/version.in.cpp ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

set(SOURCES
http/agent/server.cpp
http/agent/client.cpp
http/parser.cpp
http/post.cpp
http/protocol.cpp
http/request.cpp
http/status.cpp
io/buffer.cpp
io/network_stream.cpp
io/socket.cpp
mime_type.cpp
uri.cpp
${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

if(HTTPONY_SHARED)
    add_library(${LIBRARY_NAME} SHARED ${SOURCES})
else()
    add_library(${LIBRARY_NAME} STATIC ${SOURCES})
endif()


find_package (Boost COMPONENTS system thread REQUIRED)
target_link_libraries(${LIBRARY_NAME} ${Boost_LIBRARIES})
include_directories(${Boost_INCLUDE_DIRS})


target_link_libraries(${LIBRARY_NAME} melano_stringutils melano_time)
