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

#include "httpony/io/network_stream.hpp"

namespace httpony {
namespace io {

InputContentStream::InputContentStream(std::streambuf* buffer, const Headers& headers)
    : std::istream(buffer)
{
    start_input(buffer, headers);
}

bool InputContentStream::start_input(std::streambuf* buffer, const Headers& headers)
{
    rdbuf(buffer);

    /// \todo handle Transfer-Encoding
    std::string length = headers.get("Content-Length");
    std::string content_type = headers.get("Content-Type");

    if ( length.empty() || !std::isdigit(length[0]) || content_type.empty() )
    {
        _content_length = 0;
        _content_type = {};
        rdbuf(nullptr);
        _error = true;
        return false;
    }

    _content_length = std::stoul(length);
    _content_type = std::move(content_type);

    _error = false;
    return true;
}

std::string InputContentStream::read_all(bool preserve_input)
{
    if ( preserve_input )
    {
        std::ostringstream out;
        write_to(out);
        if ( out.str().size() != _content_length )
            _error = true;
        return out.str();
    }

    std::string all(_content_length, ' ');
    read(&all[0], _content_length);

    /// \todo if possible set a low-level failbit when the streambuf finds an error
    if ( std::size_t(gcount()) != _content_length )
    {
        _error = true;
        all.resize(gcount());
    }
    else if ( !eof() && peek() != traits_type::eof() )
    {
        _error = true;
    }

    return all;
}

void OutputContentStream::copy_from(OutputContentStream& other)
{
    other.flush();
    for ( const auto& buf : other.buffer.data() )
    {
        auto data = boost::asio::buffer_cast<const char*>(buf);
        auto size = boost::asio::buffer_size(buf);
        write(data, size);
    }
}

} // namespace io
} // namespace httpony
