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

example(ssl_server)
target_link_libraries(ssl_server ${OPENSSL_LIBRARIES})

example(ssl_client)
target_link_libraries(ssl_client ${OPENSSL_LIBRARIES})

example(ssl_ping_pong)
target_link_libraries(ssl_ping_pong ${OPENSSL_LIBRARIES})

example(maybe_ssl_server)
target_link_libraries(maybe_ssl_server ${OPENSSL_LIBRARIES})
