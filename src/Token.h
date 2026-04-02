//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <cstdint>

namespace sl_parser
{
    enum class TokenType
    {
        kIdentifier,
        kKeyword,
        kBracketBegin,
        kBracketEnd,
        kBlockBegin,
        kBlockEnd,
        kAttributeBegin,
        kAttributeEnd,
        kNewLine,
        kSemiColon,
        kPeriod,
        kComma,
        kOperator,
        kNumberLiteral,
        kStringLiteral,
        kComment,
        kPreprocessor,
        kEndOfFile,
    };

    struct Token
    {
        TokenType type;
        std::string value;
        uint32_t line;
        uint32_t pos;
    };
}

inline std::string to_string(const sl_parser::TokenType type)
{
    using namespace sl_parser;
    switch (type)
    {
    case TokenType::kNewLine: return "NewLine";
    case TokenType::kIdentifier: return "Identifier";
    case TokenType::kKeyword: return "Keyword";
    case TokenType::kBracketBegin: return "BracketBegin";
    case TokenType::kBracketEnd: return "BracketEnd";
    case TokenType::kBlockBegin: return "BlockBegin";
    case TokenType::kBlockEnd: return "BlockEnd";
    case TokenType::kAttributeBegin: return "AttributeBegin";
    case TokenType::kAttributeEnd: return "AttributeEnd";
    case TokenType::kOperator: return "Operator";
    case TokenType::kNumberLiteral: return "NumberLiteral";
    case TokenType::kStringLiteral: return "StringLiteral";
    case TokenType::kPeriod: return "Period";
    case TokenType::kComma: return "Comma";
    case TokenType::kSemiColon: return "SemiColon";
    case TokenType::kComment: return "Comment";
    case TokenType::kPreprocessor: return "Preprocessor";
    case TokenType::kEndOfFile: return "EndOfFile";
    default: return "Unknown";
    }
}
