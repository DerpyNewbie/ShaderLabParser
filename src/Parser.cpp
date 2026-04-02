//
// Created by derpy on 2026/02/13.
//

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "Parser.h"
#include "Token.h"
#include "TokenHelper.h"

namespace
{
    const std::vector<std::string_view> kCommands = {
        "AlphaToMask",
        "Blend",
        "BlendOp",
        "ColorMask",
        "Conservative",
        "Cull",
        "Offset",
        "Stencil",
        "ZClip",
        "ZTest",
        "ZWrite"
    };
}

namespace sl_parser
{
    void Parser::ParsePropertyAttributes(TokensConstIterator& pos, PropertyAttributes* attributes)
    {
#if SHADER_LAB_PARSER_LOGGING
        std::cout << "Parsing PropertyAttributes"
            << to_string(pos->type)
            << " "
            << std::string(pos->value)
            << std::endl;
#endif

        while (pos->type == TokenType::kAttributeBegin || pos->type == TokenType::kComma)
        {
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            attributes->emplace_back(pos->value);

            pos = TokenHelper::Next(pos);
            if (pos->type == TokenType::kAttributeEnd)
                pos = TokenHelper::Next(pos);
        }
    }

    void Parser::ParsePropertyDecl(TokensConstIterator& pos, PropertyDecl* decl)
    {
#if SHADER_LAB_PARSER_LOGGING
        std::cout << "Parsing PropertyDecl: " << to_string(pos->type) << " " << std::string(pos->value) << std::endl;
#endif

        if (pos->type == TokenType::kAttributeBegin)
        {
            auto attributes = PropertyAttributes{};
            ParsePropertyAttributes(pos, &attributes);
            decl->attributes = attributes;
        }

        if (pos->type == TokenType::kIdentifier)
        {
            decl->name = pos->value;
        }

        pos = TokenHelper::NextType(pos, TokenType::kBracketBegin);
        pos = TokenHelper::NextType(pos, TokenType::kStringLiteral);
        decl->display_name = pos->value;
        pos = TokenHelper::NextType(pos, TokenType::kComma);
        pos = TokenHelper::Next(pos);
        std::string buff;
        while (pos->type != TokenType::kBracketEnd)
        {
            buff += pos->value;
            pos = TokenHelper::Next(pos);
        }
        decl->type = buff;

        pos = TokenHelper::NextType(pos, TokenType::kOperator);
        pos = TokenHelper::Next(pos);

        switch (pos->type)
        {
        case TokenType::kStringLiteral:
            {
                decl->default_value = pos->value;
                pos = TokenHelper::NextType(pos, TokenType::kBlockBegin);
                pos = TokenHelper::NextType(pos, TokenType::kBlockEnd);
                break;
            }
        case TokenType::kBracketBegin:
            {
                buff = "";

                while (pos->type != TokenType::kBracketEnd)
                {
                    buff += pos->value;
                    pos = TokenHelper::Next(pos);
                }

                decl->default_value = buff;
                pos = TokenHelper::Next(pos);
                break;
            }
        default:
            {
                throw std::runtime_error("unexpected token " + to_string(pos->type) + ".");
            }
        }

        pos = TokenHelper::Next(pos);
    }

    void Parser::ParseProperties(TokensConstIterator pos, Properties* properties)
    {
        pos = TokenHelper::Next(pos);
        while (pos->type != TokenType::kBlockEnd)
        {
            PropertyDecl decl{};
            ParsePropertyDecl(pos, &decl);
            properties->emplace_back(decl);
        }
    }

    void Parser::ParseTags(TokensConstIterator& pos, std::optional<Tags>* map)
    {
        pos = TokenHelper::NextType(pos, TokenType::kBlockBegin);
        if (map->has_value() == false)
        {
            map->emplace();
        }

        while (pos->type != TokenType::kBlockEnd)
        {
#if SHADER_LAB_PARSER_LOGGING
            std::cout << "Parsing Tags: " << to_string(pos->type) << " " << std::string(pos->value) << std::endl;
#endif

            pos = TokenHelper::NextType(pos, TokenType::kStringLiteral);
            auto key = pos->value;
            pos = TokenHelper::NextType(pos, TokenType::kOperator);
            if (pos->value != "=")
            {
                throw std::runtime_error("expected '=' operator, got " + pos->value + ".");
            }

            pos = TokenHelper::NextType(pos, TokenType::kStringLiteral);
            auto value = pos->value;
            map->value().emplace(key, value);

            pos = TokenHelper::Next(pos);
        }
    }

