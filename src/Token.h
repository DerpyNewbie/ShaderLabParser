//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <cstdint>
#include <string>
#include "sl_exports.h"

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
        /// Verbatim source between a program/include keyword (CGPROGRAM, HLSLINCLUDE, ...) and its END keyword.
        kProgram,
        kEndOfFile,
    };

    struct Token
    {
        TokenType type;
        std::string value;
        /// 1-based line number.
        uint32_t line;
        /// 0-based column (in bytes) within the line.
        uint32_t pos;
        /// Byte offset of the token in the source.
        uint32_t offset = 0;
        /// Length of the token in the source, in bytes (includes quotes for string literals).
        uint32_t length = 0;
    };
}

SL_PARSER_EXPORTS inline std::string to_string(const sl_parser::TokenType type)
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
    case TokenType::kProgram: return "Program";
    case TokenType::kEndOfFile: return "EndOfFile";
    default: return "Unknown";
    }
}
