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

#ifndef HTTPONY_BASE_ENCODING_HPP
#define HTTPONY_BASE_ENCODING_HPP

/// \cond
#include <stdexcept>

#include <melanolib/utils/gsl.hpp>
#include <melanolib/string/ascii.hpp>
/// \endcond

namespace httpony {

using byte = uint8_t;
using byte_view = melanolib::gsl::array_view<const byte>;

class EncodingError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/**
 * \brief Base encoding common algorithm
 * \see https://tools.ietf.org/html/rfc4648
 */
class BaseBase
{
public:
    /**
     * \brief Name of the encoding
     */
    std::string name() const
    {
        return encoding_name;
    }

    /**
     * \brief Encodes \p input
     * \returns The base-encoded string
     */
    std::string encode(const std::string& input) const
    {
        std::string output;
        encode(input, output);
        return output;
    }

    /**
     * \brief Size estimate of an output octet string after encoding
     */
    std::size_t encoded_size(std::size_t unencoded_size) const
    {
        if ( unencoded_size % e_grp_count )
            unencoded_size += e_grp_count - (unencoded_size % e_grp_count);
        return unencoded_size * e_grp_count / u_grp_count;
    }

    /**
     * \brief Encodes \p input into \p output
     * \param input  String to encode
     * \param output String to store the result into
     */
    void encode(const std::string& input, std::string& output) const
    {
        output.clear();
        output.reserve(encoded_size(input.size()));

        encode(
            byte_view(reinterpret_cast<const byte*>(input.data()), input.size()),
            std::back_inserter(output)
        );
    }

    /**
     * \brief Encodes \p input into \p output
     * \param input     View to a byte string to encode
     * \param output    Output iterator accepting the converted characters
     */
    template<class OutputIterator>
        void encode(byte_view input, OutputIterator output) const
    {
        uint64_t group = 0;
        int count = 0;

        // Convert u_grp_count groups of u_grp_size bits
        // into e_grp_count groups of e_grp_size
        for ( auto bin : input )
        {
            group = (group << u_grp_size) | bin;
            count++;
            if ( count == u_grp_count )
            {
                encode_bits(group, output, u_grp_size * u_grp_count);
                group = 0;
                count = 0;
            }
        }

        // Handle stray octects
        if ( count )
        {
            // Encode available bits
            encode_bits(group, output, count * u_grp_size);

            // Append padding characters
            if ( pad )
            {
                int remaining = ((u_grp_count * u_grp_size) - (count * u_grp_size)) / e_grp_size;
                for ( int i = 0; i < remaining; i++ )
                {
                    *output = padding;
                    ++output;
                }
            }
        }
    }


    /**
     * \brief Size estimate of an output octet string after decoding
     */
    std::size_t decoded_size(std::size_t encoded_size) const
    {
        if ( encoded_size % u_grp_count )
            encoded_size += u_grp_count - (encoded_size % u_grp_count);
        return encoded_size * u_grp_count / e_grp_count;
    }

    /**
     * \brief Decodes \p input
     * \param input A base-encoded string
     * \returns The unencoded string
     * \throws EncodingError if the encoding failed
     */
    std::string decode(const std::string& input) const
    {
        std::string output;
        if ( !decode(input, output) )
            throw EncodingError("Invalid " + name() + " string");
        return output;
    }

    /**
     * \brief Decodes \p input into \p output
     * \param input  base-encoded string
     * \param output String to store the result into
     *
     * In case of failure \p output will result into an empty string
     * \return \b true on succees
     */
    bool decode(const std::string& input, std::string& output) const
    {
        output.clear();
        output.reserve(decoded_size(input.size()));

        bool encoded = decode(
            byte_view(reinterpret_cast<const byte*>(input.data()), input.size()),
            std::back_inserter(output)
        );

        if ( !encoded )
            output.clear();

        return encoded;
    }

