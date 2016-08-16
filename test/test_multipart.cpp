/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define BOOST_TEST_MODULE HttPony_MimeType
#include <boost/test/unit_test.hpp>

#include "httpony/multipart.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_multipart )
{
    Multipart mp("foo");
    mp.parts.push_back({{}, ""});
    BOOST_CHECK(mp.boundary == "foo");
    BOOST_CHECK(mp.parts.size() == 1);
}
