//
// Created by derpy on 2026/02/12.
//
#pragma once
#include <string>
#include <vector>
#include "ParseError.h"
#include "Token.h"
#include "sl_exports.h"

namespace sl_parser
{
    class Lexer
    {
    public:
        /// <summary>Tokenizes ShaderLab source.</summary>
        /// <remarks>A leading UTF-8 BOM is skipped.</remarks>
        /// <param name="program">ShaderLab source text.</param>
        /// <returns>Tokens, including newlines and comments, terminated by a <c>kEndOfFile</c> token.</returns>
        /// <exception cref="ParseError">An unterminated string, comment or program block was found.</exception>
        SL_PARSER_EXPORTS static std::vector<Token> Tokenize(const std::string& program);
        /// <summary>Checks a word against the ShaderLab keyword list, ignoring case.</summary>
        /// <param name="value">Word to check.</param>
        /// <returns><c>true</c> if the word is a ShaderLab keyword.</returns>
        SL_PARSER_EXPORTS static bool IsKeyword(const std::string& value);
    };
}
