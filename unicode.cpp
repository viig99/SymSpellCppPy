#include "Defines.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <stdexcept>

#include "utf8proc.h"

namespace symspellcpppy::unicode {

namespace {
void decode_into(std::string_view text, xstring &result)
{
    result.clear();
    result.reserve(text.size());
    while (!text.empty())
    {
        const auto first = static_cast<unsigned char>(text.front());
        if (first < 128)
        {
            result.push_back(first);
            text.remove_prefix(1);
            continue;
        }

        utf8proc_int32_t codepoint;
        const auto length = utf8proc_iterate(
            reinterpret_cast<const utf8proc_uint8_t *>(text.data()), text.size(), &codepoint);
        if (length < 0)
            throw std::invalid_argument("Expected valid UTF-8 text");
        result.push_back(static_cast<xchar>(codepoint));
        text.remove_prefix(length);
    }
}
}

xstring decode(std::string_view text)
{
    xstring result;
    decode_into(text, result);
    return result;
}

std::string encode(std::u32string_view text)
{
    std::string result;
    result.reserve(text.size());
    for (const auto ch : text)
    {
        if (ch < 128)
        {
            result.push_back(static_cast<char>(ch));
            continue;
        }

        utf8proc_uint8_t bytes[4];
        const auto length = utf8proc_encode_char(ch, bytes);
        result.append(reinterpret_cast<const char *>(bytes), length);
    }
    return result;
}

xstring normalize(std::u32string_view text)
{
    if (std::ranges::all_of(text, [](xchar ch) { return ch < 128; }))
        return xstring(text);

    const auto bytes = encode(text);
    utf8proc_uint8_t *buffer = nullptr;
    const auto size = utf8proc_map(
        reinterpret_cast<const utf8proc_uint8_t *>(bytes.data()), bytes.size(), &buffer,
        static_cast<utf8proc_option_t>(UTF8PROC_STABLE | UTF8PROC_COMPAT | UTF8PROC_COMPOSE));
    std::unique_ptr<utf8proc_uint8_t, decltype(&std::free)> normalized(buffer, std::free);
    if (size < 0)
        throw std::runtime_error(utf8proc_errmsg(size));
    return decode({reinterpret_cast<const char *>(normalized.get()), static_cast<std::size_t>(size)});
}

bool read_line(std::istream &stream, std::string &bytes, xstring &line, bool first_line)
{
    while (std::getline(stream, bytes))
    {
        if (!bytes.empty() && bytes.back() == '\r')
            bytes.pop_back();
        std::string_view text(bytes);
        if (first_line && text.starts_with("\xef\xbb\xbf"))
            text.remove_prefix(3);
        first_line = false;

        try
        {
            decode_into(text, line);
            return true;
        }
        catch (const std::invalid_argument &)
        {
            // Continue past malformed UTF-8 rows and retain the valid rows.
        }
    }
    return false;
}

bool is_space(xchar ch)
{
    if (ch < 128)
        return ch == U' ' || (ch >= U'\t' && ch <= U'\r');
    const auto category = utf8proc_category(ch);
    return ch == U'\u0085' || (category >= UTF8PROC_CATEGORY_ZS && category <= UTF8PROC_CATEGORY_ZP);
}

bool is_punctuation(xchar ch)
{
    const auto category = utf8proc_category(ch);
    return category >= UTF8PROC_CATEGORY_PC && category <= UTF8PROC_CATEGORY_PO;
}

bool is_word(xchar ch)
{
    if (ch < 128)
        return (ch >= U'a' && ch <= U'z') || (ch >= U'A' && ch <= U'Z') ||
               (ch >= U'0' && ch <= U'9') || ch == U'_';
    const auto category = utf8proc_category(ch);
    return (category >= UTF8PROC_CATEGORY_LU && category <= UTF8PROC_CATEGORY_ME) ||
           category == UTF8PROC_CATEGORY_ND || category == UTF8PROC_CATEGORY_PC;
}

bool is_upper(xchar ch)
{
    return ch < 128 ? ch >= U'A' && ch <= U'Z' : utf8proc_isupper(ch) != 0;
}

xchar lower(xchar ch)
{
    return ch < 128 ? (is_upper(ch) ? ch + (U'a' - U'A') : ch) : utf8proc_tolower(ch);
}

xchar upper(xchar ch)
{
    return ch < 128 ? (ch >= U'a' && ch <= U'z' ? ch - (U'a' - U'A') : ch) : utf8proc_toupper(ch);
}

}
