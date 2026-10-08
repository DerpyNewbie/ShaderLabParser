//
// Created by derpy on 2026/02/12.
//

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include "Lexer.h"
#include "StringUtil.h"

namespace
{
    using namespace sl_parser;

    const std::vector<std::string_view> kKeywords = {
        "Shader",
        "Properties",
        "SubShader",
        "Category",
        "Pass",
        "UsePass",
        "GrabPass",
        "Name",
        "Tags",
        "LOD",
        "Fallback",
        "CustomEditor",
        "CustomEditorForRenderPipeline",
        "Dependency",
        "PackageRequirements",
        "AlphaToMask",
        "Blend",
        "BlendOp",
        "ColorMask",
        "Conservative",
        "Cull",
        "Fog",
        "Offset",
        "Stencil",
        "ZClip",
        "ZTest",
        "ZWrite",
        "CGPROGRAM",
        "CGINCLUDE",
        "ENDCG",
        "HLSLPROGRAM",
        "HLSLINCLUDE",
        "ENDHLSL",
        "GLSLPROGRAM",
        "GLSLINCLUDE",
        "ENDGLSL",
    };

    /// Returns the END keyword for a program/include keyword, or an empty view.
    /// Program delimiters are matched case-sensitively, like Unity does.
    std::string_view ProgramEndKeyword(const std::string_view word)
    {
        if (word == "CGPROGRAM" || word == "CGINCLUDE") return "ENDCG";
        if (word == "HLSLPROGRAM" || word == "HLSLINCLUDE") return "ENDHLSL";
        if (word == "GLSLPROGRAM" || word == "GLSLINCLUDE") return "ENDGLSL";
        return {};
    }

    bool IsIdentifierStart(const char c)
    {
        const auto u = static_cast<unsigned char>(c);
        return std::isalpha(u) || c == '_' || u >= 0x80;
    }

    bool IsIdentifierChar(const char c)
    {
        const auto u = static_cast<unsigned char>(c);
        return std::isalnum(u) || c == '_' || u >= 0x80;
    }

    bool IsDigit(const char c)
    {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    }

    /// True if text is a plain decimal number such as "1", "-0.5", ".5" or "1e-3".
    bool IsNumber(const std::string_view text)
    {
        size_t i = 0;
        if (i < text.size() && (text[i] == '-' || text[i] == '+')) ++i;
        bool digits = false, dot = false;
        for (; i < text.size(); ++i)
        {
            if (IsDigit(text[i])) digits = true;
            else if (text[i] == '.' && !dot) dot = true;
            else break;
        }
        if (!digits) return false;
        if (i < text.size() && (text[i] == 'e' || text[i] == 'E'))
        {
            ++i;
            if (i < text.size() && (text[i] == '-' || text[i] == '+')) ++i;
            bool exp_digits = false;
            for (; i < text.size() && IsDigit(text[i]); ++i) exp_digits = true;
            if (!exp_digits) return false;
        }
        // Allow an HLSL-style float suffix.
        if (i < text.size() && (text[i] == 'f' || text[i] == 'F')) ++i;
        return i == text.size();
    }

    class LexerImpl
    {
    public:
        explicit LexerImpl(const std::string& source) : source_(source)
        {
        }

        std::vector<Token> Run()
        {
            if (source_.starts_with("\xEF\xBB\xBF"))
            {
                i_ = 3;
                line_start_ = 3;
            }

            while (i_ < source_.size())
            {
                LexOne();
            }

            Add(TokenType::kEndOfFile, "", i_, i_);
            return std::move(tokens_);
        }

    private:
        const std::string& source_;
        std::vector<Token> tokens_;
        size_t i_ = 0;
        uint32_t line_ = 1;
        size_t line_start_ = 0;

        [[noreturn]] void Fail(const size_t at, const std::string& message) const
        {
            throw ParseError(line_, static_cast<uint32_t>(at - line_start_ + 1), message);
        }

        void Add(const TokenType type, std::string value, const size_t start, const size_t end)
        {
            tokens_.push_back(Token{
                type, std::move(value), line_, static_cast<uint32_t>(start - line_start_),
                static_cast<uint32_t>(start), static_cast<uint32_t>(end - start)
            });
        }

        /// Advances line bookkeeping over source_[from, to).
        void CountLines(const size_t from, const size_t to)
        {
            for (size_t k = from; k < to; ++k)
            {
                if (source_[k] == '\n')
                {
                    ++line_;
                    line_start_ = k + 1;
                }
            }
        }

        [[nodiscard]] char Peek(const size_t offset = 0) const
        {
            return i_ + offset < source_.size() ? source_[i_ + offset] : '\0';
        }

