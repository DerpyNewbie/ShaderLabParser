//
// Created by derpy on 2026/02/13.
//
#pragma once

#include <vector>
#include <string>
#include "Token.h"

namespace sl_parser
{
    typedef std::vector<Token> Tokens;
    typedef Tokens::iterator TokensIterator;
    typedef Tokens::const_iterator TokensConstIterator;

    class TokenHelper
    {
    public:
        static TokensConstIterator FindToken(const Tokens& tokens, TokenType type, TokensConstIterator offset);
        static TokensConstIterator FindToken(const Tokens& tokens, TokenType type);
        static TokensConstIterator Next(TokensConstIterator it);
        static TokensConstIterator NextType(TokensConstIterator it, TokenType type);
        static TokensConstIterator NextTypeInBlock(TokensConstIterator it, TokenType type);
        static TokensConstIterator SkipInBetween(TokensConstIterator it, TokenType begin, TokenType end);
        static TokensConstIterator NextInBlock(TokensConstIterator it);
        static bool IsIgnoredToken(const TokenType& token);
        static bool IsEOF(const TokensConstIterator& it);
        static bool IsOpening(const TokenType& token);
        static bool IsClosing(const TokenType& token);
        static void ExpectNonEOF(TokensConstIterator it);
        static void ExpectType(TokensConstIterator it, TokenType type);
    };
}
