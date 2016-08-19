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
#ifndef HTTPONY_HTTP_USER_AGENT_HPP
#define HTTPONY_HTTP_USER_AGENT_HPP

/// \cond
#include <vector>
#include <string>
#include <ostream>
#include <melanolib/string/quickstream.hpp>
#include <melanolib/string/ascii.hpp>
#include <melanolib/string/simple_stringutils.hpp>
/// \endcond

#include "httpony/util/version.hpp"

namespace httpony {

class UserAgent
{
public:
    enum class Type
    {
        Invalid,
        Product,
        Comment
    };

    static UserAgent default_user_agent()
    {
        return UserAgent().append_product(version::name, version::version);
    }

    UserAgent() = default;

    UserAgent(const std::string& user_agent_string)
    {
        melanolib::string::QuickStream stream(user_agent_string);
        std::string item;
        while ( !stream.eof() )
        {
            stream.ignore_if(melanolib::string::ascii::is_space_noline);
            if ( stream.eof() )
                break;
            char next = stream.peek();
            if ( melanolib::string::ascii::is_space(next) )
                break;
            else if ( next == '(' )
                append_raw(stream.get_until([](char c){ return c == ')'; }, false));
            else
                append_raw(stream.get_until(melanolib::string::ascii::is_space));
        }
    }

    UserAgent(std::vector<std::string> items)
        : tokens(std::move(items))
    {
        tokens.erase(
            std::remove_if(tokens.begin(), tokens.end(),
                [](const std::string& str) { return str.empty(); }),
            tokens.end()
        );
    }

    std::size_t size() const
    {
        return tokens.size();
    }

    bool empty() const
    {
        return tokens.empty();
    }

    auto begin()
    {
        return tokens.begin();
    }

    auto end()
    {
        return tokens.end();
    }

    auto begin() const
    {
        return tokens.begin();
    }

    auto end() const
    {
        return tokens.end();
    }

    auto cbegin() const
    {
        return tokens.begin();
    }

    auto cend() const
    {
        return tokens.end();
    }

    const std::string& operator[](std::size_t pos) const
    {
        return tokens[pos];
    }

    Type type_at(std::size_t pos) const
    {
        if ( pos > size() || tokens[pos].empty() )
            return Type::Invalid;
        if ( tokens[pos][0] == '(' )
            return Type::Comment;
        return Type::Product;
    }

    std::string comment(std::size_t pos) const
    {
        if ( type_at(pos) == UserAgent::Type::Comment )
            return tokens[pos];
        return {};
    }

    std::string product(std::size_t pos) const
    {
        if ( type_at(pos) == UserAgent::Type::Product )
            return tokens[pos];
        return {};
    }

    std::string product_name(std::size_t pos) const
    {
        if ( type_at(pos) != UserAgent::Type::Product )
            return {};

        auto slash = tokens[pos].find('/');
        if ( slash == std::string::npos )
            return tokens[pos];
        else
            return tokens[pos].substr(0, slash);
    }

    std::string product_version(std::size_t pos) const
    {
        if ( type_at(pos) != UserAgent::Type::Product )
            return {};

        auto slash = tokens[pos].find('/');
        if ( slash == std::string::npos )
            return {};
        else
            return tokens[pos].substr(slash + 1);
    }

    friend std::ostream& operator<<(std::ostream& os, const UserAgent& agent)
    {
        return os << agent.string();
    }

    std::string string() const
    {
        return melanolib::string::implode(" ", tokens);
    }

    UserAgent& append_comment(const std::string& comment)
    {
        if ( comment.empty() )
            return *this;
        if ( comment.front() != '(' )
            tokens.push_back('(' + comment + ')');
        else
            tokens.push_back(comment);

        return *this;
    }

    UserAgent& append_product(const std::string& name, const std::string& version = {})
    {
        if ( name.empty() )
            return *this;
        if ( !version.empty() )
            tokens.push_back(name + '/' + version);
        else
            tokens.push_back(name);

        return *this;
    }

    UserAgent& append_raw(const std::string& item)
    {
        if ( !item.empty() )
            tokens.push_back(item);
        return *this;
    }

    UserAgent operator+ (const UserAgent& oth) const
    {
        UserAgent result = *this;
        result.tokens.insert(result.tokens.end(), oth.tokens.begin(), oth.tokens.end());
        return result;
    }

    UserAgent& operator+= (const UserAgent& oth)
    {
        tokens.insert(tokens.end(), oth.tokens.begin(), oth.tokens.end());
        return *this;
    }

private:
    std::vector<std::string> tokens;
};

} // namespace httpony
#endif // HTTPONY_HTTP_USER_AGENT_HPP
