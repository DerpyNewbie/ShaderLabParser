// Dependency-free regression tests for the ShaderLab lexer and parser.

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Format.h"
#include "Lexer.h"
#include "Parser.h"

using namespace sl_parser;

namespace
{
    struct TestCase
    {
        const char* name;
        std::function<void()> body;
    };

    std::vector<TestCase>& Registry()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    struct Registrar
    {
        Registrar(const char* name, std::function<void()> body)
        {
            Registry().push_back({name, std::move(body)});
        }
    };

    struct CheckFailure
    {
        std::string message;
    };

    std::ostream& operator<<(std::ostream& stream, const std::vector<std::string>& values)
    {
        stream << "{";
        for (size_t i = 0; i < values.size(); ++i) stream << (i ? ", " : "") << '"' << values[i] << '"';
        return stream << "}";
    }

    std::ostream& operator<<(std::ostream& stream, const std::vector<float>& values)
    {
        stream << "{";
        for (size_t i = 0; i < values.size(); ++i) stream << (i ? ", " : "") << values[i];
        return stream << "}";
    }

    template <typename A, typename B>
    void CheckEqual(const A& actual, const B& expected, const char* expr, const char* file, const int line)
    {
        if (actual == expected) return;
        std::ostringstream stream;
        stream << file << ":" << line << ": CHECK_EQ(" << expr << ") failed\n      actual: " << actual
            << "\n    expected: " << expected;
        throw CheckFailure{stream.str()};
    }
}

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define TEST(name) \
    static void name(); \
    static Registrar CONCAT(registrar_, name)(#name, name); \
    static void name()
#define CHECK(cond) \
    do { if (!(cond)) throw CheckFailure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": CHECK(" #cond ") failed"}; } while (0)
#define CHECK_EQ(actual, expected) CheckEqual((actual), (expected), #actual ", " #expected, __FILE__, __LINE__)
#define CHECK_THROWS_WITH(expr, fragment) \
    do { \
        bool thrown_ = false; \
        try { expr; } \
        catch (const ParseError& e_) { \
            thrown_ = true; \
            if (std::string(e_.what()).find(fragment) == std::string::npos) \
                throw CheckFailure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
                    ": wrong error: " + e_.what()}; \
        } \
        if (!thrown_) throw CheckFailure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": expected ParseError"}; \
    } while (0)

namespace
{
    ShaderLabObject Parse(const std::string& source)
    {
        return Parser::ParseSource(source);
    }

    /// <summary>Parses sub shader body text wrapped in a minimal shader.</summary>
    /// <param name="body">Text placed inside <c>SubShader { }</c>.</param>
    /// <returns>The parsed shader.</returns>
    ShaderLabObject ParseSubShaderBody(const std::string& body)
    {
        return Parse("Shader \"T\" {\nSubShader {\n" + body + "\n}\n}");
    }

    std::vector<std::string> Format(const std::optional<Commands>& commands)
    {
        std::vector<std::string> result;
        if (!commands.has_value()) return result;
        for (const auto& command : *commands) result.push_back(FormatCommand(*command));
        return result;
    }

    const Pass& FirstPass(const ShaderLabObject& shader, const size_t sub_shader = 0)
    {
        const auto* pass = dynamic_cast<const Pass*>(shader.sub_shaders.at(sub_shader).passes.at(0).get());
        if (pass == nullptr) throw CheckFailure{"first pass is not a Pass definition"};
        return *pass;
    }

    std::string ReadFile(const std::string& path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw CheckFailure{"cannot open " + path};
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        return buffer.str();
    }
}

// ---------------------------------------------------------------------------- sample file / structure

TEST(SampleShaderParses)
{
    const auto shader = Parse(ReadFile(std::string(SHADER_LAB_PARSER_TEST_DATA) + "/test.shader"));
    CHECK_EQ(shader.name, "Unlit/NewUnlitShader");
    CHECK_EQ(shader.properties->size(), 1u);
    CHECK_EQ(shader.properties->at(0).name, "_MainTex");
    CHECK_EQ(shader.sub_shaders.size(), 1u);
    CHECK_EQ(shader.sub_shaders[0].lod.value(), 100);
    CHECK_EQ(shader.sub_shaders[0].tags->at("RenderType"), "Opaque");
    const auto& program = FirstPass(shader).shader_program.value();
    CHECK(program.program.find("#include \"UnityCG.cginc\"") != std::string::npos);
    CHECK(program.program.find("fixed4 frag (v2f i) : SV_Target") != std::string::npos);
}

// Bug 1: a file starting directly with `Shader` failed.
TEST(ShaderAsFirstToken)
{
    CHECK_EQ(Parse("Shader \"A\" { }").name, "A");
}

TEST(ShaderAfterBom)
{
    CHECK_EQ(Parse("\xEF\xBB\xBFShader \"A\" { }").name, "A");
}

TEST(MissingShaderKeyword)
{
    CHECK_THROWS_WITH(Parse("SubShader { }"), "line 1:1: expected Shader");
}

TEST(TrailingGarbage)
{
    CHECK_THROWS_WITH(Parse("Shader \"A\" { } Pass"), "after the end of the Shader block");
}

