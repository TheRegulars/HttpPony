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

#ifndef HTTPONY_MULTIPART_HPP
#define HTTPONY_MULTIPART_HPP

#include "httpony/http/headers.hpp"

namespace httpony {

/**
 * \brief Multipart content data
 * \see https://tools.ietf.org/html/rfc2046#section-5.1
 */
struct Multipart
{
    /**
     * \brief A part of the multipart data
     */
    struct Part
    {
        Headers headers;
        std::string content;
    };

    explicit Multipart(std::string boundary)
        : boundary(std::move(boundary))
    {}
    
    std::string boundary;
    std::vector<Part> parts;
};

} // namespace httpony
#endif // HTTPONY_MULTIPART_HPP
