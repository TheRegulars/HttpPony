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

#ifndef HTTPONY_STATUS_HPP
#define HTTPONY_STATUS_HPP

/// \cond
#include <string>
#include <istream>
/// \endcond

namespace httpony {

/**
 * \brief Names for all common status codes
 */
enum class StatusCode
{
    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,

    OK = 200,
    Created = 201,
    Accepted = 202,
    Non = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    Multi = 207,
    AlreadyReported = 208,
    IMUsed = 226,

    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    SwitchProxy = 306,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,

    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    PayloadTooLarge = 413,
    URITooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImaTeapot = 418,
    MisdirectedRequest = 421,
    UnprocessableEntity = 422,
    Locked = 423,
    FailedDependency = 424,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,

    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 507,
    LoopDetected = 508,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511,
};

/**
 * \brief Type of a status code
 */
enum class StatusType
{
    Invalid       = 0,
    Informational = 100,
    Success       = 200,
    Redirection   = 300,
    ClientError   = 400,
    ServerError   = 500,
};

/**
 * \brief A response status
 * \see https://tools.ietf.org/html/rfc7231#section-6
 */
struct Status
{
    unsigned code;
    std::string message;

    Status(StatusCode status = StatusCode::OK);
    Status(unsigned code, std::string message);
    explicit Status(unsigned code);

    StatusType type() const;
    bool is_error() const;
};

inline bool operator==(const Status& status, StatusCode code)
{
    return status.code == unsigned(code);
}

inline bool operator==(StatusCode code, const Status& status)
{
    return status.code == unsigned(code);
}

inline bool operator==(const Status& a, const Status& b)
{
    return a.code == b.code;
}

inline bool operator!=(const Status& status, StatusCode code)
{
    return status.code != unsigned(code);
}

inline bool operator!=(StatusCode code, const Status& status)
{
    return status.code != unsigned(code);
}

inline bool operator!=(const Status& a, const Status& b)
{
    return a.code != b.code;
}

inline std::istream& operator>>(std::istream& in, Status& out)
{
    unsigned numeric_code = 0;
    if ( in >> numeric_code )
        out = Status(numeric_code);
    return in;
}

} // namespace httpony
#endif // HTTPONY_STATUS_HPP
