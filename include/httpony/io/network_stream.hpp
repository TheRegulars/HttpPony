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

#ifndef HTTPONY_NETWORK_STREAM_HPP
#define HTTPONY_NETWORK_STREAM_HPP

/// \cond
#include <iostream>
/// \endcond

#include "httpony/mime_type.hpp"
#include "httpony/http/headers.hpp"
#include "httpony/io/buffer.hpp"

namespace httpony {
namespace io {

/**
 * \brief Reads an incoming message payload
 * \todo Maybe instead of allowing arbitrary std::streambuf pointers
 *       only boost::asio::streambuf pointers should be allowed.
 *       This would simplify the logic for write_to/read_all.
 */
class InputContentStream : public std::istream
{
public:
    explicit InputContentStream(std::streambuf* buffer, const Headers& headers);

    InputContentStream()
        : std::istream(nullptr)
    {}

    InputContentStream(InputContentStream&& other)
        : std::istream(other.rdbuf()),
          _content_length(other._content_length),
          _content_type(std::move(other._content_type)),
          _error(other._error)
    {}


    InputContentStream& operator=(InputContentStream&& other)
    {
        rdbuf(other.rdbuf());
        _content_length = other._content_length;
        std::swap(_content_type, other._content_type);
        std::swap(_error, other._error);
        return *this;
    }

    /**
     * \brief Sets up the stream to read from the given buffer
     * \returns \b true on success
     */
    bool start_input(std::streambuf* buffer, const Headers& headers);

    /**
     * \brief Whether there is some data to read (which might have 0 length) or no data at all
     */
    bool has_data() const
    {
        return !_error && rdbuf();
    }

    /**
     * \brief Whether the stream encountered an error
     */
    bool has_error() const
    {
        return fail() || _error;
    }

    explicit operator bool() const
    {
        return !_error && !fail();
    }

    bool operator!() const
    {
        return _error || fail();
    }

    /**
     * \brief Expected size of the input, as advertised by the headers
     * passed to start_input()
     */
    std::size_t content_length() const
    {
        return _content_length;
    }

    /**
     * \brief Reads all available data and returns it as a string
     * \param preserve_input    If \b true, the read characters will not
     *                          be extracted from the stream
     */
    std::string read_all(bool preserve_input = false);

    /**
     * \brief Content type, as advertised by the headers passed to start_input()
     */
    MimeType content_type() const
    {
        return _content_type;
    }

    /**
     * \brief Writes the payload to a stream
     */
    void write_to(std::ostream& output)
    {
        if ( has_data() )
        {
            if ( boost::asio::streambuf* buffer = dynamic_cast<boost::asio::streambuf*>(rdbuf()) )
            {
                for ( const auto& buf : buffer->data() )
                {
                    auto data = boost::asio::buffer_cast<const char*>(buf);
                    auto size = boost::asio::buffer_size(buf);
                    if ( !output.write(data, size) )
                        return;
                }
            }
            else
            {
                rdbuf()->pubseekpos(0);
                output << rdbuf();
                rdbuf()->pubseekpos(0);
            }
        }
    }

private:
    std::size_t _content_length = 0;
    MimeType _content_type;
    bool _error = false;
};

/**
 * \brief Writes an outgoing message payload
 */
class OutputContentStream: public std::ostream
{
public:
    explicit OutputContentStream(MimeType content_type)
        : std::ostream(&buffer), _content_type(std::move(content_type))
    {
    }

    OutputContentStream(OutputContentStream&& other)
        : std::ostream(other.rdbuf() ? &buffer : nullptr),
          _content_type(std::move(other._content_type))
    {
        if ( has_data() )
            copy_from(other);
    }

    OutputContentStream& operator=(OutputContentStream&& other)
    {
        stop_output();
        if ( other.has_data() )
        {
            rdbuf(other.rdbuf() ? &buffer : nullptr);
            copy_from(other);
        }
        _content_type = std::move(other._content_type);
        return *this;
    }

    OutputContentStream()
        : std::ostream(nullptr)
    {
    }

    ~OutputContentStream()
    {
        flush();
        rdbuf(nullptr);
    }

    /**
     * \brief Sets up the stream to contain data in the specified encoding
     */
    void start_output(const MimeType& content_type)
    {
        rdbuf(&buffer);
        _content_type = content_type;
    }

    void start_output(const std::string& content_type)
    {
        rdbuf(&buffer);
        _content_type = content_type;
    }

    /**
     * \brief Removes all data from the stream, call start() to re-introduce it
     */
    void stop_output()
    {
        flush();
        buffer.consume(buffer.size());
        rdbuf(nullptr);
    }

