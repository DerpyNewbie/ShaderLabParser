//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "sl_exports.h"

#include "Command.h"

namespace sl_parser
{
    typedef std::vector<std::unique_ptr<Command>> Commands;
    typedef std::map<std::string, std::string> Tags;

    struct ShaderProgram
    {
        std::string program;
    };

    enum class PassType
    {
        /// Pass definition
        Definition,
        /// Referencing to a Definition
        Use,
        /// Copy screen contents into a texture
        Grab,
    };

    struct OrderedPass
    {
        virtual ~OrderedPass() = default;
        uint8_t order;

        virtual PassType GetPassType() = 0;
    };

    struct GrabPass : OrderedPass
    {
        std::optional<std::string> target_texture;

        PassType GetPassType() override
        {
            return PassType::Grab;
        };
    };

    struct UsePass : OrderedPass
    {
        std::string target_pass;

        PassType GetPassType() override
        {
            return PassType::Use;
        };
    };

    struct Pass : OrderedPass
    {
        std::optional<std::string> name;
        std::optional<Tags> tags;
        std::optional<Commands> commands;
        std::optional<ShaderProgram> shader_program;

        PassType GetPassType() override
        {
            return PassType::Definition;
        };
    };

    typedef std::vector<std::unique_ptr<OrderedPass>> Passes;

    struct SubShader
    {
        SubShader() = default;
        SubShader(const SubShader&) = delete;
        SubShader& operator=(const SubShader&) = delete;
        SubShader(SubShader&&) noexcept = default;
        SubShader& operator=(SubShader&&) noexcept = default;

        std::optional<int> lod;
        std::optional<Tags> tags;
        std::optional<Commands> commands;
        Passes passes;
    };

    typedef std::vector<SubShader> SubShaders;

    struct PropertyAttribute
    {
        std::string name;
    };

    typedef std::vector<PropertyAttribute> PropertyAttributes;

    struct PropertyDecl
    {
        enum class Type
        {
            Integer,
            Float,
            Texture2D,
            Texture2DArray,
            Texture3D,
            Cubemap,
            CubemapArray,
            Color,
            Vector
        };

        std::optional<PropertyAttributes> attributes;
        std::string name;
        std::string display_name;
        std::string type;
        std::string default_value;
    };

    typedef std::vector<PropertyDecl> Properties;

    struct ShaderLabObject
    {
        ShaderLabObject() = default;

        explicit ShaderLabObject(std::string name) : name(std::move(name))
        {
        }

        ShaderLabObject(const ShaderLabObject&) = delete;
        ShaderLabObject& operator=(const ShaderLabObject&) = delete;
        ShaderLabObject(ShaderLabObject&&) noexcept = default;
        ShaderLabObject& operator=(ShaderLabObject&&) noexcept = default;

        std::string name;
        std::optional<Properties> properties;
        SubShaders sub_shaders;
        std::optional<std::string> custom_editor;
        std::optional<std::string> fallback;
    };
}

SL_PARSER_EXPORTS inline std::string to_string(const sl_parser::PassType e)
{
    switch (e)
    {
    case sl_parser::PassType::Definition: return "Definition";
    case sl_parser::PassType::Use: return "Use";
    case sl_parser::PassType::Grab: return "Grab";
    default: return "unknown";
    }
}
