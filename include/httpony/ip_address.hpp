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

#ifndef HTTPONY_IP_ADDRESS_HPP
#define HTTPONY_IP_ADDRESS_HPP

/// \cond
#include <string>
#include <cstdint>
#include <ostream>
#include <algorithm>

#include <melanolib/string/simple_stringutils.hpp>
/// \endcond

namespace httpony {

/**
 * \brief IP address and port
 */
struct IPAddress
{
    /**
     * \brief Address type
     */
    enum class Type
    {
        Invalid,
        IPv4 = 4,
        IPv6 = 6,
    };

    IPAddress() = default;

    IPAddress(Type type, std::string string, uint16_t port)
        : type(type), string(std::move(string)), port(port)
    {}

    explicit IPAddress(uint16_t port, Type type = Type::IPv6)
        : type(type), port(port)
    {}

    explicit IPAddress(const std::string& address, Type default_type = Type::IPv6)
    {
        static std::regex ip_regex(
            "(?:"
                "((?:[0-9]{1,3}\\.){3}[0-9]{1,3})|"
                "\\[?((?:[0-9a-fA-F]|::)[0-9a-fA-F:]*)\\]?|"
                "([^:]*)"
            ")(?::([0-9]+))?",
            std::regex::ECMAScript|std::regex::optimize
        );
        std::smatch match;
        if ( std::regex_match(address, match, ip_regex) )
        {
            type = default_type;

            if ( match.length(1) )
            {
                string = match[1];
                type = IPAddress::Type::IPv4;
            }
            else if ( match.length(2) )
            {
                string = match[2];
                type = IPAddress::Type::IPv6;
            }

            if ( match.length(4) )
                port = melanolib::string::to_uint(match[4]);
        }
    }

    Type type = Type::Invalid;
    std::string string;
    uint16_t port = 0;
};

inline std::ostream& operator<<(std::ostream& os, const IPAddress& ip)
{
    if ( ip.type == IPAddress::Type::Invalid )
        return os << "(invalid)";

    if ( ip.type == IPAddress::Type::IPv6 && ip.string.find(':') != std::string::npos )
        os << '[' << ip.string << ']';
    else
        os << ip.string;
    
    return os << ':' << ip.port;
}

} // namespace httpony
#endif // HTTPONY_IP_ADDRESS_HPP