TEST(UnclosedShader)
{
    CHECK_THROWS_WITH(Parse("Shader \"A\" { SubShader { Pass { } }"), "end of file");
}

// ---------------------------------------------------------------------------- commands

// Bug 2: commands had to end with a newline.
TEST(CommandsOnOneLine)
{
    const auto shader = Parse("Shader \"T\" { SubShader { Cull Off ZWrite Off Pass { ZTest Always } } }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands), (std::vector<std::string>{"Cull Off", "ZWrite Off"}));
    CHECK_EQ(Format(FirstPass(shader).commands), (std::vector<std::string>{"ZTest Always"}));
}

TEST(BlendTwoFactorsFollowedByCommand)
{
    const auto shader = ParseSubShaderBody("Blend SrcAlpha OneMinusSrcAlpha\nBlendOp Add\nPass { }");
    const auto& commands = *shader.sub_shaders[0].commands;
    CHECK_EQ(Format(shader.sub_shaders[0].commands),
             (std::vector<std::string>{"Blend SrcAlpha OneMinusSrcAlpha", "BlendOp Add"}));
    const auto& blend = dynamic_cast<const Blend&>(*commands[0]);
    CHECK(blend.state);
    CHECK(!blend.separate_alpha);
    CHECK(blend.alpha_src_factor == Blend::Factor::SrcAlpha);
    CHECK(blend.alpha_dst_factor == Blend::Factor::OneMinusSrcAlpha);
}

TEST(BlendVariants)
{
    const auto shader = ParseSubShaderBody(
        "Blend Off\nBlend 1 Off\nBlend One One, Zero OneMinusSrcAlpha\nBlend 0 DstColor Zero\n"
        "Blend SrcAlphaSaturate One\nBlend [_Src] [_Dst]\nPass { }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands), (std::vector<std::string>{
                 "Blend Off", "Blend 1 Off", "Blend One One, Zero OneMinusSrcAlpha", "Blend 0 DstColor Zero",
                 "Blend SrcAlphaSaturate One", "Blend [_Src] [_Dst]"
                 }));
    const auto& blend = dynamic_cast<const Blend&>(*shader.sub_shaders[0].commands->at(3));
    CHECK_EQ(static_cast<int>(blend.target.value()), 0);
}

TEST(BlendRenderTargetOutOfRange)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("Blend 8 One One"), "render target must be between 0 and 7");
}

TEST(BlendOpSeparateAlpha)
{
    const auto shader = ParseSubShaderBody("BlendOp Sub, Max\nBlendOp 2 Min\nPass { }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands),
             (std::vector<std::string>{"BlendOp Sub, Max", "BlendOp 2 Min"}));
}

TEST(ColorMaskVariants)
{
    const auto shader = ParseSubShaderBody("ColorMask RGB\nColorMask 0\nColorMask RA 2\nColorMask [_Mask]\n"
        "AlphaToMask On\nPass { }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands), (std::vector<std::string>{
                 "ColorMask RGB", "ColorMask 0", "ColorMask RA 2", "ColorMask [_Mask]", "AlphaToMask On"
                 }));
}

// Bug 13: ColorMask had uninitialized channels.
TEST(CommandDefaults)
{
    const ColorMask color_mask;
    CHECK(color_mask.channels == ColorMask::Channels::RGBA);
    CHECK(!color_mask.target.has_value());
    const BlendOp blend_op;
    CHECK(blend_op.operation == BlendOp::Op::Add);
    const Offset offset;
    CHECK_EQ(offset.factor.value, 0.0f);
    CHECK_EQ(offset.units.value, 0.0f);
}

TEST(InvalidColorMask)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("ColorMask RGX"), "expected color mask");
}

// Bug 6: ZClip and Conservative were not keywords.
TEST(ConservativeAndZClip)
{
    const auto shader = ParseSubShaderBody("Conservative True\nZClip False\nPass { }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands),
             (std::vector<std::string>{"Conservative True", "ZClip False"}));
}

// Bug 7: Offset 1, 1 and negative numbers failed.
TEST(OffsetVariants)
{
    const auto shader = ParseSubShaderBody("Offset 1, 1\nOffset -1, -1\nOffset -0.5, [_Units]\nPass { }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands),
             (std::vector<std::string>{"Offset 1, 1", "Offset -1, -1", "Offset -0.5, [_Units]"}));
}

TEST(ZTestAndCullValues)
{
    const auto shader = ParseSubShaderBody(
        "ZTest Less\nZTest Greater\nZTest LEqual\nZTest GEqual\nZTest Equal\nZTest NotEqual\nZTest Always\n"
        "Cull Back\nCull Front\nCull Off\nPass { }");
    CHECK_EQ(shader.sub_shaders[0].commands->size(), 10u);
    CHECK_EQ(FormatCommand(*shader.sub_shaders[0].commands->at(5)), "ZTest NotEqual");
}

TEST(InvalidCommandValue)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("Cull Sideways"), "line 3:6: expected cull mode (Off, Back, Front");
}

