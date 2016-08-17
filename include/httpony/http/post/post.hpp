/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright  Mattia Basaglia
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
#ifndef HTTPONY_HTTP_POST_POST_HPP
#define HTTPONY_HTTP_POST_POST_HPP

/// \cond
#include <melanolib/utils/singleton.hpp>
/// \endcond

#include "httpony/http/request.hpp"

namespace httpony {
namespace post {

/**
 * \brief Base class for fomats used to handle payloads
 * \todo Some way to handle json (and maybe even xml)
 */
class PostFormat
{
public:
    virtual ~PostFormat(){}

    /**
     * \brief Whether this format chan handle data provided in the given request
     */
    bool can_parse(const Request& request) const
    {
        if ( !request.body.has_input() )
            return false;
        return do_can_parse(request);
    }

    /**
     * \brief Parses the given data according to its mime type
     * \returns \b true on success
     */
    bool parse(Request& request) const
    {
        if ( !can_parse(request) )
            return false;
        return do_parse(request);
    }

    /**
     * \brief Whether this format chan handle data provided in the given request
     */
    bool can_format(const Request& request) const
    {
        if ( request.body.has_input() )
            return false;
        return do_can_format(request);
    }

    /**
     * \brief Formats the request post data
     * \returns \b true on success
     */
    bool format(Request& request) const
    {
        if ( !can_format(request) )
            return false;
        return do_format(request);
    }

private:

    /**
     * \brief Whether this format chan handle data provided in the given request
     */
    virtual bool do_can_parse(const Request& request) const = 0;

    /**
     * \brief Parses the given data according to its mime type
     * \returns \b true on success
     */
    virtual bool do_parse(Request& request) const = 0;

    /**
     * \brief Whether this format chan handle data provided in the given request
     */
    virtual bool do_can_format(const Request& request) const = 0;

    /**
     * \brief Formats the request post data
     * \returns \b true on success
     */
    virtual bool do_format(Request& request) const = 0;
};

class FormatRegistry : public melanolib::Singleton<FormatRegistry>
{
public:
    void register_format(std::unique_ptr<PostFormat>&& format)
    {
        formats.emplace_back(std::move(format));
    }

    template<class Format, class... Args>
        void register_format(Args&&... args)
    {
        formats.emplace_back(melanolib::New<Format>(std::forward<Args>(args)...));
    }

    void load_default();

    bool can_parse(const Request& request)
    {
        if ( formats.empty() )
            load_default();

        for ( const auto& format : formats )
            if ( format->can_parse(request) )
                return true;

        return false;
    }

    bool parse(Request& request)
    {
        if ( formats.empty() )
            load_default();

        for ( const auto& format : formats )
            if ( format->can_parse(request) )
                return format->parse(request);

        return false;
    }

    bool can_format(const Request& request)
    {
        if ( formats.empty() )
            load_default();

        for ( const auto& format : formats )
            if ( format->can_format(request) )
                return true;

        return false;
    }

    bool format(Request& request)
    {
        if ( formats.empty() )
            load_default();

        for ( const auto& format : formats )
            if ( format->can_format(request) )
                return format->format(request);

        return false;
    }

private:
    FormatRegistry(){}
    friend ParentSingleton;

    std::vector<std::unique_ptr<PostFormat>> formats;
};

} // namespace post
} // namespace httpony
#endif // HTTPONY_HTTP_POST_POST_HPP