    void Parser::ParseShaderProgram(TokensConstIterator& pos, ShaderProgram* program)
    {
#if SHADER_LAB_PARSER_LOGGING
        std::cout << "Parsing Program: " << to_string(pos->type) << " " << pos->value << std::endl;
#endif

        std::string end = "ENDCG";
        if (pos->value == "CGPROGRAM")
        {
            end = "ENDCG";
        }
        else if (pos->value == "HLSLPROGRAM")
        {
            end = "ENDHLSL";
        }
        else
        {
            throw std::runtime_error("expected CGPROGRAM or HLSLPROGRAM, got " + pos->value + ".");
        }

        pos = TokenHelper::Next(pos);

        std::stringstream ss;
        while (pos->type != TokenType::kKeyword && pos->value != end)
        {
            ss << pos->value;
            if (pos->type != TokenType::kNewLine)
            {
                ss << " ";
            }

            pos = ++pos;
        }

        program->program = ss.str();
    }

    void Parser::ParsePass(TokensConstIterator& pos, Pass* pass)
    {
        pos = TokenHelper::NextType(pos, TokenType::kBlockBegin);
        pos = TokenHelper::Next(pos);
        while (pos->type != TokenType::kBlockEnd)
        {
#if SHADER_LAB_PARSER_LOGGING
            std::cout << "Parsing Pass: " << to_string(pos->type) << " " << std::string(pos->value) << std::endl;
#endif

            if (pos->type != TokenType::kKeyword)
            {
                throw std::runtime_error(
                    "expected keyword, got " + to_string(pos->type) + " " + std::string(pos->value) + ".");
            }

            if (pos->value == "Tags")
            {
                ParseTags(pos, &pass->tags);
                pos = TokenHelper::Next(pos);
            }
            else if (pos->value == "Name")
            {
                pos = TokenHelper::NextType(pos, TokenType::kStringLiteral);
                pass->name = pos->value;
                pos = TokenHelper::Next(pos);
            }
            else if (pos->value == "CGPROGRAM" || pos->value == "HLSLPROGRAM")
            {
                pass->shader_program.emplace();
                ParseShaderProgram(pos, &pass->shader_program.value());
                pos = TokenHelper::Next(pos);
            }
            else
            {
                throw std::runtime_error("unexpected keyword " + pos->value + ".");
            }
        }
    }

