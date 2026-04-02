//
// Created by derpy on 2026/02/13.
//

#include "TokenHelper.h"

#include <iostream>
#include <ostream>
#include <stdexcept>


namespace sl_parser
{
    TokensConstIterator TokenHelper::FindToken(const Tokens& tokens, const TokenType type,
                                               TokensConstIterator offset)
    {
        for (auto& token = offset; token != tokens.end(); ++token)
        {
            if (token->type == type)
            {
                return token;
            }
        }

        return tokens.end();
    }

    TokensConstIterator TokenHelper::FindToken(const Tokens& tokens, const TokenType type)
    {
        return FindToken(tokens, type, tokens.begin());
    }

    TokensConstIterator TokenHelper::Next(const TokensConstIterator it)
    {
        ExpectNonEOF(it);

        const auto& next = it + 1;
        if (IsIgnoredToken(next->type))
        {
            return Next(next);
        }

        return next;
    }

    TokensConstIterator TokenHelper::NextType(const TokensConstIterator it, const TokenType type)
    {
        ExpectNonEOF(it);

        const auto& next = it + 1;
        if (next->type != type)
        {
            if (IsIgnoredToken(next->type))
            {
                return NextType(next, type);
            }

            ExpectType(next, type);
        }

        return next;
    }

    TokensConstIterator TokenHelper::NextTypeInBlock(TokensConstIterator it, const TokenType type)
    {
        ExpectNonEOF(it);

        const auto& next = ++it;
        if (next->type != type)
        {
            if (IsIgnoredToken(next->type))
            {
                return NextType(next, type);
            }

            ExpectType(it, type);
        }

        return next;
    }

    TokensConstIterator TokenHelper::SkipInBetween(TokensConstIterator it, const TokenType begin, const TokenType end)
    {
        ExpectNonEOF(it);

        auto& next = it;
        int indent = 0;
        do
        {
            if (next->type == begin)
            {
                ++indent;
            }

            if (next->type == end)
            {
                --indent;
            }

            ++next;
        }
        while (indent > 0 && !IsEOF(next));
        return next;
    }

    TokensConstIterator TokenHelper::NextInBlock(TokensConstIterator it)
    {
        ExpectNonEOF(it);

        const auto& next = ++it;
        if (IsIgnoredToken(next->type))
        {
            return NextInBlock(next);
        }

        if (next->type == TokenType::kBlockBegin)
        {
            return NextInBlock(SkipInBetween(next, TokenType::kBlockBegin, TokenType::kBlockEnd));
        }

        if (next->type == TokenType::kBracketBegin)
        {
            return NextInBlock(SkipInBetween(next, TokenType::kBracketBegin, TokenType::kBracketEnd));
        }

        return next;
    }

    void TokenHelper::ExpectNonEOF(const TokensConstIterator it)
    {
        if (IsEOF(it))
        {
            throw std::runtime_error("unexpected end of file.");
        }
    }

    void TokenHelper::ExpectType(TokensConstIterator it, TokenType type)
    {
        if (it->type != type)
        {
            throw std::runtime_error(
                "unexpected token " +
                to_string(it->type) +
                " '" +
                std::string(it->value) +
                "' at line " +
                std::to_string(it->line) +
                ":" +
                std::to_string(it->pos) +
                ", expected " +
                to_string(type) + "."
            );
        }
    }

    bool TokenHelper::IsIgnoredToken(const TokenType& token)
    {
        return token == TokenType::kNewLine || token == TokenType::kComment;
    }

    bool TokenHelper::IsEOF(const TokensConstIterator& it)
    {
        return it->type == TokenType::kEndOfFile;
    }

    bool TokenHelper::IsOpening(const TokenType& token)
    {
        return token == TokenType::kBracketBegin || token == TokenType::kBlockBegin;
    }

    bool TokenHelper::IsClosing(const TokenType& token)
    {
        return token == TokenType::kBracketEnd || token == TokenType::kBlockEnd;
    }
}