// Property references in every command.
TEST(PropertyReferences)
{
    const auto shader = ParseSubShaderBody(
        "Cull [_Cull]\nZWrite [_ZWrite]\nZTest [_ZTest]\nAlphaToMask [_A2M]\nBlendOp [_Op]\nPass { }");
    const auto& commands = *shader.sub_shaders[0].commands;
    const auto& cull = dynamic_cast<const Cull&>(*commands[0]);
    CHECK(cull.state.IsProperty());
    CHECK_EQ(*cull.state.property, "_Cull");
    CHECK_EQ(Format(shader.sub_shaders[0].commands), (std::vector<std::string>{
                 "Cull [_Cull]", "ZWrite [_ZWrite]", "ZTest [_ZTest]", "AlphaToMask [_A2M]", "BlendOp [_Op]"
                 }));
}

TEST(CaseInsensitiveKeywordsAndValues)
{
    const auto shader = Parse("shader \"T\" { subshader { cull off zwrite ON pass { } } FallBack \"Diffuse\" }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands), (std::vector<std::string>{"Cull Off", "ZWrite On"}));
    // Bug 14: FallBack was silently dropped.
    CHECK_EQ(shader.fallback.value(), "Diffuse");
}

// ---------------------------------------------------------------------------- stencil

// Bug 5: Stencil always threw.
TEST(StencilBlock)
{
    const auto shader = ParseSubShaderBody(
        "Stencil {\n Ref 2\n ReadMask 3\n Comp Equal\n Pass Replace\n Fail IncrSat\n ZFail DecrWrap\n"
        " CompBack Never\n PassBack Zero\n FailBack Invert\n ZFailBack IncrWrap\n"
        " CompFront Less\n PassFront DecrSat\n FailFront Replace\n ZFailFront Zero\n}\nPass { }");
    const auto& stencil = dynamic_cast<const Stencil&>(*shader.sub_shaders[0].commands->at(0));
    CHECK_EQ(static_cast<int>(stencil.ref.value), 2);
    CHECK_EQ(static_cast<int>(stencil.read_mask.value), 3);
    CHECK_EQ(static_cast<int>(stencil.write_mask.value), 255);
    CHECK(stencil.comparison_operation == Stencil::Comparison::Equal);
    CHECK(stencil.pass_operation == Stencil::Operation::Replace);
    CHECK(stencil.fail_operation == Stencil::Operation::IncrSat);
    CHECK(stencil.z_fail_operation == Stencil::Operation::DecrWrap);
    CHECK(stencil.comparison_operation_back == Stencil::Comparison::Never);
    CHECK(stencil.pass_operation_back == Stencil::Operation::Zero);
    CHECK(stencil.fail_operation_back == Stencil::Operation::Invert);
    CHECK(stencil.z_fail_operation_back == Stencil::Operation::IncrWrap);
    CHECK(stencil.comparison_operation_front == Stencil::Comparison::Less);
    CHECK(stencil.pass_operation_front == Stencil::Operation::DecrSat);
    CHECK(stencil.fail_operation_front == Stencil::Operation::Replace);
    CHECK(stencil.z_fail_operation_front == Stencil::Operation::Zero);
    CHECK_EQ(shader.sub_shaders[0].passes.size(), 1u);
}

TEST(StencilOneLineWithProperties)
{
    const auto shader = ParseSubShaderBody("Stencil { Ref [_Ref] Comp [_Comp] Pass Replace } Pass { }");
    CHECK_EQ(FormatCommand(*shader.sub_shaders[0].commands->at(0)),
             "Stencil { Ref [_Ref] Comp [_Comp] Pass Replace }");
}

TEST(StencilRefRange)
{
    CHECK_EQ(static_cast<int>(dynamic_cast<const Stencil&>(
                 *ParseSubShaderBody("Stencil { Ref 255 }").sub_shaders[0].commands->at(0)).ref.value), 255);
    CHECK_THROWS_WITH(ParseSubShaderBody("Stencil { Ref 256 }"), "between 0 and 255");
}

TEST(StencilUnknownKey)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("Stencil { Foo 1 }"), "unknown stencil operation 'Foo'");
}

TEST(FogBlock)
{
    const auto shader = ParseSubShaderBody(
        "Fog { Mode Off }\nFog { Color (0,0,0,0) }\nFog { Mode Exp2 Density 0.5 Range 1, 100 }\nPass { }");
    CHECK_EQ(Format(shader.sub_shaders[0].commands), (std::vector<std::string>{
                 "Fog { Mode Off }", "Fog { Color (0,0,0,0) }", "Fog { Mode Exp2 Density 0.5 Range 1, 100 }"
                 }));
}

// ---------------------------------------------------------------------------- tags

// Bug 3: Tags with more than one entry failed.
TEST(MultipleTags)
{
    const auto shader = ParseSubShaderBody(
        "Tags { \"Queue\"=\"Transparent\" \"IgnoreProjector\"=\"True\"\n \"RenderType\" = \"Transparent\" }\n"
        "Pass { Tags { \"LightMode\"=\"ForwardBase\" \"A\"=\"B\" } }");
    const auto& tags = *shader.sub_shaders[0].tags;
    CHECK_EQ(tags.size(), 3u);
    CHECK_EQ(tags.at("Queue"), "Transparent");
    CHECK_EQ(tags.at("IgnoreProjector"), "True");
    CHECK_EQ(tags.at("RenderType"), "Transparent");
    CHECK_EQ(FirstPass(shader).tags->size(), 2u);
}

