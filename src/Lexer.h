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
        /// Tokenizes ShaderLab source. A leading UTF-8 BOM is skipped.
        /// Throws ParseError on unterminated strings, comments and program blocks.
        SL_PARSER_EXPORTS static std::vector<Token> Tokenize(const std::string& program);
        /// Case-insensitive check against the ShaderLab keyword list.
        SL_PARSER_EXPORTS static bool IsKeyword(const std::string& value);
    };
}
