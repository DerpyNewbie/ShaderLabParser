//
// Created by derpy on 2026/02/13.
//
// Internal ShaderLab name <-> enum tables shared by the parser and formatter. Not part of the installed headers.
//
#pragma once
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include "Command.h"
#include "StringUtil.h"

namespace sl_parser::tables
{
    template <typename E>
    struct Entry
    {
        std::string_view name;
        E value;
    };

    /// <summary>Looks up a value by name, ignoring case.</summary>
    /// <param name="table">Table to search.</param>
    /// <param name="name">Name to find.</param>
    /// <returns>The value, or empty if the name is not in the table.</returns>
    template <typename E>
    std::optional<E> Find(const std::span<const Entry<E>> table, const std::string_view name)
    {
        for (const auto& entry : table)
        {
            if (EqualsIgnoreCase(entry.name, name)) return entry.value;
        }
        return std::nullopt;
    }

    /// <summary>Finds the first name registered for a value.</summary>
    /// <param name="table">Table to search.</param>
    /// <param name="value">Value to find.</param>
    /// <returns>The name, or <c>?</c> if the value is not in the table.</returns>
    template <typename E>
    std::string_view NameOf(const std::span<const Entry<E>> table, const E value)
    {
        for (const auto& entry : table)
        {
            if (entry.value == value) return entry.name;
        }
        return "?";
    }

    template <typename E>
    std::string Names(const std::span<const Entry<E>> table)
    {
        std::string result;
        for (const auto& entry : table)
        {
            if (!result.empty()) result += ", ";
            result += entry.name;
        }
        return result;
    }

    inline constexpr Entry<AlphaToMask::State> kAlphaToMask[] = {
        {"Off", AlphaToMask::State::Off},
        {"On", AlphaToMask::State::On},
    };

    inline constexpr Entry<Blend::Factor> kBlendFactor[] = {
        {"One", Blend::Factor::One},
        {"Zero", Blend::Factor::Zero},
        {"SrcColor", Blend::Factor::SrcColor},
        {"SrcAlpha", Blend::Factor::SrcAlpha},
        {"DstColor", Blend::Factor::DstColor},
        {"DstAlpha", Blend::Factor::DstAlpha},
        {"OneMinusSrcColor", Blend::Factor::OneMinusSrcColor},
        {"OneMinusSrcAlpha", Blend::Factor::OneMinusSrcAlpha},
        {"OneMinusDstColor", Blend::Factor::OneMinusDstColor},
        {"OneMinusDstAlpha", Blend::Factor::OneMinusDstAlpha},
        {"SrcAlphaSaturate", Blend::Factor::SrcAlphaSaturate},
    };

    inline constexpr Entry<BlendOp::Op> kBlendOp[] = {
        {"Add", BlendOp::Op::Add},
        {"Sub", BlendOp::Op::Sub},
        {"RevSub", BlendOp::Op::RevSub},
        {"Min", BlendOp::Op::Min},
        {"Max", BlendOp::Op::Max},
        {"LogicalClear", BlendOp::Op::LogicalClear},
        {"LogicalSet", BlendOp::Op::LogicalSet},
        {"LogicalCopy", BlendOp::Op::LogicalCopy},
        {"LogicalCopyInverted", BlendOp::Op::LogicalCopyInverted},
        {"LogicalNoop", BlendOp::Op::LogicalNoop},
        {"LogicalInvert", BlendOp::Op::LogicalInvert},
        {"LogicalAnd", BlendOp::Op::LogicalAnd},
        {"LogicalNand", BlendOp::Op::LogicalNand},
        {"LogicalOr", BlendOp::Op::LogicalOr},
        {"LogicalNor", BlendOp::Op::LogicalNor},
        {"LogicalXor", BlendOp::Op::LogicalXor},
        {"LogicalEquiv", BlendOp::Op::LogicalEquiv},
        {"LogicalAndReverse", BlendOp::Op::LogicalAndReverse},
        {"LogicalAndInverted", BlendOp::Op::LogicalAndInverted},
        {"LogicalOrReverse", BlendOp::Op::LogicalOrReverse},
        {"LogicalOrInverted", BlendOp::Op::LogicalOrInverted},
        {"Multiply", BlendOp::Op::Multiply},
        {"Screen", BlendOp::Op::Screen},
        {"Overlay", BlendOp::Op::Overlay},
        {"Darken", BlendOp::Op::Darken},
        {"Lighten", BlendOp::Op::Lighten},
        {"ColorDodge", BlendOp::Op::ColorDodge},
        {"ColorBurn", BlendOp::Op::ColorBurn},
        {"HardLight", BlendOp::Op::HardLight},
        {"SoftLight", BlendOp::Op::SoftLight},
        {"Difference", BlendOp::Op::Difference},
        {"Exclusion", BlendOp::Op::Exclusion},
        {"HSLHue", BlendOp::Op::HSLHue},
        {"HSLSaturation", BlendOp::Op::HSLSaturation},
        {"HSLColor", BlendOp::Op::HSLColor},
        {"HSLLuminosity", BlendOp::Op::HSLLuminosity},
    };

