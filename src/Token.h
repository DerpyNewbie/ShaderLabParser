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
        /// <summary>Verbatim source between a program/include keyword (<c>CGPROGRAM</c>, <c>HLSLINCLUDE</c>, ...) and its END keyword.</summary>
        kProgram,
        kEndOfFile,
    };

    struct Token
    {
        TokenType type;
        std::string value;
        /// <summary>1-based line number.</summary>
        uint32_t line;
        /// <summary>0-based column (in bytes) within the line.</summary>
        uint32_t pos;
        /// <summary>Byte offset of the token in the source.</summary>
        uint32_t offset = 0;
        /// <summary>Length of the token in the source, in bytes.</summary>
        /// <remarks>Includes the quotes for string literals.</remarks>
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
