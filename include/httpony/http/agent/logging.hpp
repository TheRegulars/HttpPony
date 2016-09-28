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

#ifndef HTTPONY_LOGGING_HPP
#define HTTPONY_LOGGING_HPP

/// \cond
#include <ostream>
/// \endcond

namespace httpony {

template<class T>
    struct CommonLogFormatItem
    {
        template<class... U>
            CommonLogFormatItem(U&&... args)
                : value(std::forward<U>(args)...)
            {}
        T value;
    };

template<class T>
    std::ostream& operator<<(std::ostream& out, const CommonLogFormatItem<T>& clf)
{
    if ( clf.value )
        return out << clf.value;
    return out << '-';
}

template<>
    inline std::ostream& operator<<(std::ostream& out, const CommonLogFormatItem<std::string>& clf)
{
    if ( !clf.value.empty() )
        return out << clf.value;
    return out << '-';
}

/**
 * \brief Streams values as Common Log Format
 */
template<class T>
    CommonLogFormatItem<std::decay_t<T>> clf(T&& item)
    {
        return CommonLogFormatItem<std::decay_t<T>>(std::forward<T>(item));
    }

} // namespace httpony
#endif // HTTPONY_LOGGING_HPP