    inline constexpr Entry<Conservative::Enabled> kConservative[] = {
        {"False", Conservative::Enabled::False},
        {"True", Conservative::Enabled::True},
    };

    inline constexpr Entry<Cull::State> kCull[] = {
        {"Off", Cull::State::Off},
        {"Back", Cull::State::Back},
        {"Front", Cull::State::Front},
    };

    inline constexpr Entry<Stencil::Comparison> kStencilComparison[] = {
        {"Never", Stencil::Comparison::Never},
        {"Less", Stencil::Comparison::Less},
        {"Equal", Stencil::Comparison::Equal},
        {"LEqual", Stencil::Comparison::LEqual},
        {"Greater", Stencil::Comparison::Greater},
        {"NotEqual", Stencil::Comparison::NotEqual},
        {"GEqual", Stencil::Comparison::GEqual},
        {"Always", Stencil::Comparison::Always},
    };

    inline constexpr Entry<Stencil::Operation> kStencilOperation[] = {
        {"Keep", Stencil::Operation::Keep},
        {"Zero", Stencil::Operation::Zero},
        {"Replace", Stencil::Operation::Replace},
        {"IncrSat", Stencil::Operation::IncrSat},
        {"DecrSat", Stencil::Operation::DecrSat},
        {"Invert", Stencil::Operation::Invert},
        {"IncrWrap", Stencil::Operation::IncrWrap},
        {"DecrWrap", Stencil::Operation::DecrWrap},
    };

    inline constexpr Entry<ZClip::Enabled> kZClip[] = {
        {"False", ZClip::Enabled::False},
        {"True", ZClip::Enabled::True},
    };

    inline constexpr Entry<ZTest::Operation> kZTest[] = {
        {"Disabled", ZTest::Operation::Disabled},
        {"Never", ZTest::Operation::Never},
        {"Less", ZTest::Operation::Less},
        {"Equal", ZTest::Operation::Equal},
        {"LEqual", ZTest::Operation::LEqual},
        {"Greater", ZTest::Operation::Greater},
        {"NotEqual", ZTest::Operation::NotEqual},
        {"GEqual", ZTest::Operation::GEqual},
        {"Always", ZTest::Operation::Always},
    };

    inline constexpr Entry<ZWrite::State> kZWrite[] = {
        {"Off", ZWrite::State::Off},
        {"On", ZWrite::State::On},
    };

    inline constexpr Entry<Fog::Mode> kFogMode[] = {
        {"Off", Fog::Mode::Off},
        {"Global", Fog::Mode::Global},
        {"Linear", Fog::Mode::Linear},
        {"Exp", Fog::Mode::Exp},
        {"Exp2", Fog::Mode::Exp2},
    };

    inline constexpr Entry<CommandType> kCommandType[] = {
        {"AlphaToMask", CommandType::AlphaToMask},
        {"Blend", CommandType::Blend},
        {"BlendOp", CommandType::BlendOp},
        {"ColorMask", CommandType::ColorMask},
        {"Conservative", CommandType::Conservative},
        {"Cull", CommandType::Cull},
        {"Offset", CommandType::Offset},
        {"Stencil", CommandType::Stencil},
        {"ZClip", CommandType::ZClip},
        {"ZTest", CommandType::ZTest},
        {"ZWrite", CommandType::ZWrite},
        {"Fog", CommandType::Fog},
    };
}
