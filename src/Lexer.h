//
// Created by derpy on 2026/02/12.
//
#pragma once
#include "sl_exports.h"

namespace sl_parser
{
    class Lexer
    {
    public:
        SL_PARSER_EXPORTS static std::vector<Token> Tokenize(std::string program);
        SL_PARSER_EXPORTS static bool IsKeyword(const std::string& value);
    };
}
