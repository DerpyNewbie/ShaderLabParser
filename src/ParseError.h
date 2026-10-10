//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

namespace sl_parser
{
    /// <summary>Error raised by the Lexer and Parser.</summary>
    /// <remarks><c>what()</c> is formatted as <c>line L:C: message</c>.</remarks>
    struct ParseError : std::runtime_error
    {
        ParseError(const uint32_t line, const uint32_t column, const std::string& message)
            : std::runtime_error(
                  "line " + std::to_string(line) + ":" + std::to_string(column) + ": " + message),
              line(line), column(column), message(message)
        {
        }

        /// <summary>1-based line number.</summary>
        uint32_t line;
        /// <summary>1-based column number.</summary>
        uint32_t column;
        /// <summary>Message without the location prefix.</summary>
        std::string message;
    };
}