    /**
     * \brief Decodes \p input into \p output
     * \param input     View to a base-encoded byte string
     * \param output    Output iterator accepting the converted characters
     * \return \b true on succees
     */
    template<class OutputIterator>
        bool decode(byte_view input, OutputIterator output) const
    {
        if ( pad && input.size() % e_grp_count )
            return false;

        uint64_t group = 0;
        int count = 0;
        std::size_t i = 0;

        // Converts e_grp_count groups of e_grp_size bits
        // into u_grp_count groups of u_grp_size bits
        for ( ; i < input.size(); i++ )
        {
            if ( input[i] == padding && u_grp_size % e_grp_size )
            {
                if ( i < input.size() - (e_grp_count - 1) )
                    return false;
                else
                    break;
            }

            uint8_t bout;
            if ( !decode_group(input[i], bout) )
                return false;

            group = (group << e_grp_size) | bout;
            count++;

            if ( count == e_grp_count )
            {
                decode_bits(group, output, e_grp_count * e_grp_size);
                group = 0;
                count = 0;
            }
        }

        // Handle padded string
        if ( count )
        {
            decode_bits(group, output, count * e_grp_size);
        }

        return true;
    }

protected:
    explicit BaseBase(
        int u_grp_size,
        int u_grp_count,
        int e_grp_size,
        int e_grp_count,
        bool pad,
        char padding,
        const char* encoding_name
    )
        : u_grp_size(u_grp_size),
          u_grp_count(u_grp_count),
          e_grp_size(e_grp_size),
          e_grp_count(e_grp_count),
          u2e_bitmask((1 << e_grp_size) - 1),
          e2u_bitmask((1 << u_grp_size) - 1),
          pad(pad),
          padding(padding),
          encoding_name(encoding_name)
    {}

private:

    /**
     * \brief Converts a e_grp_size-bit integer into a base-encoded 8-bit character
     * \param data e_grp_size-bit integer [0, 64)
     * \pre data & u2e_bitmask == data
     */
    virtual byte encode_group(byte data) const = 0;

    /**
     * \brief Encodes an integer into the base encoding
     * \param data      Integer containing the bits to be converted
     * \param output    Output iterator accepting the converted characters
     * \param bits      Number of bits in \p data to be considered
     * \pre \p bits <= 64 (Usually should be e_grp_count * e_grp_size)
     */
    template<class OutputIterator>
        void encode_bits(uint64_t data, OutputIterator output, int bits) const
    {
        // Align the most significan bit to e_grp_size
        if ( bits % e_grp_size )
        {
            int diff = e_grp_size - (bits % e_grp_size);
            data <<= diff;
            bits += diff;
        }

        // Eats groups of e_grp_size bits from the most significant to the least
        for ( ;  bits > 0; bits -= e_grp_size )
        {
            int shift = bits - e_grp_size;
            byte bout = encode_group((data >> shift) & u2e_bitmask);
            *output = bout;
            ++output;
        }
    }

    /**
     * \brief Converts a base-encoded character into a e_grp_size-bit integer
     * \returns \b true on success
     */
    virtual bool decode_group(byte data, byte& output) const = 0;

    /**
     * \brief Decodes an integer from base encoding
     * \param data      Integer containing the bits to be converted
     * \param output    Output iterator accepting the converted characters
     * \param bits      Number of bits in \p data to be considered
     * \pre \p bits <= 64 (Usually should be u_grp_count * u_grp_size)
     */
    template<class OutputIterator>
        void decode_bits(uint64_t data, OutputIterator output, int bits) const
    {
        // Align the least significan bit to u_grp_size
        data >>= bits % u_grp_size;
        bits -= bits % u_grp_size;

        // Eats groups of u_grp_size bits from the most significant to the least
        for ( ;  bits > 0; bits -= u_grp_size )
        {
            int shift = bits - u_grp_size;
            *output = (data >> shift) & e2u_bitmask;
            ++output;
        }
    }

    int u_grp_size;         ///< Size in bits of an unencoded group
    int u_grp_count;        ///< Number of unencoded bit groups to encode
    int e_grp_size;         ///< Size in bits of an encoded group
    int e_grp_count;        ///< Number of encoded bit groups
    uint8_t u2e_bitmask;    ///< Mask extract the least significant e_grp_size bits from an integer (for encoding)
    uint8_t e2u_bitmask;    ///< Mask extract the least significant u_grp_size bits from an integer (for decoding)
    bool pad;               ///< Whether it needs padding
    char padding;           ///< Padding character
    const char* encoding_name;

};

/**
 * \brief Base 64 encoding
 * \see https://tools.ietf.org/html/rfc4648#section-4
 */
class Base64 : public BaseBase
{
public:
    /**
     * \param c62 The 62nd character of the base 64 alphabet
     * \param c63 The 63rd character of the base 64 alphabet
     * \param pad Whether to ensure data is properly padded
     */
    Base64(byte c62, byte c63, bool pad = true)
        : BaseBase(8, 3, 6, 4, pad, '=', "Base 64"),
        c62(c62), c63(c63)
    {}