        void LexOne()
        {
            const size_t start = i_;
            switch (const char c = source_[i_])
            {
            case ' ':
            case '\t':
            case '\r':
            case '\f':
            case '\v':
                ++i_;
                return;
            case '\n':
                Add(TokenType::kNewLine, "\n", start, start + 1);
                ++i_;
                ++line_;
                line_start_ = i_;
                return;
            case ',': return Single(TokenType::kComma);
            case ';': return Single(TokenType::kSemiColon);
            case '(': return Single(TokenType::kBracketBegin);
            case ')': return Single(TokenType::kBracketEnd);
            case '{': return Single(TokenType::kBlockBegin);
            case '}': return Single(TokenType::kBlockEnd);
            case '[': return Single(TokenType::kAttributeBegin);
            case ']': return Single(TokenType::kAttributeEnd);
            case '"':
            case '\'':
                return LexString(c);
            case '#':
                {
                    size_t end = source_.find('\n', i_);
                    if (end == std::string::npos) end = source_.size();
                    size_t value_end = end;
                    if (value_end > start && source_[value_end - 1] == '\r') --value_end;
                    Add(TokenType::kPreprocessor, source_.substr(start, value_end - start), start, value_end);
                    i_ = end;
                    return;
                }
            case '/':
                if (Peek(1) == '/')
                {
                    size_t end = source_.find('\n', i_);
                    if (end == std::string::npos) end = source_.size();
                    size_t value_end = end;
                    if (value_end > start && source_[value_end - 1] == '\r') --value_end;
                    Add(TokenType::kComment, source_.substr(start, value_end - start), start, value_end);
                    i_ = end;
                    return;
                }
                if (Peek(1) == '*')
                {
                    const size_t end = source_.find("*/", i_ + 2);
                    if (end == std::string::npos)
                    {
                        Fail(start, "unterminated block comment.");
                    }
                    Add(TokenType::kComment, source_.substr(start, end + 2 - start), start, end + 2);
                    CountLines(start, end + 2);
                    i_ = end + 2;
                    return;
                }
                return Single(TokenType::kOperator);
            default:
                break;
            }

            const char c = source_[i_];
            const bool signed_number = (c == '-' || c == '+') &&
                (IsDigit(Peek(1)) || (Peek(1) == '.' && IsDigit(Peek(2))));
            if (IsDigit(c) || (c == '.' && IsDigit(Peek(1))) || signed_number)
            {
                return LexNumberOrWord();
            }

            if (c == '.')
            {
                return Single(TokenType::kPeriod);
            }

            if (IsIdentifierStart(c))
            {
                return LexIdentifier();
            }

            // Any other character (=, :, +, -, *, <, >, |, ...) is a single-character operator.
            Single(TokenType::kOperator);
        }

        void Single(const TokenType type)
        {
            Add(type, std::string{source_[i_]}, i_, i_ + 1);
            ++i_;
        }

        void LexString(const char quote)
        {
            const size_t start = i_;
            std::string value;
            size_t k = i_ + 1;
            for (;; ++k)
            {
                if (k >= source_.size() || source_[k] == '\n')
                {
                    Fail(start, "unterminated string literal.");
                }
                if (source_[k] == '\\' && k + 1 < source_.size() &&
                    (source_[k + 1] == quote || source_[k + 1] == '\\'))
                {
                    value += source_[++k];
                    continue;
                }
                if (source_[k] == quote)
                {
                    break;
                }
                value += source_[k];
            }

            Add(TokenType::kStringLiteral, std::move(value), start, k + 1);
            i_ = k + 1;
        }

        void LexNumberOrWord()
        {
            const size_t start = i_;
            size_t k = i_;
            if (source_[k] == '-' || source_[k] == '+') ++k;
            for (; k < source_.size(); ++k)
            {
                const char ch = source_[k];
                if (IsIdentifierChar(ch) || ch == '.')
                {
                    continue;
                }
                // Exponent sign, e.g. 1e-3.
                if ((ch == '-' || ch == '+') && (source_[k - 1] == 'e' || source_[k - 1] == 'E') &&
                    k + 1 < source_.size() && IsDigit(source_[k + 1]) &&
                    IsNumber(source_.substr(start, k - start) + "0"))
                {
                    continue;
                }
                break;
            }

            std::string text = source_.substr(start, k - start);
            // Words such as 2D, 3D, 2DArray are identifiers.
            Add(IsNumber(text) ? TokenType::kNumberLiteral : TokenType::kIdentifier, text, start, k);
            i_ = k;
        }

        void LexIdentifier()
        {
            const size_t start = i_;
            size_t k = i_ + 1;
            while (k < source_.size() && IsIdentifierChar(source_[k])) ++k;
            std::string word = source_.substr(start, k - start);
            i_ = k;

            if (const auto end_keyword = ProgramEndKeyword(word); !end_keyword.empty())
            {
                Add(TokenType::kKeyword, word, start, k);
                LexProgramBody(word, end_keyword);
                return;
            }

            const auto type = Lexer::IsKeyword(word) ? TokenType::kKeyword : TokenType::kIdentifier;
            Add(type, std::move(word), start, k);
        }

        /// Captures everything up to the END keyword verbatim, so shader code is never tokenized as ShaderLab.
        void LexProgramBody(const std::string& begin_keyword, const std::string_view end_keyword)
        {
            const size_t body_start = i_;
            size_t search = i_;
            size_t found = std::string::npos;
            while ((search = source_.find(end_keyword, search)) != std::string::npos)
            {
                const bool boundary_before = search == 0 || !IsIdentifierChar(source_[search - 1]);
                const size_t after = search + end_keyword.size();
                const bool boundary_after = after >= source_.size() || !IsIdentifierChar(source_[after]);
                if (boundary_before && boundary_after)
                {
                    found = search;
                    break;
                }
                search += end_keyword.size();
            }

            if (found == std::string::npos)
            {
                Fail(tokens_.back().offset, "unterminated " + begin_keyword + " block, expected " +
                     std::string(end_keyword) + ".");
            }

            Add(TokenType::kProgram, source_.substr(body_start, found - body_start), body_start, found);
            CountLines(body_start, found);
            i_ = found;
            Add(TokenType::kKeyword, std::string(end_keyword), found, found + end_keyword.size());
            i_ = found + end_keyword.size();
        }
    };
}

namespace sl_parser
{
    std::vector<Token> Lexer::Tokenize(const std::string& program)
    {
        return LexerImpl(program).Run();
    }

    bool Lexer::IsKeyword(const std::string& value)
    {
        return std::ranges::any_of(kKeywords, [&](const auto& keyword) { return EqualsIgnoreCase(keyword, value); });
    }
}
