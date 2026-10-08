//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

namespace sl_parser
{
    /// Error raised by the Lexer and Parser. what() is formatted as "line L:C: message".
    struct ParseError : std::runtime_error
    {
        ParseError(const uint32_t line, const uint32_t column, const std::string& message)
            : std::runtime_error(
                  "line " + std::to_string(line) + ":" + std::to_string(column) + ": " + message),
              line(line), column(column), message(message)
        {
        }

        /// 1-based line number.
        uint32_t line;
        /// 1-based column number.
        uint32_t column;
        /// Message without the location prefix.
        std::string message;
    };
}