    /**
     * \brief Whether there is some data to send (which might have 0 length) or no data at all
     */
    bool has_data() const
    {
        return rdbuf() && _content_type.valid();
    }

    std::size_t content_length() const
    {
        return buffer.size();
    }

    MimeType content_type() const
    {
        return _content_type;
    }

    /**
     * \brief Writes the payload to a stream
     */
    void write_to(std::ostream& output)
    {
        if ( has_data() )
        {
            for ( const auto& buf : buffer.data() )
            {
                auto data = boost::asio::buffer_cast<const char*>(buf);
                auto size = boost::asio::buffer_size(buf);
                if ( !output.write(data, size) )
                    return;
            }
        }
    }

private:
    void copy_from(OutputContentStream& other);

    boost::asio::streambuf buffer;
    MimeType _content_type;
};

/**
 * Stream than can be used either for input or for output
 */
class ContentStream : public std::iostream
{
public:
    enum class OpenMode
    {
        None,
        Input,
        Output
    };

    ContentStream()
        : ContentStream(OpenMode::None)
    {}

    explicit ContentStream(OpenMode mode)
    {
        set_mode(mode);
    }

    ContentStream(ContentStream&& oth)
        : _input(std::move(oth.input())),
          _output(std::move(oth.output())),
          _mode(oth._mode)
    {
        set_mode(oth._mode);
    }

    ContentStream& operator=(ContentStream&& oth)
    {
        _output = std::move(oth.output());
        _input = std::move(oth.input());
        set_mode(oth._mode);
        return *this;
    }

// Input
    bool start_input(std::streambuf* buffer, const Headers& headers)
    {
        if ( _mode == OpenMode::Output )
            return false;
        bool ok = _input.start_input(buffer, headers);
        set_mode(OpenMode::Input);
        return ok;
    }

    std::string read_all(bool preserve_input = false)
    {
        if ( _mode == ContentStream::OpenMode::Input )
            return _input.read_all(preserve_input);

        if ( _mode == ContentStream::OpenMode::Output )
        {
            std::ostringstream out;
            _output.write_to(out);
            return out.str();
        }

        return "";
    }

// Output
    bool start_output(const MimeType& content_type)
    {
        if ( _mode == OpenMode::Input )
            return false;
        _output.start_output(content_type);
        set_mode(OpenMode::Output);
        return true;
    }

    bool start_output(const std::string& content_type)
    {
        return start_output(MimeType(content_type));
    }

    bool stop_output()
    {
        if ( _mode != OpenMode::Output )
            return false;
        _output.stop_output();
        return true;
    }

// Both
    bool has_data() const
    {
        if ( _mode == ContentStream::OpenMode::Input )
            return _input.has_data();
        if ( _mode == ContentStream::OpenMode::Output )
            return _output.has_data();
        return false;
    }

    bool has_error() const
    {
        if ( _mode == ContentStream::OpenMode::Input )
            return _input.has_error();
        if ( _mode == ContentStream::OpenMode::Output )
            return _output.fail();
        return false;
    }

    explicit operator bool() const
    {
        return !has_error();
    }

    bool operator!() const
    {
        return has_error();
    }

    std::size_t content_length() const
    {
        if ( _mode == ContentStream::OpenMode::Input )
            return _input.content_length();
        if ( _mode == ContentStream::OpenMode::Output )
            return _output.content_length();
        return 0;
    }

    MimeType content_type() const
    {
        if ( _mode == ContentStream::OpenMode::Input )
            return _input.content_type();
        if ( _mode == ContentStream::OpenMode::Output )
            return _output.content_type();
        return {};
    }

    /**
     * \brief Writes the payload to a stream
     */
    void write_to(std::ostream& output)
    {
        if ( _mode == ContentStream::OpenMode::Input )
            _input.write_to(output);
        if ( _mode == ContentStream::OpenMode::Output )
            _output.write_to(output);
    }

// Extra
    OpenMode mode() const
    {
        return _mode;
    }

    bool has_input() const
    {
        return _mode == ContentStream::OpenMode::Input && has_data();
    }

    bool has_output() const
    {
        return _mode == ContentStream::OpenMode::Output && has_data();
    }

    InputContentStream& input()
    {
        return _input;
    }

    OutputContentStream& output()
    {
        return _output;
    }

private:
    void set_mode(OpenMode mode)
    {
        _mode = mode;
        if ( _mode == ContentStream::OpenMode::Output )
            rdbuf(_output.rdbuf());
        else if ( _mode == ContentStream::OpenMode::Input )
            rdbuf(_input.rdbuf());
        else
            rdbuf(nullptr);
    }

    InputContentStream _input;
    OutputContentStream _output;
    OpenMode _mode;
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_NETWORK_STREAM_HPP