TEST(EmptyTags)
{
    CHECK_EQ(ParseSubShaderBody("Tags { } Pass { }").sub_shaders[0].tags->size(), 0u);
}

// ---------------------------------------------------------------------------- properties

// Bug 4: number, vector and range defaults were broken.
TEST(AllPropertyTypes)
{
    const auto shader = Parse(R"(Shader "T" {
    Properties {
        _F ("Float", Float) = 0.5
        _I ("Int", Int) = 2
        _I2 ("Integer", Integer) = -3
        _R ("Range", Range(0, 1)) = 0.25
        _C ("Color", Color) = (1, 0.5, 0, 1)
        _V ("Vector", Vector) = (-1, 0, 0, 0)
        _T ("Tex", 2D) = "white" {}
        _TA ("TexArray", 2DArray) = "" {}
        _T3 ("Tex3D", 3D) = "black" {}
        _Cube ("Cube", Cube) = "grey" {}
        _CubeA ("CubeArray", CubeArray) = "" {}
        _Any ("Any", Any) = "" {}
        _NoBraces ("NoBraces", 2D) = "bump"
        _Legacy ("Legacy", Cube) = "" { TexGen CubeReflect }
    }
    SubShader { Pass { } }
})");
    const auto& p = *shader.properties;
    CHECK_EQ(p.size(), 14u);
    CHECK(p[0].property_type == PropertyDecl::Type::Float);
    CHECK_EQ(p[0].default_numbers, (std::vector<float>{0.5f}));
    CHECK_EQ(p[0].default_value, "0.5");
    CHECK(p[1].property_type == PropertyDecl::Type::Integer);
    CHECK(p[2].property_type == PropertyDecl::Type::Integer);
    CHECK_EQ(p[2].default_numbers, (std::vector<float>{-3.0f}));
    CHECK(p[3].property_type == PropertyDecl::Type::Range);
    CHECK_EQ(p[3].type, "Range(0, 1)");
    CHECK_EQ(p[3].range->first, 0.0f);
    CHECK_EQ(p[3].range->second, 1.0f);
    CHECK_EQ(p[3].default_numbers, (std::vector<float>{0.25f}));
    CHECK(p[4].property_type == PropertyDecl::Type::Color);
    CHECK_EQ(p[4].default_value, "(1, 0.5, 0, 1)");
    CHECK_EQ(p[4].default_numbers, (std::vector<float>{1.0f, 0.5f, 0.0f, 1.0f}));
    CHECK_EQ(p[5].default_numbers, (std::vector<float>{-1.0f, 0.0f, 0.0f, 0.0f}));
    CHECK(p[6].property_type == PropertyDecl::Type::Texture2D);
    CHECK_EQ(p[6].default_value, "white");
    CHECK_EQ(p[6].type, "2D");
    CHECK(p[7].property_type == PropertyDecl::Type::Texture2DArray);
    CHECK(p[8].property_type == PropertyDecl::Type::Texture3D);
    CHECK(p[9].property_type == PropertyDecl::Type::Cubemap);
    CHECK(p[10].property_type == PropertyDecl::Type::CubemapArray);
    CHECK(p[11].property_type == PropertyDecl::Type::TextureAny);
    CHECK_EQ(p[12].default_value, "bump");
    CHECK_EQ(p[13].name, "_Legacy");
}

TEST(PropertyAttributeParsing)
{
    const auto shader = Parse(R"(Shader "T" {
    Properties {
        [HDR] _Color ("Color", Color) = (1,1,1,1)
        [Toggle, NoScaleOffset] _T ("T", 2D) = "white" {}
        [HideInInspector][PerRendererData] _H ("H", Float) = 0
        [Enum(UnityEngine.Rendering.CullMode)] _Cull ("Cull", Float) = 2
        [KeywordEnum(None, Add, Multiply)] _Overlay ("Overlay mode", Float) = 0
        [Header(Main Settings)] [Space(10)] [PowerSlider(3.0)] _Shininess ("Shininess", Range (0.01, 1)) = 0.08
    }
    SubShader { Pass { } }
})");
    const auto& p = *shader.properties;
    CHECK_EQ(p.size(), 6u);
    CHECK_EQ(p[0].attributes->at(0).name, "HDR");
    CHECK(!p[0].attributes->at(0).arguments.has_value());
    CHECK_EQ(p[1].attributes->size(), 2u);
    CHECK_EQ(p[1].attributes->at(1).name, "NoScaleOffset");
    CHECK_EQ(p[2].attributes->size(), 2u);
    CHECK_EQ(p[3].attributes->at(0).name, "Enum");
    CHECK_EQ(p[3].attributes->at(0).arguments.value(), "UnityEngine.Rendering.CullMode");
    CHECK_EQ(p[4].attributes->at(0).arguments.value(), "None, Add, Multiply");
    CHECK_EQ(p[5].attributes->size(), 3u);
    CHECK_EQ(p[5].attributes->at(0).arguments.value(), "Main Settings");
    CHECK_EQ(p[5].attributes->at(2).arguments.value(), "3.0");
    CHECK_EQ(p[5].range->first, 0.01f);
}

