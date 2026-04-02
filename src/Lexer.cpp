//
// Created by derpy on 2026/02/12.
//

#include <vector>
#include <string>
#include "Token.h"
#include "Lexer.h"

namespace
{
    const std::vector<std::string_view> kKeywords = {
        "Shader",
        "Properties",
        "SubShader",
        "Pass",
        "Name",
        "Tags",
        "Blend",
        "BlendOp",
        "ZWrite",
        "ZTest",
        "Offset",
        "Cull",
        "ColorMask",
        "Fog",
        "AlphaToMask",
        "LOD",
        "GrabPass",
        "UsePass",
        "Fallback",
        "Stencil",
        "Instancing",
        "OnlyRenderers",
        "ExcludeRenderers",
        "CGPROGRAM",
        "CGINCLUDE",
        "ENDCG",
        "HLSLPROGRAM",
        "HLSLINCLUDE",
        "ENDHLSL",
    };
}

namespace sl_parser
{
    std::vector<Token> Lexer::Tokenize(std::string program)
    {
        std::vector<Token> result;
        int line = 1, last_new_line = 0;
        for (size_t i = 0; i < program.length(); ++i)
        {
            size_t pos = i - last_new_line;
            switch (const auto current_char = program[i])
            {
            case ' ':
                break;
            case '=':
                result.emplace_back(TokenType::kOperator, std::string{current_char}, line, pos);
                break;
            case ',':
                result.emplace_back(TokenType::kComma, std::string{current_char}, line, pos);
                break;
            case '\n':
                result.emplace_back(TokenType::kNewLine, "\n", line, pos);
                ++line;
                last_new_line = i + 1;
                break;
            case ';':
                result.emplace_back(TokenType::kSemiColon, std::string{current_char}, line, pos);
                break;
            case '(':
                result.emplace_back(TokenType::kBracketBegin, std::string{current_char}, line, pos);
                break;
            case ')':
                result.emplace_back(TokenType::kBracketEnd, std::string{current_char}, line, pos);
                break;
            case '{':
                result.emplace_back(TokenType::kBlockBegin, std::string{current_char}, line, pos);
                break;
            case '}':
                result.emplace_back(TokenType::kBlockEnd, std::string{current_char}, line, pos);
                break;
            case '[':
                result.emplace_back(TokenType::kAttributeBegin, std::string{current_char}, line, pos);
                break;
            case ']':
                result.emplace_back(TokenType::kAttributeEnd, std::string{current_char}, line, pos);
                break;
            case '/':
                {
                    switch (i + 1 < program.size() ? program[i + 1] : '\0')
                    {
                    case '/':
                        {
                            const auto end = program.find('\n', i);
                            result.emplace_back(TokenType::kComment, program.substr(i, end - i), line, pos);
                            i = end - 1;
                            break;
                        }
                    case '*':
                        {
                            const auto end = program.find("*/", i);
                            result.emplace_back(TokenType::kComment, program.substr(i, end - i + 2), line, pos);
                            i = end + 1;
                            break;
                        }
                    default:
                        {
                            result.emplace_back(TokenType::kOperator, std::string{current_char}, line, pos);
                            break;
                        }
                    }

                    break;
                }
            case '#':
                {
                    const auto end = program.find(' ', i);
                    result.emplace_back(TokenType::kPreprocessor, program.substr(i, end - i), line, pos);
                    i = end;
                    break;
                }
            case '.':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                {
                    bool has_alphabet = false;
                    size_t peek = i + 1;
                    for (; peek < program.length(); ++peek)
                    {
                        has_alphabet |= std::isalpha(program[peek]);
                        if (!std::isalpha(program[peek]) && !std::isdigit(program[peek]) && program[peek] != '.')
                            break;
                    }

                    std::string buff = program.substr(i, peek - i);
                    i = peek - 1L;

                    if (buff.size() == 1 && buff[0] == '.')
                    {
                        result.emplace_back(TokenType::kPeriod, buff, line, pos);
                    }
                    else if (has_alphabet)
                    {
                        result.emplace_back(TokenType::kIdentifier, buff, line, pos);
                    }
                    else
                    {
                        result.emplace_back(TokenType::kNumberLiteral, buff, line, pos);
                    }
                    break;
                }
            case '\'':
            case '"':
                {
                    size_t next = program.find(current_char, i + 1);
                    while (next < program.length() && program[next - 1] == '\\')
                    {
                        next = program.find(current_char, i);
                    }

                    result.emplace_back(TokenType::kStringLiteral, program.substr(i + 1, next - i - 1), line, pos);
                    i = next;
                    break;
                }
            default:
                {
                    size_t peek = i + 1;
                    for (; peek < program.length(); ++peek)
                    {
                        if (const auto peek_char = program[peek]; !std::isalnum(peek_char) && peek_char != '_')
                            break;
                    }

                    std::string buff = program.substr(i, peek - i);
                    i = peek - 1L;

                    if (IsKeyword(buff))
                    {
                        result.emplace_back(TokenType::kKeyword, buff, line, pos);
                    }
                    else
                    {
                        result.emplace_back(TokenType::kIdentifier, buff, line, pos);
                    }

                    break;
                }
            }
        }

        result.emplace_back(TokenType::kEndOfFile, "\0", line + 1, 0);
        return result;
    }

    bool Lexer::IsKeyword(const std::string& value)
    {
        return std::ranges::find_if(kKeywords, [&](const auto& keyword) { return keyword == value; }) != kKeywords.
            end();
    }
}
