//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <string>

namespace sl_parser
{
    enum class RenderTarget
    {
        None,
        One,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
    };

    enum class CommandType
    {
        AlphaToMask,
        Blend,
        BlendOp,
        ColorMask,
        Conservative,
        Cull,
        Offset,
        Stencil,
        ZClip,
        ZTest,
        ZWrite,
    };

    struct Command
    {
        virtual ~Command() = default;
        virtual CommandType GetCommandType() = 0;
    };

    struct AlphaToMask : Command
    {
        enum class State
        {
            Off,
            On,
        };

        State state = State::Off;

        CommandType GetCommandType() override
        {
            return CommandType::AlphaToMask;
        };
    };

    struct Blend : Command
    {
        enum class Factor
        {
            One,
            Zero,
            SrcColor,
            SrcAlpha,
            DstColor,
            DstAlpha,
            OneMinusSrcColor,
            OneMinusSrcAlpha,
            OneMinusDstColor,
            OneMinusDstAlpha,
        };

        RenderTarget target = RenderTarget::None;
        bool state = false;
        Factor src_factor = Factor::One;
        Factor dst_factor = Factor::One;
        Factor alpha_src_factor = Factor::One;
        Factor alpha_dst_factor = Factor::One;

        CommandType GetCommandType() override
        {
            return CommandType::Blend;
        };
    };

    struct BlendOp : Command
    {
        enum class Op
        {
            Add,
            Sub,
            RevSub,
            Min,
            Max,
            LogicalClear,
            LogicalSet,
            LogicalCopy,
            LogicalCopyInverted,
            LogicalNoop,
            LogicalInvert,
            LogicalAnd,
            LogicalNand,
            LogicalOr,
            LogicalNor,
            LogicalXor,
            LogicalEquiv,
            LogicalAndReverse,
            LogicalAndInverted,
            LogicalOrReverse,
            LogicalOrInverted,
            Multiply,
            Screen,
            Overlay,
            Darken,
            Lighten,
            ColorDodge,
            ColorBurn,
            HardLight,
            SoftLight,
            Difference,
            Exclusion,
            HSLHue,
            HSLSaturation,
            HSLColor,
            HSLLuminosity,
        };

        Op operation;

        CommandType GetCommandType() override
        {
            return CommandType::BlendOp;
        };
    };

    struct ColorMask : Command
    {
        enum class Channels : uint8_t
        {
            Zero = 0,
            R = 1,
            G = 2,
            B = 4,
            A = 8,
            RGB = R | G | B,
            RGBA = R | G | B | A,
        };

        RenderTarget target;
        Channels channels;

        CommandType GetCommandType() override
        {
            return CommandType::ColorMask;
        };
    };

    struct Conservative : Command
    {
        enum class Enabled
        {
            False,
            True,
        };

        Enabled enabled = Enabled::False;

        CommandType GetCommandType() override
        {
            return CommandType::Conservative;
        };
    };

    struct Cull : Command
    {
        enum class State
        {
            Off,
            Back,
            Front,
        };

        State state = State::Back;

        CommandType GetCommandType() override
        {
            return CommandType::Cull;
        };
    };

    struct Offset : Command
    {
        float factor;
        float units;

        CommandType GetCommandType() override
        {
            return CommandType::Offset;
        };
    };

    struct Stencil : Command
    {
        enum class Comparison
        {
            Never,
            Less,
            Equal,
            LEqual,
            Greater,
            NotEqual,
            GEqual,
            Always,
        };

        enum class Operation
        {
            Keep,
            Zero,
            Replace,
            IncrSat,
            DecrSat,
            Invert,
            IncrWrap,
            DecrWrap,
        };

        char ref = 0;
        char read_mask = static_cast<char>(0xFF);
        char write_mask = static_cast<char>(0xFF);

        Comparison comparison_operation = Comparison::Always;
        Operation pass_operation = Operation::Keep;
        Operation fail_operation = Operation::Keep;
        Operation z_fail_operation = Operation::Keep;
        Comparison comparison_operation_back = Comparison::Always;
        Operation pass_operation_back = Operation::Keep;
        Operation fail_operation_back = Operation::Keep;
        Operation z_fail_operation_back = Operation::Keep;
        Comparison comparison_operation_front = Comparison::Always;
        Operation pass_operation_front = Operation::Keep;
        Operation fail_operation_front = Operation::Keep;
        Operation z_fail_operation_front = Operation::Keep;

        CommandType GetCommandType() override
        {
            return CommandType::Stencil;
        };
    };

    struct ZClip : Command
    {
        enum class Enabled
        {
            False,
            True,
        };

        Enabled enabled = Enabled::True;

        CommandType GetCommandType() override
        {
            return CommandType::ZClip;
        };
    };

    struct ZTest : Command
    {
        enum class Operation
        {
            Disabled,
            Never,
            Less,
            Equal,
            LEqual,
            Greater,
            NotEqual,
            GEqual,
            Always,
        };

        Operation operation = Operation::LEqual;

        CommandType GetCommandType() override
        {
            return CommandType::ZTest;
        };
    };

    struct ZWrite : Command
    {
        enum class State
        {
            Off,
            On,
        };

        State state = State::On;

        CommandType GetCommandType() override
        {
            return CommandType::ZWrite;
        };
    };
}