TEST(UnknownPropertyType)
{
    CHECK_THROWS_WITH(Parse("Shader \"T\" { Properties { _X (\"X\", Matrix) = 0 } }"),
                      "unknown property type 'Matrix'");
}

TEST(TextureDefaultMustBeString)
{
    CHECK_THROWS_WITH(Parse("Shader \"T\" { Properties { _X (\"X\", 2D) = 0 } }"), "default texture name");
}

// ---------------------------------------------------------------------------- passes

// Not implemented before: commands inside Pass.
TEST(CommandsInsidePass)
{
    const auto shader = ParseSubShaderBody(
        "Pass {\nName \"Main\"\nCull Off\nBlend One One\nStencil { Ref 1 Comp Always Pass Replace }\n"
        "Tags { \"LightMode\"=\"ForwardBase\" }\nCGPROGRAM\nENDCG\n}");
    const auto& pass = FirstPass(shader);
    CHECK_EQ(pass.name.value(), "Main");
    CHECK_EQ(Format(pass.commands), (std::vector<std::string>{
                 "Cull Off", "Blend One One", "Stencil { Ref 1 Pass Replace }"
                 }));
    CHECK(pass.shader_program.has_value());
}

// Bug 8: GrabPass blocks were not consumed.
TEST(GrabPassVariants)
{
    const auto shader = ParseSubShaderBody(
        "GrabPass { }\nGrabPass { \"_Grab\" }\nGrabPass { Name \"G\" Tags { \"A\"=\"B\" } \"_T\" }\n"
        "UsePass \"Other/PASS\"\nPass { }");
    const auto& passes = shader.sub_shaders[0].passes;
    CHECK_EQ(passes.size(), 5u);
    const auto* first = dynamic_cast<const GrabPass*>(passes[0].get());
    CHECK(first != nullptr && !first->target_texture.has_value());
    const auto* second = dynamic_cast<const GrabPass*>(passes[1].get());
    CHECK_EQ(second->target_texture.value(), "_Grab");
    const auto* third = dynamic_cast<const GrabPass*>(passes[2].get());
    CHECK_EQ(third->name.value(), "G");
    CHECK_EQ(third->target_texture.value(), "_T");
    CHECK_EQ(third->tags->at("A"), "B");
    const auto* use = dynamic_cast<const UsePass*>(passes[3].get());
    CHECK_EQ(use->target_pass, "Other/PASS");
    for (size_t i = 0; i < passes.size(); ++i) CHECK_EQ(static_cast<size_t>(passes[i]->order), i);
}

TEST(TwoProgramsInPass)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("Pass { CGPROGRAM\nENDCG\nCGPROGRAM\nENDCG }"), "only contain one");
}

// Bug 9: an unknown keyword in SubShader hung forever.
TEST(UnknownKeywordInSubShaderErrors)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("Lighting Off\nPass { }"), "line 3:1: unexpected 'Lighting' in SubShader");
}

TEST(UnknownKeywordInPassErrors)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("Pass { SetTexture [_MainTex] }"), "unexpected 'SetTexture' in Pass");
}

// ---------------------------------------------------------------------------- programs

// Bug 11: program text stopped at ShaderLab keywords and lost quotes.
TEST(ProgramIsVerbatim)
{
    const std::string body =
        "\n#include \"UnityCG.cginc\"\nfloat Offset = 1; float Pass; // Cull Off\n"
        "/* } */ float4 frag() : SV_Target { return float4(1, 0.5, -1, 1e-3); }\n";
    const auto shader = ParseSubShaderBody("Pass {\nCGPROGRAM" + body + "ENDCG\n}");
    const auto& program = FirstPass(shader).shader_program.value();
    CHECK_EQ(program.program, body);
    CHECK(program.language == ShaderProgram::Language::CG);
    CHECK_EQ(program.line, 4u);
}

TEST(HlslAndGlslPrograms)
{
    const auto shader = ParseSubShaderBody(
        "Pass { HLSLPROGRAM\nfloat4 x;\nENDHLSL }\nPass { GLSLPROGRAM\nvoid main() {}\nENDGLSL }");
    CHECK(FirstPass(shader).shader_program->language == ShaderProgram::Language::HLSL);
    const auto* second = dynamic_cast<const Pass*>(shader.sub_shaders[0].passes[1].get());
    CHECK(second->shader_program->language == ShaderProgram::Language::GLSL);
    CHECK_EQ(second->shader_program->program, "\nvoid main() {}\n");
}

TEST(EndKeywordNeedsWordBoundary)
{
    const auto shader = ParseSubShaderBody("Pass { CGPROGRAM\nint ENDCGX; int MY_ENDCG;\nENDCG }");
    CHECK_EQ(FirstPass(shader).shader_program->program, "\nint ENDCGX; int MY_ENDCG;\n");
}

TEST(UnterminatedProgram)
{
    CHECK_THROWS_WITH(ParseSubShaderBody("Pass { CGPROGRAM\nfloat x;\n}"), "unterminated CGPROGRAM block");
}