    void Parser::ParseCommand(TokensConstIterator& pos, std::optional<Commands>* commands)
    {
        auto parse_render_target = [](const std::string& value)
        {
            if (value == "1")
            {
                return RenderTarget::One;
            }
            if (value == "2")
            {
                return RenderTarget::Two;
            }
            if (value == "3")
            {
                return RenderTarget::Three;
            }
            if (value == "4")
            {
                return RenderTarget::Four;
            }
            if (value == "5")
            {
                return RenderTarget::Five;
            }
            if (value == "6")
            {
                return RenderTarget::Six;
            }
            if (value == "7")
            {
                return RenderTarget::Seven;
            }
            if (value == "8")
            {
                return RenderTarget::Eight;
            }

            throw std::runtime_error("expected render target, got " + value + ".");
        };

        if (!commands->has_value())
        {
            commands->emplace();
        }

        if (pos->value == "AlphaToMask")
        {
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            AlphaToMask::State state;
            if (pos->value == "On")
            {
                state = AlphaToMask::State::On;
            }
            else if (pos->value == "Off")
            {
                state = AlphaToMask::State::Off;
            }
            else
            {
                throw std::runtime_error("expected On or Off, got " + pos->value + ".");
            }

            auto alpha_to_mask = std::make_unique<AlphaToMask>();
            alpha_to_mask->state = state;
            commands->value().push_back(std::move(alpha_to_mask));

            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            return;
        }

        if (pos->value == "Blend")
        {
            auto parse_blend_factor = [](const std::string& value)
            {
                if (value == "One")
                {
                    return Blend::Factor::One;
                }
                if (value == "Zero")
                {
                    return Blend::Factor::Zero;
                }

                if (value == "SrcColor")
                {
                    return Blend::Factor::SrcColor;
                }
                if (value == "OneMinusSrcColor")
                {
                    return Blend::Factor::OneMinusSrcColor;
                }
                if (value == "DstColor")
                {
                    return Blend::Factor::DstColor;
                }
                if (value == "OneMinusDstColor")
                {
                    return Blend::Factor::OneMinusDstColor;
                }
                if (value == "SrcAlpha")
                {
                    return Blend::Factor::SrcAlpha;
                }
                if (value == "OneMinusSrcAlpha")
                {
                    return Blend::Factor::OneMinusSrcAlpha;
                }
                if (value == "DstAlpha")
                {
                    return Blend::Factor::DstAlpha;
                }
                if (value == "OneMinusDstAlpha")
                {
                    return Blend::Factor::OneMinusDstAlpha;
                }

                throw std::runtime_error("expected blend factor, got " + value + ".");
            };

            pos = TokenHelper::Next(pos);
            auto blend = std::make_unique<Blend>();
            if (pos->type == TokenType::kNumberLiteral)
            {
                blend->target = parse_render_target(pos->value);
                pos = TokenHelper::Next(pos);
            }

            TokenHelper::ExpectType(pos, TokenType::kIdentifier);

            if (pos->value == "Off")
            {
                blend->state = false;
                commands->value().push_back(std::move(blend));
                pos = TokenHelper::NextType(pos, TokenType::kNewLine);
                return;
            }

            blend->state = true;
            blend->src_factor = parse_blend_factor(pos->value);

            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            blend->dst_factor = parse_blend_factor(pos->value);

            pos = TokenHelper::Next(pos);
            if (pos->type == TokenType::kNewLine)
            {
                commands->value().push_back(std::move(blend));
                return;
            }

            if (pos->type != TokenType::kComma)
            {
                throw std::runtime_error("expected comma, got " + to_string(pos->type) + ".");
            }

            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            blend->alpha_src_factor = parse_blend_factor(pos->value);
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            blend->alpha_dst_factor = parse_blend_factor(pos->value);
            commands->value().push_back(std::move(blend));

            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            return;
        }

        if (pos->value == "BlendOp")
        {
            auto parse_operation = [](const std::string& value)
            {
                if (value == "Add")
                {
                    return BlendOp::Op::Add;
                }

                if (value == "Sub")
                {
                    return BlendOp::Op::Sub;
                }
                if (value == "RevSub")
                {
                    return BlendOp::Op::RevSub;
                }
                if (value == "Min")
                {
                    return BlendOp::Op::Min;
                }
                if (value == "Max")
                {
                    return BlendOp::Op::Max;
                }
                if (value == "LogicalClear")
                {
                    return BlendOp::Op::LogicalClear;
                }
                if (value == "LogicalSet")
                {
                    return BlendOp::Op::LogicalSet;
                }
                if (value == "LogicalCopy")
                {
                    return BlendOp::Op::LogicalCopy;
                }
                if (value == "LogicalCopyInverted")
                {
                    return BlendOp::Op::LogicalCopyInverted;
                }
                if (value == "LogicalNoop")
                {
                    return BlendOp::Op::LogicalNoop;
                }
                if (value == "LogicalInvert")
                {
                    return BlendOp::Op::LogicalInvert;
                }
                if (value == "LogicalAnd")
                {
                    return BlendOp::Op::LogicalAnd;
                }
                if (value == "LogicalNand")
                {
                    return BlendOp::Op::LogicalNand;
                }
                if (value == "LogicalOr")
                {
                    return BlendOp::Op::LogicalOr;
                }
                if (value == "LogicalNor")
                {
                    return BlendOp::Op::LogicalNor;
                }
                if (value == "LogicalXor")
                {
                    return BlendOp::Op::LogicalXor;
                }
                if (value == "LogicalEquiv")
                {
                    return BlendOp::Op::LogicalEquiv;
                }
                if (value == "LogicalAndReverse")
                {
                    return BlendOp::Op::LogicalAndReverse;
                }
                if (value == "LogicalAndInverted")
                {
                    return BlendOp::Op::LogicalAndInverted;
                }
                if (value == "LogicalOrReverse")
                {
                    return BlendOp::Op::LogicalOrReverse;
                }
                if (value == "LogicalOrInverted")
                {
                    return BlendOp::Op::LogicalOrInverted;
                }
                if (value == "Multiply")
                {
                    return BlendOp::Op::Multiply;
                }
                if (value == "Screen")
                {
                    return BlendOp::Op::Screen;
                }
                if (value == "Overlay")
                {
                    return BlendOp::Op::Overlay;
                }
                if (value == "Darken")
                {
                    return BlendOp::Op::Darken;
                }
                if (value == "Lighten")
                {
                    return BlendOp::Op::Lighten;
                }
                if (value == "ColorDodge")
                {
                    return BlendOp::Op::ColorDodge;
                }
                if (value == "ColorBurn")
                {
                    return BlendOp::Op::ColorBurn;
                }
                if (value == "HardLight")
                {
                    return BlendOp::Op::HardLight;
                }
                if (value == "SoftLight")
                {
                    return BlendOp::Op::SoftLight;
                }
                if (value == "Difference")
                {
                    return BlendOp::Op::Difference;
                }
                if (value == "Exclusion")
                {
                    return BlendOp::Op::Exclusion;
                }
                if (value == "HSLHue")
                {
                    return BlendOp::Op::HSLHue;
                }
                if (value == "HSLSaturation")
                {
                    return BlendOp::Op::HSLSaturation;
                }
                if (value == "HSLColor")
                {
                    return BlendOp::Op::HSLColor;
                }
                if (value == "HSLLuminosity")
                {
                    return BlendOp::Op::HSLLuminosity;
                }

                throw std::runtime_error("expected blend operation, got " + value + ".");
            };
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);

            auto blend_op = std::make_unique<BlendOp>();
            blend_op->operation = parse_operation(pos->value);
            commands->value().push_back(std::move(blend_op));

            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            return;
        }

