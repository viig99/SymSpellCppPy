#pragma once

#include <istream>
#include <string>
#include <string_view>

using xstring = std::u32string;
using xchar = char32_t;

#define XL(x) U##x

namespace symspellcpppy::unicode {
xstring decode(std::string_view text);
std::string encode(std::u32string_view text);
xstring normalize(std::u32string_view text);
bool read_line(std::istream &stream, std::string &bytes, xstring &line, bool first_line);
bool is_space(xchar ch);
bool is_punctuation(xchar ch);
bool is_word(xchar ch);
bool is_upper(xchar ch);
xchar lower(xchar ch);
xchar upper(xchar ch);
}