// Not implemented before: CGINCLUDE / HLSLINCLUDE.
TEST(IncludeBlocksAtEveryLevel)
{
    const auto shader = Parse(R"(Shader "T" {
    CGINCLUDE
    float shared;
    ENDCG
    SubShader {
        HLSLINCLUDE
        float sub;
        ENDHLSL
        Pass {
            CGINCLUDE
            float pass;
            ENDCG
            CGPROGRAM
            ENDCG
        }
    }
})");
    CHECK_EQ(shader.includes.size(), 1u);
    CHECK(shader.includes[0].program.find("float shared;") != std::string::npos);
    CHECK_EQ(shader.sub_shaders[0].includes.size(), 1u);
    CHECK(shader.sub_shaders[0].includes[0].language == ShaderProgram::Language::HLSL);
    CHECK_EQ(FirstPass(shader).includes.size(), 1u);
}

TEST(SurfaceShaderProgramInSubShader)
{
    const auto shader = ParseSubShaderBody(
        "Tags { \"RenderType\"=\"Opaque\" }\nLOD 200\nCGPROGRAM\n#pragma surface surf Standard\nENDCG");
    CHECK_EQ(shader.sub_shaders[0].shader_programs.size(), 1u);
    CHECK(shader.sub_shaders[0].shader_programs[0].program.find("#pragma surface") != std::string::npos);
    CHECK_EQ(shader.sub_shaders[0].passes.size(), 0u);
}

// ---------------------------------------------------------------------------- shader level

TEST(FallbackVariants)
{
    CHECK_EQ(Parse("Shader \"T\" { Fallback \"Diffuse\" }").fallback.value(), "Diffuse");
    CHECK(!Parse("Shader \"T\" { Fallback Off }").fallback.has_value());
    CHECK(!Parse("Shader \"T\" { FallBack off }").fallback.has_value());
}

TEST(CustomEditorsAndDependencies)
{
    const auto shader = Parse(R"(Shader "T" {
    SubShader { Pass { } }
    CustomEditor "MyEditor"
    CustomEditorForRenderPipeline "UrpEditor" "UnityEngine.Rendering.Universal.UniversalRenderPipelineAsset"
    Dependency "BaseMapShader" = "Hidden/Base"
    Dependency "AddPassShader" = "Hidden/Add"
})");
    CHECK_EQ(shader.custom_editor.value(), "MyEditor");
    CHECK_EQ(shader.custom_editors_for_render_pipeline.at(
                 "UnityEngine.Rendering.Universal.UniversalRenderPipelineAsset"), "UrpEditor");
    CHECK_EQ(shader.dependencies.size(), 2u);
    CHECK_EQ(shader.dependencies.at("BaseMapShader"), "Hidden/Base");
}

TEST(LegacyCustomEditorWithEquals)
{
    CHECK_EQ(Parse("Shader \"T\" { CustomEditor = \"E\" }").custom_editor.value(), "E");
}

TEST(PackageRequirementsBlocks)
{
    const auto shader = ParseSubShaderBody(
        "PackageRequirements {\n\"com.unity.render-pipelines.universal\": \"[10.2.1,11.0]\"\n"
        "\"com.unity.textmeshpro\"\n\"unity\": \"2021.2\"\n}\n"
        "Pass { PackageRequirements { \"com.unity.x\" } }");
    const auto& requirements = *shader.sub_shaders[0].package_requirements;
    CHECK_EQ(requirements.size(), 3u);
    CHECK_EQ(requirements.at("com.unity.render-pipelines.universal"), "[10.2.1,11.0]");
    CHECK_EQ(requirements.at("com.unity.textmeshpro"), "");
    CHECK_EQ(requirements.at("unity"), "2021.2");
    CHECK_EQ(FirstPass(shader).package_requirements->size(), 1u);
}

TEST(CategoryIsFlattenedIntoSubShaders)
{
    const auto shader = Parse(R"(Shader "T" {
    Category {
        Tags { "Queue"="Transparent" "RenderType"="Transparent" }
        Blend SrcAlpha One
        Cull Off
        SubShader { Tags { "RenderType"="Opaque" } ZWrite Off Pass { } }
        SubShader { Pass { } }
    }
    SubShader { Pass { } }
})");
    CHECK_EQ(shader.sub_shaders.size(), 3u);
    CHECK_EQ(Format(shader.sub_shaders[0].commands),
             (std::vector<std::string>{"Blend SrcAlpha One", "Cull Off", "ZWrite Off"}));
    CHECK_EQ(shader.sub_shaders[0].tags->at("RenderType"), "Opaque"); // sub shader wins
    CHECK_EQ(shader.sub_shaders[0].tags->at("Queue"), "Transparent");
    CHECK_EQ(Format(shader.sub_shaders[1].commands),
             (std::vector<std::string>{"Blend SrcAlpha One", "Cull Off"}));
    CHECK(!shader.sub_shaders[2].commands.has_value());
}

TEST(UnknownShaderLevelItem)
{
    CHECK_THROWS_WITH(Parse("Shader \"T\" { Pass { } }"), "unexpected 'Pass' in Shader");
}

// ---------------------------------------------------------------------------- lexer