        if (pos->value == "ColorMask")
        {
            pos = TokenHelper::Next(pos);
            auto color_mask = std::make_unique<ColorMask>();

            const auto mask = pos->value;
            for (auto i = 0; i < mask.length(); ++i)
            {
                switch (mask[i])
                {
                case '0':
                    color_mask->channels = ColorMask::Channels::Zero;
                    break;
                case 'R':
                    color_mask->channels = static_cast<ColorMask::Channels>(
                        static_cast<uint8_t>(color_mask->channels) | static_cast<uint8_t>(ColorMask::Channels::R));
                    break;
                case 'G':
                    color_mask->channels = static_cast<ColorMask::Channels>(
                        static_cast<uint8_t>(color_mask->channels) | static_cast<uint8_t>(ColorMask::Channels::G));
                    break;
                case 'B':
                    color_mask->channels = static_cast<ColorMask::Channels>(
                        static_cast<uint8_t>(color_mask->channels) | static_cast<uint8_t>(ColorMask::Channels::B));
                    break;
                case 'A':
                    color_mask->channels = static_cast<ColorMask::Channels>(
                        static_cast<uint8_t>(color_mask->channels) | static_cast<uint8_t>(ColorMask::Channels::A));
                    break;
                default:
                    throw std::runtime_error("expected color mask, got " + pos->value + ".");
                }
            }

            pos = TokenHelper::Next(pos);
            if (pos->type == TokenType::kNumberLiteral)
            {
                color_mask->target = parse_render_target(pos->value);
                pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            }

            TokenHelper::ExpectType(pos, TokenType::kNewLine);
            commands->value().push_back(std::move(color_mask));
            return;
        }

