//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "sl_exports.h"

#include "Command.h"

namespace sl_parser
{
    typedef std::vector<std::unique_ptr<Command>> Commands;
    typedef std::map<std::string, std::string> Tags;
    /// <summary>Maps a package name to its version restriction (empty when no version was given).</summary>
    /// <remarks>The <c>unity</c> entry restricts the editor version.</remarks>
    typedef std::map<std::string, std::string> PackageRequirements;

    struct ShaderProgram
    {
        enum class Language
        {
            CG,
            HLSL,
            GLSL,
        };

        Language language = Language::CG;
        /// <summary>Verbatim source between the begin and END keywords.</summary>
        std::string program;
        /// <summary>Line of the begin keyword (<c>CGPROGRAM</c>, <c>HLSLINCLUDE</c>, ...).</summary>
        uint32_t line = 0;
    };

    typedef std::vector<ShaderProgram> ShaderPrograms;

    enum class PassType
    {
        /// <summary>Pass definition.</summary>
        Definition,
        /// <summary>Reference to a Definition in another shader.</summary>
        Use,
        /// <summary>Copies the screen contents into a texture.</summary>
        Grab,
    };

    struct OrderedPass
    {
        virtual ~OrderedPass() = default;
        uint8_t order = 0;

        virtual PassType GetPassType() = 0;
    };

    struct GrabPass : OrderedPass
    {
        std::optional<std::string> target_texture;
        std::optional<std::string> name;
        std::optional<Tags> tags;

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
        std::optional<PackageRequirements> package_requirements;
        /// <summary><c>CGINCLUDE</c> / <c>HLSLINCLUDE</c> / <c>GLSLINCLUDE</c> blocks in this pass.</summary>
        ShaderPrograms includes;
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
        /// <summary>Render state for all passes.</summary>
        /// <remarks>Commands inherited from an enclosing Category come first.</remarks>
        std::optional<Commands> commands;
        std::optional<PackageRequirements> package_requirements;
        /// <summary><c>CGINCLUDE</c> / <c>HLSLINCLUDE</c> / <c>GLSLINCLUDE</c> blocks in this sub shader and its enclosing Category.</summary>
        ShaderPrograms includes;
        /// <summary>Programs placed directly in the sub shader, i.e. surface shaders.</summary>
        ShaderPrograms shader_programs;
        Passes passes;
    };

    typedef std::vector<SubShader> SubShaders;

    struct PropertyAttribute
    {
        std::string name;
        /// <summary>Raw text inside the parentheses.</summary>
        /// <remarks><c>UnityEngine.Rendering.CullMode</c> for <c>[Enum(UnityEngine.Rendering.CullMode)]</c>.</remarks>
        std::optional<std::string> arguments;
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
            Vector,
            Range,
            /// <summary><c>Any</c> texture dimension.</summary>
            TextureAny,
        };

        std::optional<PropertyAttributes> attributes;
        std::string name;
        std::string display_name;
        /// <summary>Type as written, e.g. <c>2D</c> or <c>Range(0,1)</c>.</summary>
        std::string type;
        Type property_type = Type::Float;
        /// <summary>Minimum and maximum; set for Range properties.</summary>
        std::optional<std::pair<float, float>> range;
        /// <summary>Default value as written.</summary>
        /// <remarks>The texture name for textures, <c>0.5</c> for numbers, <c>(1,1,1,1)</c> for vectors.</remarks>
        std::string default_value;
        /// <summary>Numeric default value.</summary>
        /// <remarks>One element for Float/Int/Range, the components for Color/Vector, empty for textures.</remarks>
        std::vector<float> default_numbers;
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
        /// <summary>Shader-level <c>CGINCLUDE</c> / <c>HLSLINCLUDE</c> / <c>GLSLINCLUDE</c> blocks.</summary>
        ShaderPrograms includes;
        SubShaders sub_shaders;
        std::optional<std::string> custom_editor;
        /// <summary>Maps a render pipeline asset type to its custom editor.</summary>
        /// <remarks>From <c>CustomEditorForRenderPipeline "Editor" "Pipeline"</c>.</remarks>
        std::map<std::string, std::string> custom_editors_for_render_pipeline;
        /// <summary><c>Dependency "Name" = "Shader"</c> entries.</summary>
        std::map<std::string, std::string> dependencies;
        /// <summary>Fallback shader name.</summary>
        /// <remarks>Empty when there is no Fallback or for <c>Fallback Off</c>.</remarks>
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

SL_PARSER_EXPORTS inline std::string to_string(const sl_parser::ShaderProgram::Language e)
{
    switch (e)
    {
    case sl_parser::ShaderProgram::Language::CG: return "CG";
    case sl_parser::ShaderProgram::Language::HLSL: return "HLSL";
    case sl_parser::ShaderProgram::Language::GLSL: return "GLSL";
    default: return "unknown";
    }
}
