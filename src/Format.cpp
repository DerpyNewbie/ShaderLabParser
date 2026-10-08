//
// Created by derpy on 2026/02/13.
//

#include <locale>
#include <sstream>
#include "EnumTables.h"
#include "Format.h"

namespace
{
    using namespace sl_parser;

    std::string FormatFloat(const float value)
    {
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        stream << value;
        return stream.str();
    }

    std::string FormatValue(const Value<float>& value)
    {
        return value.IsProperty() ? "[" + *value.property + "]" : FormatFloat(value.value);
    }

    std::string FormatValue(const Value<uint8_t>& value)
    {
        return value.IsProperty() ? "[" + *value.property + "]" : std::to_string(value.value);
    }

    template <typename E, size_t N>
    std::string FormatValue(const tables::Entry<E> (&table)[N], const Value<E>& value)
    {
        return value.IsProperty() ? "[" + *value.property + "]" : std::string(tables::NameOf<E>(table, value.value));
    }

    std::string FormatTarget(const std::optional<uint8_t>& target)
    {
        return target.has_value() ? " " + std::to_string(*target) : "";
    }

    std::string FormatChannels(const Value<ColorMask::Channels>& channels)
    {
        if (channels.IsProperty()) return "[" + *channels.property + "]";
        const auto mask = static_cast<uint8_t>(channels.value);
        if (mask == 0) return "0";
        std::string result;
        if (mask & static_cast<uint8_t>(ColorMask::Channels::R)) result += 'R';
        if (mask & static_cast<uint8_t>(ColorMask::Channels::G)) result += 'G';
        if (mask & static_cast<uint8_t>(ColorMask::Channels::B)) result += 'B';
        if (mask & static_cast<uint8_t>(ColorMask::Channels::A)) result += 'A';
        return result;
    }

    std::string FormatStencil(const Stencil& s)
    {
        std::string result = "Stencil { Ref " + FormatValue(s.ref);
        auto add_byte = [&](const char* name, const Value<uint8_t>& v)
        {
            if (v.IsProperty() || v.value != 0xFF) result += std::string(" ") + name + " " + FormatValue(v);
        };
        auto add_cmp = [&](const char* name, const Value<Stencil::Comparison>& v)
        {
            if (v.IsProperty() || v.value != Stencil::Comparison::Always)
                result += std::string(" ") + name + " " + FormatValue(tables::kStencilComparison, v);
        };
        auto add_op = [&](const char* name, const Value<Stencil::Operation>& v)
        {
            if (v.IsProperty() || v.value != Stencil::Operation::Keep)
                result += std::string(" ") + name + " " + FormatValue(tables::kStencilOperation, v);
        };

        add_byte("ReadMask", s.read_mask);
        add_byte("WriteMask", s.write_mask);
        add_cmp("Comp", s.comparison_operation);
        add_op("Pass", s.pass_operation);
        add_op("Fail", s.fail_operation);
        add_op("ZFail", s.z_fail_operation);
        add_cmp("CompBack", s.comparison_operation_back);
        add_op("PassBack", s.pass_operation_back);
        add_op("FailBack", s.fail_operation_back);
        add_op("ZFailBack", s.z_fail_operation_back);
        add_cmp("CompFront", s.comparison_operation_front);
        add_op("PassFront", s.pass_operation_front);
        add_op("FailFront", s.fail_operation_front);
        add_op("ZFailFront", s.z_fail_operation_front);
        return result + " }";
    }

    std::string FormatFog(const Fog& fog)
    {
        std::string result = "Fog {";
        if (fog.mode) result += " Mode " + FormatValue(tables::kFogMode, *fog.mode);
        if (fog.color)
        {
            if (fog.color->IsProperty())
            {
                result += " Color [" + *fog.color->property + "]";
            }
            else
            {
                const auto& c = fog.color->value;
                result += " Color (" + FormatFloat(c[0]) + "," + FormatFloat(c[1]) + "," + FormatFloat(c[2]) + "," +
                    FormatFloat(c[3]) + ")";
            }
        }
        if (fog.density) result += " Density " + FormatValue(*fog.density);
        if (fog.range) result += " Range " + FormatValue(fog.range->first) + ", " + FormatValue(fog.range->second);
        return result + " }";
    }
}

namespace sl_parser
{
    std::string FormatCommand(const Command& command)
    {
        switch (command.GetCommandType())
        {
        case CommandType::AlphaToMask:
            return "AlphaToMask " + FormatValue(tables::kAlphaToMask, static_cast<const AlphaToMask&>(command).state);
        case CommandType::Blend:
            {
                const auto& c = static_cast<const Blend&>(command);
                std::string result = "Blend" + FormatTarget(c.target);
                if (!c.state) return result + " Off";
                result += " " + FormatValue(tables::kBlendFactor, c.src_factor) + " " +
                    FormatValue(tables::kBlendFactor, c.dst_factor);
                if (c.separate_alpha)
                {
                    result += ", " + FormatValue(tables::kBlendFactor, c.alpha_src_factor) + " " +
                        FormatValue(tables::kBlendFactor, c.alpha_dst_factor);
                }
                return result;
            }
        case CommandType::BlendOp:
            {
                const auto& c = static_cast<const BlendOp&>(command);
                std::string result = "BlendOp" + FormatTarget(c.target) + " " +
                    FormatValue(tables::kBlendOp, c.operation);
                if (c.alpha_operation) result += ", " + FormatValue(tables::kBlendOp, *c.alpha_operation);
                return result;
            }
        case CommandType::ColorMask:
            {
                const auto& c = static_cast<const ColorMask&>(command);
                return "ColorMask " + FormatChannels(c.channels) + FormatTarget(c.target);
            }
        case CommandType::Conservative:
            return "Conservative " +
                FormatValue(tables::kConservative, static_cast<const Conservative&>(command).enabled);
        case CommandType::Cull:
            return "Cull " + FormatValue(tables::kCull, static_cast<const Cull&>(command).state);
        case CommandType::Offset:
            {
                const auto& c = static_cast<const Offset&>(command);
                return "Offset " + FormatValue(c.factor) + ", " + FormatValue(c.units);
            }
        case CommandType::Stencil:
            return FormatStencil(static_cast<const Stencil&>(command));
        case CommandType::ZClip:
            return "ZClip " + FormatValue(tables::kZClip, static_cast<const ZClip&>(command).enabled);
        case CommandType::ZTest:
            return "ZTest " + FormatValue(tables::kZTest, static_cast<const ZTest&>(command).operation);
        case CommandType::ZWrite:
            return "ZWrite " + FormatValue(tables::kZWrite, static_cast<const ZWrite&>(command).state);
        case CommandType::Fog:
            return FormatFog(static_cast<const Fog&>(command));
        }
        return "?";
    }
}

std::string to_string(const sl_parser::CommandType type)
{
    return std::string(sl_parser::tables::NameOf<sl_parser::CommandType>(sl_parser::tables::kCommandType, type));
}