// Bug 10: tabs and CRLF line endings.
TEST(TabsAndCrlf)
{
    const auto tabs = Parse("Shader \"T\" {\n\tSubShader {\n\t\tCull Off\n\t\tPass { }\n\t}\n}");
    CHECK_EQ(tabs.sub_shaders.size(), 1u);
    CHECK_EQ(tabs.sub_shaders[0].commands->size(), 1u);

    const auto crlf = Parse("Shader \"T\" {\r\n\tSubShader {\r\n\t\tCull Off // c\r\n\t\tPass {\r\n"
        "CGPROGRAM\r\nfloat x;\r\nENDCG\r\n}\r\n\t}\r\n}\r\n");
    CHECK_EQ(crlf.sub_shaders.size(), 1u);
    CHECK_EQ(FirstPass(crlf).shader_program->program, "\r\nfloat x;\r\n");
}

TEST(LineNumbersAfterBlockComment)
{
    CHECK_THROWS_WITH(Parse("/* a\nb\nc */ Shader \"T\" {\n Foo\n}"), "line 4:2:");
}

TEST(ColumnNumbersAfterBom)
{
    CHECK_THROWS_WITH(Parse("\xEF\xBB\xBFShader \"T\" {\n  Foo\n}"), "line 2:3:");
}

// Bug 12: escaped quotes and end-of-file comments.
TEST(StringEscapes)
{
    const auto tokens = Lexer::Tokenize("\"a\\\"b\" 'c\\'d'");
    CHECK_EQ(tokens[0].value, "a\"b");
    CHECK_EQ(tokens[1].value, "c'd");
    CHECK(tokens[2].type == TokenType::kEndOfFile);
}

TEST(UnterminatedString)
{
    CHECK_THROWS_WITH(Lexer::Tokenize("Shader \"abc\n"), "line 1:8: unterminated string literal");
}

TEST(UnterminatedBlockComment)
{
    CHECK_THROWS_WITH(Lexer::Tokenize("/* abc"), "unterminated block comment");
}

TEST(CommentAndPreprocessorAtEndOfFile)
{
    const auto tokens = Lexer::Tokenize("Shader \"T\" { } // trailing");
    CHECK(tokens[tokens.size() - 2].type == TokenType::kComment);
    CHECK_EQ(tokens[tokens.size() - 2].value, "// trailing");
    const auto pre = Lexer::Tokenize("#pragma x");
    CHECK(pre[0].type == TokenType::kPreprocessor);
    CHECK_EQ(pre[0].value, "#pragma x");
}

TEST(NumberLexing)
{
    const auto tokens = Lexer::Tokenize("-1 0.5 .5 1e-3 2D 3D 2DArray +2");
    CHECK(tokens[0].type == TokenType::kNumberLiteral);
    CHECK_EQ(tokens[0].value, "-1");
    CHECK(tokens[1].type == TokenType::kNumberLiteral);
    CHECK(tokens[2].type == TokenType::kNumberLiteral);
    CHECK(tokens[3].type == TokenType::kNumberLiteral);
    CHECK_EQ(tokens[3].value, "1e-3");
    CHECK(tokens[4].type == TokenType::kIdentifier);
    CHECK(tokens[5].type == TokenType::kIdentifier);
    CHECK_EQ(tokens[6].value, "2DArray");
    CHECK(tokens[7].type == TokenType::kNumberLiteral);
}

TEST(TokenPositions)
{
    const auto tokens = Lexer::Tokenize("Shader\n  \"T\"");
    CHECK_EQ(tokens[2].line, 2u);
    CHECK_EQ(tokens[2].pos, 2u);
    CHECK_EQ(tokens[2].offset, 9u);
    CHECK_EQ(tokens[2].length, 3u);
}

TEST(KeywordsAreCaseInsensitive)
{
    CHECK(Lexer::IsKeyword("FallBack"));
    CHECK(Lexer::IsKeyword("subshader"));
    CHECK(Lexer::IsKeyword("ZClip"));
    CHECK(!Lexer::IsKeyword("_MainTex"));
    const auto tokens = Lexer::Tokenize("FallBack ZWrite _MainTex");
    CHECK(tokens[0].type == TokenType::kKeyword);
    CHECK_EQ(tokens[0].value, "FallBack");
    CHECK(tokens[1].type == TokenType::kKeyword);
    CHECK(tokens[2].type == TokenType::kIdentifier);
}

TEST(NonAsciiInComments)
{
    const auto shader = Parse("// コメント\nShader \"日本語/名前\" { SubShader { Pass { } } }");
    CHECK_EQ(shader.name, "日本語/名前");
}

// ---------------------------------------------------------------------------- error recovery

TEST(TryParseCollectsMultipleErrors)
{
    const auto tokens = Lexer::Tokenize(R"(Shader "T" {
    Properties {
        _Good ("Good", Float) = 1
        _Bad ("Bad", Matrix) = 0
        _AlsoGood ("Also good", Color) = (1,1,1,1)
    }
    SubShader {
        Cull Sideways
        ZWrite Off
        Pass {
            Lighting Off
            Cull Front
        }
    }
    SubShader { Pass { } }
    Fallback "Diffuse"
})");
    const auto result = Parser::TryParse(tokens);
    CHECK(!result.Succeeded());
    CHECK_EQ(result.errors.size(), 3u);
    CHECK_EQ(result.errors[0].line, 4u);
    CHECK_EQ(result.errors[1].line, 8u);
    CHECK_EQ(result.errors[2].line, 11u);
    CHECK_EQ(result.shader.properties->size(), 2u);
    CHECK_EQ(result.shader.properties->at(1).name, "_AlsoGood");
    CHECK_EQ(result.shader.sub_shaders.size(), 2u);
    CHECK_EQ(Format(result.shader.sub_shaders[0].commands), (std::vector<std::string>{"ZWrite Off"}));
    CHECK_EQ(Format(FirstPass(result.shader).commands), (std::vector<std::string>{"Cull Front"}));
    CHECK_EQ(result.shader.fallback.value(), "Diffuse");
}