    explicit Base64(bool pad) : Base64('+', '/', pad)
    {}

    Base64() : Base64(true)
    {}


private:
    byte encode_group(byte data) const override
    {
        if ( data < 26 )
            return 'A' + data;
        if ( data < 52 )
            return 'a' + (data - 26);
        if ( data < 62 )
            return '0' + (data - 52);
        if ( data == 62 )
            return c62;
        // Since is being called masking the lowest 6 bits, it can never be
        // larger than 63
        return c63;
    }

    bool decode_group(byte data, byte& output) const override
    {
        if ( data == c63 )
            output = 63;
        else if ( data == c62 )
            output = 62;
        else if ( melanolib::string::ascii::is_digit(data) )
            output = data - '0' + 52;
        else if ( melanolib::string::ascii::is_lower(data) )
            output = data - 'a' + 26;
        else if ( melanolib::string::ascii::is_upper(data) )
            output = data - 'A';
        else
            return false;
        return true;
    }

    byte c62;
    byte c63;
};

/**
 * \brief Base 32 encoding
 * \see https://tools.ietf.org/html/rfc4648#section-6
 */
class Base32 : public BaseBase
{
public:
    /**
     * \param pad Whether to ensure data is properly padded
     */
    explicit Base32(bool pad)
        : BaseBase(8, 5, 5, 8, pad, '=', "Base 32")
    {}

    Base32() : Base32(true)
    {}


private:
    byte encode_group(byte data) const override
    {
        if ( data < 26 )
            return 'A' + data;
        return '2' + (data - 26);
    }

    bool decode_group(byte data, byte& output) const override
    {
        if ( data >= '2' && data <= '7' )
            output = data - '2' + 26;
        else if ( melanolib::string::ascii::is_lower(data) )
            output = data - 'a';
        else if ( melanolib::string::ascii::is_upper(data) )
            output = data - 'A';
        else
            return false;
        return true;
    }
};

/**
 * \brief Base 32 hex encoding
 * \see https://tools.ietf.org/html/rfc4648#section-7
 */
class Base32Hex : public BaseBase
{
public:
    /**
     * \param pad Whether to ensure data is properly padded
     */
    explicit Base32Hex(bool pad)
        : BaseBase(8, 5, 5, 8, pad, '=', "Base 32 Hex")
    {}

    Base32Hex() : Base32Hex(true)
    {}

private:
    byte encode_group(byte data) const override
    {
        if ( data < 10 )
            return '0' + data;
        return 'A' + (data - 10);
    }

    bool decode_group(byte data, byte& output) const override
    {
        if ( melanolib::string::ascii::is_digit(data) )
            output = data - '0';
        else if ( melanolib::string::ascii::is_lower(data) && data <= 'v' )
            output = data - 'a' + 10;
        else if ( melanolib::string::ascii::is_upper(data) && data <= 'V' )
            output = data - 'A' + 10;
        else
            return false;
        return true;
    }
};

/**
 * \brief Base 16 (hex) encoding
 * \see https://tools.ietf.org/html/rfc4648#section-8
 */
class Base16 : public BaseBase
{
public:
    Base16()
        : BaseBase(8, 1, 4, 2, true, '=', "Base 16")
    {}

private:
    byte encode_group(byte data) const override
    {
        return melanolib::string::ascii::hex_digit(data);
    }

    bool decode_group(byte data, byte& output) const override
    {
        if ( melanolib::string::ascii::is_xdigit(data) )
        {
            output = melanolib::string::ascii::get_hex(data);
            return true;
        }
        return false;
    }
};

} // namespace httpony
#endif // HTTPONY_BASE_ENCODING_HPP
