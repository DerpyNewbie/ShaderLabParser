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
    /// Package name -> version restriction (empty when no version was given). `unity` restricts the editor version.
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
        /// Verbatim source between the begin and END keywords.
        std::string program;
        /// Line of the begin keyword (CGPROGRAM, HLSLINCLUDE, ...).
        uint32_t line = 0;
    };

    typedef std::vector<ShaderProgram> ShaderPrograms;

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
        /// CGINCLUDE / HLSLINCLUDE / GLSLINCLUDE blocks in this pass.
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
        /// Render state for all passes. Commands inherited from an enclosing Category come first.
        std::optional<Commands> commands;
        std::optional<PackageRequirements> package_requirements;
        /// CGINCLUDE / HLSLINCLUDE / GLSLINCLUDE blocks in this sub shader (and its enclosing Category).
        ShaderPrograms includes;
        /// Programs placed directly in the sub shader, i.e. surface shaders.
        ShaderPrograms shader_programs;
        Passes passes;
    };

    typedef std::vector<SubShader> SubShaders;

    struct PropertyAttribute
    {
        std::string name;
        /// Raw text inside the parentheses, e.g. `UnityEngine.Rendering.CullMode` for `[Enum(UnityEngine.Rendering.CullMode)]`.
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
            /// `Any` texture dimension.
            TextureAny,
        };

        std::optional<PropertyAttributes> attributes;
        std::string name;
        std::string display_name;
        /// Type as written, e.g. `2D` or `Range(0,1)`.
        std::string type;
        Type property_type = Type::Float;
        /// Set for Range properties.
        std::optional<std::pair<float, float>> range;
        /// Default as written: the texture name for textures, `0.5` for numbers, `(1,1,1,1)` for vectors.
        std::string default_value;
        /// Numeric default: one element for Float/Int/Range, the components for Color/Vector, empty for textures.
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
        /// Shader-level CGINCLUDE / HLSLINCLUDE / GLSLINCLUDE blocks.
        ShaderPrograms includes;
        SubShaders sub_shaders;
        std::optional<std::string> custom_editor;
        /// Render pipeline asset type -> custom editor, from `CustomEditorForRenderPipeline "Editor" "Pipeline"`.
        std::map<std::string, std::string> custom_editors_for_render_pipeline;
        /// `Dependency "Name" = "Shader"` entries.
        std::map<std::string, std::string> dependencies;
        /// Empty when there is no Fallback or for `Fallback Off`.
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
