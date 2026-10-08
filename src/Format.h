//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <string>
#include "Command.h"
#include "sl_exports.h"

namespace sl_parser
{
    /// <summary>Formats a command back into ShaderLab syntax.</summary>
    /// <remarks>
    /// Stencil and Fog blocks are written on one line and only list values that differ from the defaults
    /// (Stencil always lists Ref).
    /// </remarks>
    /// <param name="command">Command to format.</param>
    /// <returns>ShaderLab text such as <c>Blend SrcAlpha OneMinusSrcAlpha</c> or <c>Cull [_Cull]</c>.</returns>
    SL_PARSER_EXPORTS std::string FormatCommand(const Command& command);
}

SL_PARSER_EXPORTS std::string to_string(sl_parser::CommandType type);