TEST(TryParseOnSuccess)
{
    const auto result = Parser::TryParse(Lexer::Tokenize("Shader \"T\" { SubShader { Pass { } } }"));
    CHECK(result.Succeeded());
    CHECK_EQ(result.shader.sub_shaders.size(), 1u);
}

TEST(TryParseUnclosedBlocksTerminates)
{
    const auto result = Parser::TryParse(Lexer::Tokenize("Shader \"T\" { SubShader { Pass { Cull Off "));
    CHECK(!result.Succeeded());
}

TEST(ParseErrorHasLocation)
{
    try
    {
        ParseSubShaderBody("  ZWrite Maybe");
        CHECK(false);
    }
    catch (const ParseError& e)
    {
        CHECK_EQ(e.line, 3u);
        CHECK_EQ(e.column, 10u);
        CHECK(e.message.find("expected ZWrite state") != std::string::npos);
    }
}

// ---------------------------------------------------------------------------- realistic shader

TEST(RealisticTransparentShader)
{
    const auto shader = Parse(R"(Shader "Custom/Transparent" {
    Properties {
        [MainTexture] _MainTex ("Albedo", 2D) = "white" {}
        [MainColor] _Color ("Color", Color) = (1,1,1,0.5)
        [Enum(UnityEngine.Rendering.BlendMode)] _SrcBlend ("Src", Float) = 5
        [Enum(UnityEngine.Rendering.BlendMode)] _DstBlend ("Dst", Float) = 10
        [Enum(Off, 0, On, 1)] _ZWrite ("ZWrite", Float) = 0
        [IntRange] _StencilRef ("Stencil Ref", Range(0, 255)) = 0
    }
    SubShader {
        Tags { "Queue"="Transparent" "RenderType"="Transparent" "IgnoreProjector"="True" }
        LOD 100

        Pass {
            Name "FORWARD"
            Tags { "LightMode"="ForwardBase" }
            Blend [_SrcBlend] [_DstBlend]
            ZWrite [_ZWrite]
            Cull Back
            Stencil {
                Ref [_StencilRef]
                Comp Always
                Pass Replace
            }

            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"
            sampler2D _MainTex;
            fixed4 _Color;
            struct v2f { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
            v2f vert (appdata_base v) { v2f o; o.pos = UnityObjectToClipPos(v.vertex); o.uv = v.texcoord; return o; }
            fixed4 frag (v2f i) : SV_Target { return tex2D(_MainTex, i.uv) * _Color; }
            ENDCG
        }

        Pass {
            Name "SHADOWCASTER"
            Tags { "LightMode"="ShadowCaster" }
            ZWrite On ZTest LEqual
            ColorMask 0
            Offset 1, 1
            HLSLPROGRAM
            float4 frag() : SV_Target { return 0; }
            ENDHLSL
        }
    }
    FallBack "Transparent/VertexLit"
    CustomEditor "MyShaderGUI"
})");
    CHECK_EQ(shader.properties->size(), 6u);
    CHECK_EQ(shader.sub_shaders[0].tags->size(), 3u);
    CHECK_EQ(shader.sub_shaders[0].passes.size(), 2u);
    CHECK_EQ(Format(FirstPass(shader).commands), (std::vector<std::string>{
                 "Blend [_SrcBlend] [_DstBlend]", "ZWrite [_ZWrite]", "Cull Back",
                 "Stencil { Ref [_StencilRef] Pass Replace }"
                 }));
    const auto* shadow = dynamic_cast<const Pass*>(shader.sub_shaders[0].passes[1].get());
    CHECK_EQ(Format(shadow->commands), (std::vector<std::string>{
                 "ZWrite On", "ZTest LEqual", "ColorMask 0", "Offset 1, 1"
                 }));
    CHECK_EQ(shader.fallback.value(), "Transparent/VertexLit");
    CHECK_EQ(shader.custom_editor.value(), "MyShaderGUI");
}

// ---------------------------------------------------------------------------- main

int main()
{
    int failed = 0;
    for (const auto& test : Registry())
    {
        try
        {
            test.body();
            std::cout << "[PASS] " << test.name << std::endl;
        }
        catch (const CheckFailure& failure)
        {
            ++failed;
            std::cout << "[FAIL] " << test.name << "\n    " << failure.message << std::endl;
        }
        catch (const std::exception& e)
        {
            ++failed;
            std::cout << "[FAIL] " << test.name << "\n    unexpected exception: " << e.what() << std::endl;
        }
    }

    std::cout << std::endl << Registry().size() - failed << "/" << Registry().size() << " tests passed." << std::endl;
    return failed == 0 ? 0 : 1;
}
