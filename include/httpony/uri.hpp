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

#ifndef HTTPONY_URI_HPP
#define HTTPONY_URI_HPP

#include "httpony/http/headers.hpp"

namespace httpony {


std::string urlencode(const std::string& input, bool plus_spaces = false);
std::string urldecode(const std::string& input, bool plus_spaces = false);

/**
 * \brief Class representing paths.
 *
 * Paths segments are separated by forward slashes
 * \todo Test
 */
class Path
{
public:
// STL stuff
    using container_type = std::vector<std::string>;
    using value_type = container_type::value_type;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;

    Path(container_type data)
        : data(std::move(data))
    {}

    Path(std::initializer_list<value_type> data)
        : data(data)
    {}

    template<class InputIterator>
    Path(InputIterator&& first, InputIterator&& last)
        : data(std::forward<InputIterator>(first), std::forward<InputIterator>(last))
    {}

    Path() = default;
    Path(const Path&) = default;
    Path(Path&&) = default;

    Path& operator=(const Path&) = default;
    Path& operator=(Path&&) = default;
    Path& operator=(std::initializer_list<value_type> data)
    {
        this->data = data;
        return *this;
    }

    auto begin()        { return data.begin(); }
    auto end()          { return data.end();   }
    auto begin() const  { return data.begin(); }
    auto end() const    { return data.end();   }
    auto cbegin() const { return data.cbegin();}
    auto cend() const   { return data.cend();  }

    auto rbegin()        { return data.rbegin(); }
    auto rend()          { return data.rend();   }
    auto rbegin() const  { return data.rbegin(); }
    auto rend() const    { return data.rend();   }
    auto crbegin() const { return data.crbegin();}
    auto crend() const   { return data.crend();  }

    reference back() { return data.back(); }
    const_reference back() const { return data.back(); }

    reference front() { return data.front(); }
    const_reference front() const { return data.front(); }

    auto empty() const { return data.empty(); }
    auto size() const { return data.size(); }
    auto max_size() const { return data.max_size(); }

    void clear()
    {
        data.clear();
    }

// Extra ctors
    Path(const_reference path, bool url_decode = false)
    {
        auto segments = melanolib::string::char_split(path, '/');
        for ( auto& segment : segments )
        {
            if ( segment == ".." && !data.empty() )
                data.pop_back();
            else if ( segment != "." )
                data.push_back(url_decode ? urldecode(segment) : segment);
        }
    }

// Concat
    Path parent() const
    {
        if ( !empty() )
            return Path(begin(), end() -1);
        return Path();
    }

    Path child(const std::string& child) const
    {
        return *this / child;
    }

    Path& operator+=(const Path& path)
    {
        data.insert(data.end(), path.begin(), path.end());
        return *this;
    }


    Path& operator+=(const_reference path)
    {
        return *this += Path(path);
    }

    Path& operator/=(const_reference path)
    {
        return *this += path;
    }

    Path& operator/=(const Path& path)
    {
        return *this += path;
    }

    Path operator+(const Path& path) const
    {
        auto copy = *this;
        copy += path;
        return copy;
    }

    Path operator+(const_reference path) const
    {
        auto copy = *this;
        copy += path;
        return copy;
    }

    friend Path operator+(const_reference a, const Path& b)
    {
        return Path(a) + b;
    }


    Path operator/(const Path& path) const
    {
        return *this + path;
    }

    Path operator/(const_reference path) const
    {
        return *this + path;
    }

    friend Path operator/(const_reference a, const Path& b)
    {
        return Path(a) + b;
    }
// To string
    /**
     * \brief Converts the path to a string
     */
    std::string string() const
    {
        return melanolib::string::implode("/", data);
    }

    /**
     * \brief Converts the path to a string, url-encoding each segment
     * This will \b not convert / to %2F
     */
    std::string url_encoded() const
    {
        std::string result;
        for ( const auto& segment : data )
            result += '/' + urlencode(segment);
        return result;
    }

    /**
     * \brief Converts the path to an urlencoded string
     * This will also convert / to %2F
     */
    std::string full_url_encoded() const
    {
        return urlencode(string());
    }


private:
    container_type data;
};

/**
 * \brief Uniform resource identifier
 * \see http://www.faqs.org/rfcs/rfc3986.html
 */
struct Uri
{

    Uri(const char* uri)
        : Uri(std::string(uri))
    {}
    Uri(const std::string& uri);
    Uri() = default;
    Uri(
        std::string scheme,
        std::string authority,
        Path path,
        DataMap query,
        std::string fragment
    );

    std::string full() const;

    std::string query_string(bool question_mark = false) const;

    std::string scheme;
    std::string authority;
    Path path;
    DataMap query;
    std::string fragment;

    /**
     * \brief URI equivalence
     */
    bool operator==(const Uri& oth) const;

    bool operator!=(const Uri& oth) const
    {
        return !(*this == oth);
    }
};

DataMap parse_query_string(const std::string& str);

std::string build_query_string(const DataMap& headers, bool question_mark = false);

} // namespace httpony
#endif // HTTPONY_URI_HPP