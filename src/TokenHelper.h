//
// Created by derpy on 2026/02/13.
//
#pragma once

#include <vector>
#include "Token.h"
#include "sl_exports.h"

namespace sl_parser
{
    typedef std::vector<Token> Tokens;
    typedef Tokens::iterator TokensIterator;
    typedef Tokens::const_iterator TokensConstIterator;

    class TokenHelper
    {
    public:
        SL_PARSER_EXPORTS static TokensConstIterator FindToken(const Tokens& tokens, TokenType type, TokensConstIterator offset);
        SL_PARSER_EXPORTS static TokensConstIterator FindToken(const Tokens& tokens, TokenType type);
        SL_PARSER_EXPORTS static TokensConstIterator Next(TokensConstIterator it);
        SL_PARSER_EXPORTS static TokensConstIterator NextType(TokensConstIterator it, TokenType type);
        SL_PARSER_EXPORTS static TokensConstIterator NextTypeInBlock(TokensConstIterator it, TokenType type);
        SL_PARSER_EXPORTS static TokensConstIterator SkipInBetween(TokensConstIterator it, TokenType begin, TokenType end);
        SL_PARSER_EXPORTS static TokensConstIterator NextInBlock(TokensConstIterator it);
        SL_PARSER_EXPORTS static bool IsIgnoredToken(const TokenType& token);
        SL_PARSER_EXPORTS static bool IsEOF(const TokensConstIterator& it);
        SL_PARSER_EXPORTS static bool IsOpening(const TokenType& token);
        SL_PARSER_EXPORTS static bool IsClosing(const TokenType& token);
        SL_PARSER_EXPORTS static void ExpectNonEOF(TokensConstIterator it);
        SL_PARSER_EXPORTS static void ExpectType(TokensConstIterator it, TokenType type);
    };
}
