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

/// \cond
#include <melanolib/utils/c++-compat.hpp>
#include <melanolib/math/math.hpp>
/// \endcond

#include "httpony/http/headers.hpp"
#include "httpony/ip_address.hpp"

namespace httpony {


std::string urlencode(const std::string& input, bool plus_spaces = false);
std::string urldecode(const std::string& input, bool plus_spaces = false);

/**
 * \brief Class representing paths.
 *
 * Paths segments are separated by forward slashes
 * \todo Test
 *
 * \todo Path should distinguish /foo/bar/ from /foo/bar
 *       Perhaps this can be achieved by appending an empty string for filename
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

    const_reference operator[](size_type pos) const { return data[pos]; }
    reference operator[](size_type pos) { return data[pos]; }

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

    Path(value_type::const_pointer path, bool url_decode = false)
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

    friend Path operator+(const_reference a, const Path& b)
    {
        return Path(a) + b;
    }


    Path operator/(const Path& path) const
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
     * \param empty_root Whether to add the initial "/" if the path is empty
     */
    std::string string(bool empty_root = false) const
    {
        if ( empty_root && empty() )
            return "/";
        return "/" + melanolib::string::implode("/", data);
    }

    /**
     * \brief Converts the path to a string, url-encoding each segment
     * This will \b not convert / to %2F
     * \param empty_root Whether to add the initial "/" if the path is empty
     */
    std::string url_encoded(bool empty_root = false) const
    {
        if ( empty_root && empty() )
            return "/";

        std::string result;
        for ( const auto& segment : data )
            result += '/' + urlencode(segment);
        return result;
    }

    /**
     * \brief Converts the path to an urlencoded string
     * This will also convert / to %2F
     * \param empty_root Whether to add the initial "/" if the path is empty
     */
    std::string full_url_encoded(bool empty_root = false) const
    {
        return urlencode(string(empty_root));
    }

private:
    container_type data;
};

/**
 * \brief Represents a sub-sequence within a Path object
 */
class PathSlice
{
public:
    using value_type = httpony::Path::value_type;
    using reference = httpony::Path::const_reference;
    using iterator = httpony::Path::const_iterator;
    using reverse_iterator = httpony::Path::const_reverse_iterator;
    using size_type = httpony::Path::size_type;

    PathSlice(iterator begin, iterator end)
        : range_begin(begin), range_end(end)
    {}
    PathSlice(const httpony::Path& path)
        : range_begin(path.begin()), range_end(path.end())
    {}

    /**
     * \brief Whether the given path is a prefix of this slice
     */
    bool match_prefix(const Path& prefix) const
    {
        return prefix.empty() || (
            size() >= prefix.size() &&
            std::equal(prefix.begin(), prefix.end(), range_begin)
        );
    }

    /**
     * \brief Whether the given path is a suffix of this slice
     */
    bool match_suffix(const Path& suffix) const
    {
        return suffix.empty() || (
            size() >= suffix.size() &&
            std::equal(suffix.rbegin(), suffix.rend(), rbegin())
        );
    }

    /**
     * \brief Whether the given path is a equal to this slice
     */
    bool match_exactly(const httpony::Path& suffix) const
    {
        return std::equal(suffix.begin(), suffix.end(), range_begin, range_end);
    }

    /**
     * \brief Returns a slice removing \p count items from the front
     */
    PathSlice left_stripped(size_type count) const
    {
        return {melanolib::math::min(range_begin + count, range_end), range_end};
    }

    /**
     * \brief Returns a slice removing \p count items from the back
     */
    PathSlice right_stripped(size_type count) const
    {
        return {range_begin, melanolib::math::max(range_begin, range_end - count)};
    }

    /**
     * \brief Strips the longest matching suffix from \p path
     */
    PathSlice strip_path_suffix(const httpony::Path& path) const
    {
        auto iter_pair = std::mismatch(path.rbegin(), path.rend(), rbegin(), rend());
        return PathSlice(path.begin(), iter_pair.first.base());
    }

    /**
     * \brief Converts the slice to a path object
     */
    httpony::Path to_path() const
    {
        return httpony::Path(range_begin, range_end);
    }

    reference operator[](const size_type pos) const
    {
        return range_begin[pos];
    }

    iterator begin() const { return range_begin; }
    iterator end() const { return range_end; }
    reverse_iterator rbegin() const { return reverse_iterator(range_end); }
    reverse_iterator rend() const { return reverse_iterator(range_begin); }
    size_type size() const { return range_end - range_begin; }
    bool empty() const { return range_end <= range_begin; }

private:
    iterator range_begin;
    iterator range_end;
};

/**
 * \brief URI authority information
 * \see https://tools.ietf.org/html/rfc3986#section-3.2
 */
struct Authority
{
    Authority(const IPAddress& address)
        : host(address.string), port(address.port)
    {
    }

    explicit Authority(const std::string& string);
    Authority() = default;

    melanolib::Optional<std::string> user;
    melanolib::Optional<std::string> password;
    std::string host;
    melanolib::Optional<uint16_t> port;

    std::string full() const;

    bool empty() const
    {
        return !user && !password && host.empty() && !port;
    }

    bool operator==(const Authority& oth) const
    {
        return user == oth.user && password == oth.password &&
            host == oth.host && port == oth.port;
    }

    bool operator!=(const Authority& oth) const
    {
        return !(*this == oth);
    }
};

/**
 * \brief Uniform resource identifier
 * \see https://tools.ietf.org/html/rfc3986
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


    Uri(
        std::string base,
        DataMap query,
        std::string fragment = ""
    ) : Uri(base)
    {
        this->query = std::move(query);
        this->fragment = std::move(fragment);
    }

    Uri(
        std::string scheme,
        Authority authority,
        Path path,
        DataMap query,
        std::string fragment
    );

    std::string full() const;

    std::string query_string(bool question_mark = false) const;

    std::string scheme;
    Authority authority;
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
