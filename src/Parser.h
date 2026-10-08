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
        /// <summary>Everything that could be parsed.</summary>
        /// <remarks>Items that failed to parse are left out.</remarks>
        ShaderLabObject shader;
        std::vector<ParseError> errors;

        /// <summary>Checks whether parsing finished without errors.</summary>
        /// <returns><c>true</c> if <c>errors</c> is empty.</returns>
        [[nodiscard]] bool Succeeded() const
        {
            return errors.empty();
        }
    };

    class Parser
    {
    public:
        /// <summary>Parses tokens, stopping at the first error.</summary>
        /// <param name="tokens">Tokens from <see cref="Lexer::Tokenize"/>.</param>
        /// <returns>The parsed shader.</returns>
        /// <exception cref="ParseError">The tokens are not valid ShaderLab.</exception>
        SL_PARSER_EXPORTS static ShaderLabObject Parse(const Tokens& tokens);
        /// <summary>Parses tokens, recovering from errors where possible.</summary>
        /// <remarks>Never throws <see cref="ParseError"/>; errors are collected in the result.</remarks>
        /// <param name="tokens">Tokens from <see cref="Lexer::Tokenize"/>.</param>
        /// <returns>Everything that could be parsed and every error found.</returns>
        SL_PARSER_EXPORTS static ParseResult TryParse(const Tokens& tokens);
        /// <summary>Tokenizes and parses source text, stopping at the first error.</summary>
        /// <param name="source">ShaderLab source text.</param>
        /// <returns>The parsed shader.</returns>
        /// <exception cref="ParseError">The source is not valid ShaderLab.</exception>
        SL_PARSER_EXPORTS static ShaderLabObject ParseSource(const std::string& source);
    };
}