        if (pos->value == "Conservative")
        {
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            auto conservative = std::make_unique<Conservative>();
            Conservative::Enabled enabled;
            if (pos->value == "True")
            {
                enabled = Conservative::Enabled::True;
            }
            else if (pos->value == "False")
            {
                enabled = Conservative::Enabled::False;
            }
            else
            {
                throw std::runtime_error("expected True or False, got " + pos->value + ".");
            }

            conservative->enabled = enabled;
            commands->value().push_back(std::move(conservative));
            return;
        }

        if (pos->value == "Cull")
        {
            auto parse_cull_mode = [](const std::string& value)
            {
                if (value == "Off")
                {
                    return Cull::State::Off;
                }
                if (value == "Front")
                {
                    return Cull::State::Front;
                }
                if (value == "Back")
                {
                    return Cull::State::Back;
                }

                throw std::runtime_error("expected cull mode, got " + value + ".");
            };

            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            auto cull = std::make_unique<Cull>();
            cull->state = parse_cull_mode(pos->value);
            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            commands->value().push_back(std::move(cull));
            return;
        }

        if (pos->value == "Offset")
        {
            pos = TokenHelper::NextType(pos, TokenType::kNumberLiteral);
            auto offset = std::make_unique<Offset>();
            offset->factor = stod(pos->value);
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            offset->units = stod(pos->value);
            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            commands->value().push_back(std::move(offset));
            return;
        }

