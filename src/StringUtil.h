//
// Created by derpy on 2026/02/13.
//
// Internal helpers. Not part of the installed public headers.
//
#pragma once
#include <algorithm>
#include <cctype>
#include <string_view>

namespace sl_parser
{
    inline bool EqualsIgnoreCase(const std::string_view a, const std::string_view b)
    {
        return std::ranges::equal(a, b, [](const char x, const char y)
        {
            return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
        });
    }
}
