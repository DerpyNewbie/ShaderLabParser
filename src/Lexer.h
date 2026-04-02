//
// Created by derpy on 2026/02/12.
//
#pragma once

namespace sl_parser {
    class Lexer {
    public:
        static std::vector<Token> Tokenize(std::string program);

        static bool IsKeyword(const std::string &value);
    };
}