        if (pos->value == "Stencil")
        {
            auto parse_comparison_values = [](const std::string& value)
            {
                if (value == "Never")
                {
                    return Stencil::Comparison::Never;
                }
                if (value == "Less")
                {
                    return Stencil::Comparison::Less;
                }
                if (value == "Equal")
                {
                    return Stencil::Comparison::Equal;
                }
                if (value == "LEqual")
                {
                    return Stencil::Comparison::LEqual;
                }
                if (value == "Greater")
                {
                    return Stencil::Comparison::Greater;
                }
                if (value == "NotEqual")
                {
                    return Stencil::Comparison::NotEqual;
                }
                if (value == "GEqual")
                {
                    return Stencil::Comparison::GEqual;
                }
                if (value == "Always")
                {
                    return Stencil::Comparison::Always;
                }

                throw std::runtime_error("expected comparison value, got " + value + ".");
            };
            auto parse_operation_values = [](const std::string& value)
            {
                if (value == "Keep")
                {
                    return Stencil::Operation::Keep;
                }
                if (value == "Zero")
                {
                    return Stencil::Operation::Zero;
                }
                if (value == "Replace")
                {
                    return Stencil::Operation::Replace;
                }
                if (value == "IncrSat")
                {
                    return Stencil::Operation::IncrSat;
                }
                if (value == "DecrSat")
                {
                    return Stencil::Operation::DecrSat;
                }
                if (value == "Invert")
                {
                    return Stencil::Operation::Invert;
                }
                if (value == "IncrWrap")
                {
                    return Stencil::Operation::IncrWrap;
                }
                if (value == "DecrWrap")
                {
                    return Stencil::Operation::DecrWrap;
                }

                throw std::runtime_error("expected stencil value, got " + value + ".");
            };
            auto stencil = std::make_unique<Stencil>();
            pos = TokenHelper::NextType(pos, TokenType::kBlockBegin);
            auto tmp_pos = pos;
            while (tmp_pos->type != TokenType::kEndOfFile)
            {
                tmp_pos = TokenHelper::NextInBlock(tmp_pos);

                if (tmp_pos->value == "Ref")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kNumberLiteral);
                    stencil->ref = stoi(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "ReadMask")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kNumberLiteral);
                    stencil->read_mask = stoi(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "WriteMask")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kNumberLiteral);
                    stencil->write_mask = stoi(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "Comp")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->comparison_operation = parse_comparison_values(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "Fail")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->fail_operation = parse_operation_values(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "ZFail")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->z_fail_operation = parse_operation_values(tmp_pos->value);
                }
                if (tmp_pos->value == "CompBack")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->comparison_operation_back = parse_comparison_values(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "PassBack")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->pass_operation_back = parse_operation_values(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "FailBack")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->fail_operation_back = parse_operation_values(tmp_pos->value);
                }
                if (tmp_pos->value == "ZFailBack")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->z_fail_operation_back = parse_operation_values(tmp_pos->value);
                }
                if (tmp_pos->value == "CompFront")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->comparison_operation_front = parse_comparison_values(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "PassFront")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->pass_operation_front = parse_operation_values(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "FailFront")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->fail_operation_front = parse_operation_values(tmp_pos->value);
                    continue;
                }
                if (tmp_pos->value == "ZFailFront")
                {
                    tmp_pos = TokenHelper::NextType(tmp_pos, TokenType::kIdentifier);
                    stencil->z_fail_operation_front = parse_operation_values(tmp_pos->value);
                    continue;
                }

                throw std::runtime_error("expected stencil syntax, got " + tmp_pos->value + ".");
            }

            pos = TokenHelper::NextInBlock(--pos);
            TokenHelper::ExpectType(pos, TokenType::kNewLine);
            commands->value().push_back(std::move(stencil));
            return;
        }

        if (pos->value == "ZClip")
        {
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            auto z_clip = std::make_unique<ZClip>();
            ZClip::Enabled enabled;
            if (pos->value == "True")
            {
                enabled = ZClip::Enabled::True;
            }
            else if (pos->value == "False")
            {
                enabled = ZClip::Enabled::False;
            }
            else
            {
                throw std::runtime_error("expected True or False, got " + pos->value + ".");
            }

            z_clip->enabled = enabled;

            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            commands->value().push_back(std::move(z_clip));
            return;
        }

        if (pos->value == "ZTest")
        {
            auto parse_z_test_values = [](const std::string& value)
            {
                if (value == "Disabled")
                {
                    return ZTest::Operation::Disabled;
                }
                if (value == "Never")
                {
                    return ZTest::Operation::Never;
                }
                if (value == "Less")
                {
                    return ZTest::Operation::Less;
                }
                if (value == "Equal")
                {
                    return ZTest::Operation::Equal;
                }
                if (value == "LEqual")
                {
                    return ZTest::Operation::LEqual;
                }
                if (value == "Greater")
                {
                    return ZTest::Operation::Greater;
                }
                if (value == "NotEqual")
                {
                    return ZTest::Operation::NotEqual;
                }
                if (value == "GEqual")
                {
                    return ZTest::Operation::GEqual;
                }
                if (value == "Always")
                {
                    return ZTest::Operation::Always;
                }

                throw std::runtime_error("expected z test value, got " + value + ".");
            };

            auto z_test = std::make_unique<ZTest>();
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            z_test->operation = parse_z_test_values(pos->value);
            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            commands->value().push_back(std::move(z_test));
            return;
        }

        if (pos->value == "ZWrite")
        {
            pos = TokenHelper::NextType(pos, TokenType::kIdentifier);
            auto z_write = std::make_unique<ZWrite>();
            ZWrite::State state;
            if (pos->value == "On")
            {
                state = ZWrite::State::On;
            }
            else if (pos->value == "Off")
            {
                state = ZWrite::State::Off;
            }
            else
            {
                throw std::runtime_error("expected On or Off, got " + pos->value + ".");
            }

            z_write->state = state;

            pos = TokenHelper::NextType(pos, TokenType::kNewLine);
            commands->value().push_back(std::move(z_write));
            return;
        }
    }

    void Parser::ParseSubShader(TokensConstIterator& pos, SubShader* sub_shader)
    {
        pos = TokenHelper::Next(pos);
        uint8_t pass_order = 0;
        while (pos->type != TokenType::kBlockEnd)
        {
            if (pos->type != TokenType::kKeyword)
            {
                throw std::runtime_error(
                    "expected keyword, got " + to_string(pos->type) + " " + std::string(pos->value) + ".");
            }

#if SHADER_LAB_PARSER_LOGGING
            std::cout << "Parsing SubShader: " << to_string(pos->type) << " " << std::string(pos->value) << std::endl;
#endif

            if (IsCommand(pos->value))
            {
                ParseCommand(pos, &sub_shader->commands);
                pos = TokenHelper::Next(pos);
            }
            else if (pos->value == "Tags")
            {
                ParseTags(pos, &sub_shader->tags);
                pos = TokenHelper::Next(pos);
            }
            else if (pos->value == "LOD")
            {
                pos = TokenHelper::NextType(pos, TokenType::kNumberLiteral);
                sub_shader->lod = stoi(pos->value);
                pos = TokenHelper::Next(pos);
            }
            else if (pos->value == "Pass")
            {
                auto pass = std::make_unique<Pass>();
                pass->order = pass_order++;
                ParsePass(pos, pass.get());
                sub_shader->passes.emplace_back(std::move(pass));
                pos = TokenHelper::Next(pos);
            }
            else if (pos->value == "GrabPass")
            {
                auto grab_pass = std::make_unique<GrabPass>();
                grab_pass->order = pass_order++;
                sub_shader->passes.emplace_back(std::move(grab_pass));
                pos = TokenHelper::Next(pos);
            }
            else if (pos->value == "UsePass")
            {
                auto use_pass = std::make_unique<UsePass>();
                pos = TokenHelper::NextType(pos, TokenType::kStringLiteral);
                use_pass->order = pass_order++;
                use_pass->target_pass = pos->value;
                sub_shader->passes.emplace_back(std::move(use_pass));
                pos = TokenHelper::Next(pos);
            }
        }
    }

    bool Parser::IsCommand(const std::string& value)
    {
        return std::ranges::find_if(
            kCommands, [&](const auto& keyword) { return keyword == value; }
        ) != kCommands.end();
    }

    ShaderLabObject Parser::Parse(const Tokens& tokens)
    {
        const auto shader = TokenHelper::NextType(tokens.begin(), TokenType::kKeyword);
        if (shader->value != "Shader")
        {
            throw std::runtime_error("expected Shader keyword, got " + shader->value + ".");
        }

        const auto shader_name_literal = TokenHelper::NextType(shader, TokenType::kStringLiteral);
        ShaderLabObject shader_lab_object{shader_name_literal->value};
        for (auto it = TokenHelper::NextInBlock(TokenHelper::Next(shader_name_literal)); !TokenHelper::IsEOF(it); it =
             TokenHelper::NextInBlock(it))
        {
#if SHADER_LAB_PARSER_LOGGING
            std::cout << "Parsing: " << to_string(it->type) << " " << std::string(it->value) << std::endl;
#endif

            if (it->type != TokenType::kKeyword)
            {
                continue;
            }

            if (it->value == "Properties")
            {
                Properties properties;
                it = TokenHelper::NextType(it, TokenType::kBlockBegin);
                ParseProperties(it, &properties);
                shader_lab_object.properties = properties;
            }
            else if (it->value == "SubShader")
            {
                SubShader sub_shader;
                it = TokenHelper::NextType(it, TokenType::kBlockBegin);
                ParseSubShader(it, &sub_shader);
                shader_lab_object.sub_shaders.emplace_back(std::move(sub_shader));
            }
            else if (it->value == "CustomEditor")
            {
                it = TokenHelper::NextType(it, TokenType::kOperator);
                if (it->value != "=")
                {
                    throw std::runtime_error("expected '=' operator, got " + it->value + ".");
                }

                it = TokenHelper::NextType(it, TokenType::kStringLiteral);
                shader_lab_object.custom_editor = it->value;
            }
            else if (it->value == "Fallback")
            {
                it = TokenHelper::NextType(it, TokenType::kStringLiteral);
                shader_lab_object.fallback = it->value;
            }
            else
            {
                throw std::runtime_error(
                    "unexpected keyword " + it->value + ". expected <Properties|SubShader|CustomEditor|Fallback>.");
            }
        }
        return shader_lab_object;
    }
}
