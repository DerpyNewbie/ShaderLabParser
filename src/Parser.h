//
// Created by derpy on 2026/02/13.
//
#pragma once

#include <string>
#include <vector>
#include "ParseError.h"
#include "ShaderLab.h"
#include "TokenHelper.h"
#include "sl_exports.h"

namespace sl_parser
{
    struct ParseResult
    {
        /// Everything that could be parsed. Items that failed are left out.
        ShaderLabObject shader;
        std::vector<ParseError> errors;

        [[nodiscard]] bool Succeeded() const
        {
            return errors.empty();
        }
    };

    class Parser
    {
    public:
        /// Parses tokens from Lexer::Tokenize. Throws ParseError on the first error.
        SL_PARSER_EXPORTS static ShaderLabObject Parse(const Tokens& tokens);
        /// Parses tokens, recovering from errors where possible. Never throws ParseError.
        SL_PARSER_EXPORTS static ParseResult TryParse(const Tokens& tokens);
        /// Tokenizes and parses source text. Throws ParseError on the first error.
        SL_PARSER_EXPORTS static ShaderLabObject ParseSource(const std::string& source);
    };
}
