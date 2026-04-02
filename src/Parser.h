//
// Created by derpy on 2026/02/13.
//
#pragma once

#include "ShaderLab.h"
#include "TokenHelper.h"
#include "sl_exports.h"

namespace sl_parser
{
    class Parser
    {
        static void ParsePropertyAttributes(TokensConstIterator& pos, PropertyAttributes* attributes);
        static void ParsePropertyDecl(TokensConstIterator& pos, PropertyDecl* decl);
        static void ParseProperties(TokensConstIterator pos, Properties* properties);
        static void ParseTags(TokensConstIterator& pos, std::optional<Tags>* map);
        static void ParseShaderProgram(TokensConstIterator& pos, ShaderProgram* program);
        static void ParsePass(TokensConstIterator& pos, Pass* pass);
        static void ParseCommand(TokensConstIterator& pos, std::optional<Commands>* commands);
        static void ParseSubShader(TokensConstIterator& pos, SubShader* sub_shader);
        static bool IsCommand(const std::string& value);

    public:
        SL_PARSER_EXPORTS static ShaderLabObject Parse(const Tokens& tokens);
    };
}
