//
// Created by derpy on 2026/02/13.
//
#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace sl_parser
{
    /// <summary>
    /// A command argument that is either a literal or a reference to a material property, e.g. <c>Cull [_CullMode]</c>.
    /// </summary>
    /// <remarks>When <c>property</c> is set, <c>value</c> holds a default-constructed placeholder.</remarks>
    /// <typeparam name="T">Type of the literal value.</typeparam>
    template <typename T>
    struct Value
    {
        T value{};
        std::optional<std::string> property;

        Value() = default;

        Value(T v) : value(std::move(v)) // NOLINT(google-explicit-constructor)
        {
        }

        /// <summary>Creates a value that refers to a material property.</summary>
        /// <param name="name">Property name without brackets, e.g. <c>_CullMode</c>.</param>
        /// <returns>A value whose <c>property</c> is set.</returns>
        static Value FromProperty(std::string name)
        {
            Value result;
            result.property = std::move(name);
            return result;
        }

        /// <summary>Checks whether this value refers to a material property.</summary>
        /// <returns><c>true</c> if <c>property</c> is set.</returns>
        [[nodiscard]] bool IsProperty() const
        {
            return property.has_value();
        }

        operator const T&() const // NOLINT(google-explicit-constructor)
        {
            return value;
        }
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
        Fog,
    };

    struct Command
    {
        virtual ~Command() = default;
        /// <summary>Gets the kind of this command.</summary>
        /// <returns>The command type, which tells which derived struct this is.</returns>
        [[nodiscard]] virtual CommandType GetCommandType() const = 0;
        /// <summary>Creates a deep copy of this command.</summary>
        /// <returns>A new command of the same derived type.</returns>
        [[nodiscard]] virtual std::unique_ptr<Command> Clone() const = 0;
    };

    template <typename Derived, CommandType Type>
    struct CommandImpl : Command
    {
        static constexpr CommandType kType = Type;

        [[nodiscard]] CommandType GetCommandType() const override
        {
            return Type;
        }

        [[nodiscard]] std::unique_ptr<Command> Clone() const override
        {
            return std::make_unique<Derived>(static_cast<const Derived&>(*this));
        }
    };

    struct AlphaToMask : CommandImpl<AlphaToMask, CommandType::AlphaToMask>
    {
        enum class State
        {
            Off,
            On,
        };

        Value<State> state = State::Off;
    };

    struct Blend : CommandImpl<Blend, CommandType::Blend>
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
            SrcAlphaSaturate,
        };

        /// <summary>Render target index (0-7). Empty means all render targets.</summary>
        std::optional<uint8_t> target;
        /// <summary><c>false</c> for <c>Blend Off</c>.</summary>
        bool state = false;
        Value<Factor> src_factor = Factor::One;
        Value<Factor> dst_factor = Factor::Zero;
        /// <summary><c>true</c> when the alpha factors were given explicitly (<c>Blend A B, C D</c>).</summary>
        /// <remarks>Otherwise the alpha factors are equal to the color factors.</remarks>
        bool separate_alpha = false;
        Value<Factor> alpha_src_factor = Factor::One;
        Value<Factor> alpha_dst_factor = Factor::Zero;
    };

    struct BlendOp : CommandImpl<BlendOp, CommandType::BlendOp>
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

        /// <summary>Render target index (0-7). Empty means all render targets.</summary>
        std::optional<uint8_t> target;
        Value<Op> operation = Op::Add;
        /// <summary>Set for <c>BlendOp colorOp, alphaOp</c>.</summary>
        std::optional<Value<Op>> alpha_operation;
    };

    struct ColorMask : CommandImpl<ColorMask, CommandType::ColorMask>
    {
        /// <summary>Bit flags; any combination of R, G, B and A is valid.</summary>
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

        /// <summary>Render target index (0-7). Empty means all render targets.</summary>
        std::optional<uint8_t> target;
        Value<Channels> channels = Channels::RGBA;
    };

    struct Conservative : CommandImpl<Conservative, CommandType::Conservative>
    {
        enum class Enabled
        {
            False,
            True,
        };

        Value<Enabled> enabled = Enabled::False;
    };

    struct Cull : CommandImpl<Cull, CommandType::Cull>
    {
        enum class State
        {
            Off,
            Back,
            Front,
        };

        Value<State> state = State::Back;
    };

    struct Offset : CommandImpl<Offset, CommandType::Offset>
    {
        Value<float> factor = 0.0f;
        Value<float> units = 0.0f;
    };

    struct Stencil : CommandImpl<Stencil, CommandType::Stencil>
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

        Value<uint8_t> ref = static_cast<uint8_t>(0);
        Value<uint8_t> read_mask = static_cast<uint8_t>(0xFF);
        Value<uint8_t> write_mask = static_cast<uint8_t>(0xFF);

        Value<Comparison> comparison_operation = Comparison::Always;
        Value<Operation> pass_operation = Operation::Keep;
        Value<Operation> fail_operation = Operation::Keep;
        Value<Operation> z_fail_operation = Operation::Keep;
        Value<Comparison> comparison_operation_back = Comparison::Always;
        Value<Operation> pass_operation_back = Operation::Keep;
        Value<Operation> fail_operation_back = Operation::Keep;
        Value<Operation> z_fail_operation_back = Operation::Keep;
        Value<Comparison> comparison_operation_front = Comparison::Always;
        Value<Operation> pass_operation_front = Operation::Keep;
        Value<Operation> fail_operation_front = Operation::Keep;
        Value<Operation> z_fail_operation_front = Operation::Keep;
    };

    struct ZClip : CommandImpl<ZClip, CommandType::ZClip>
    {
        enum class Enabled
        {
            False,
            True,
        };

        Value<Enabled> enabled = Enabled::True;
    };

    struct ZTest : CommandImpl<ZTest, CommandType::ZTest>
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

        Value<Operation> operation = Operation::LEqual;
    };

    struct ZWrite : CommandImpl<ZWrite, CommandType::ZWrite>
    {
        enum class State
        {
            Off,
            On,
        };

        Value<State> state = State::On;
    };

    /// <summary>Legacy fixed-function fog block: <c>Fog { Mode Off }</c>, <c>Fog { Color (1,1,1,1) }</c>, ...</summary>
    struct Fog : CommandImpl<Fog, CommandType::Fog>
    {
        enum class Mode
        {
            Off,
            Global,
            Linear,
            Exp,
            Exp2,
        };

        std::optional<Value<Mode>> mode;
        std::optional<Value<std::array<float, 4>>> color;
        std::optional<Value<float>> density;
        std::optional<std::pair<Value<float>, Value<float>>> range;
    };
}
