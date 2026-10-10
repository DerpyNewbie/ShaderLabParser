//
// Created by derpy on 2026/02/13.
//

#include <functional>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>

#include "EnumTables.h"
#include "Lexer.h"
#include "Parser.h"
#include "StringUtil.h"
#include "Token.h"

namespace
{
    using namespace sl_parser;

    bool IsWord(const Token& token)
    {
        return token.type == TokenType::kIdentifier || token.type == TokenType::kKeyword;
    }

    bool IsProgramBegin(const Token& token)
    {
        return token.type == TokenType::kKeyword &&
            (token.value == "CGPROGRAM" || token.value == "HLSLPROGRAM" || token.value == "GLSLPROGRAM");
    }

    bool IsIncludeBegin(const Token& token)
    {
        return token.type == TokenType::kKeyword &&
            (token.value == "CGINCLUDE" || token.value == "HLSLINCLUDE" || token.value == "GLSLINCLUDE");
    }

    bool IsCommand(const Token& token)
    {
        return IsWord(token) && tables::Find<CommandType>(tables::kCommandType, token.value).has_value();
    }

    std::string Describe(const Token& token)
    {
        switch (token.type)
        {
        case TokenType::kEndOfFile: return "end of file";
        case TokenType::kStringLiteral: return "\"" + token.value + "\"";
        case TokenType::kProgram: return "program block";
        default: return "'" + token.value + "'";
        }
    }

    /// <summary>Parses a float independently of the current locale.</summary>
    /// <param name="text">Number text; an HLSL-style <c>f</c> suffix is allowed.</param>
    /// <param name="out">Receives the value on success.</param>
    /// <returns><c>false</c> if <paramref name="text"/> is not entirely a number.</returns>
    bool TryParseFloat(std::string text, float* out)
    {
        if (!text.empty() && (text.back() == 'f' || text.back() == 'F')) text.pop_back();
        std::istringstream stream(text);
        stream.imbue(std::locale::classic());
        float value;
        stream >> value;
        if (stream.fail() || !stream.eof()) return false;
        *out = value;
        return true;
    }

    class ParserImpl
    {
    public:
        ParserImpl(const Tokens& tokens, std::vector<ParseError>* errors) : errors_(errors)
        {
            for (const auto& token : tokens)
            {
                if (token.type == TokenType::kNewLine || token.type == TokenType::kComment) continue;
                if (token.type == TokenType::kEndOfFile) break;
                tokens_.push_back(&token);
            }

            const uint32_t eof_line = tokens.empty() ? 1 : tokens.back().line;
            eof_ = Token{TokenType::kEndOfFile, "", eof_line, 0};
            tokens_.push_back(&eof_);
        }

        ShaderLabObject ParseShader()
        {
            ShaderLabObject shader;
            try
            {
                ExpectWord("Shader");
                shader.name = ExpectString("shader name");
                Expect(TokenType::kBlockBegin, "'{'");
                ParseItems("Shader", [&] { ParseShaderItem(shader); });
                Expect(TokenType::kBlockEnd, "'}'");
                if (!IsEnd())
                {
                    Fail(Peek(), "unexpected " + Describe(Peek()) + " after the end of the Shader block.");
                }
            }
            catch (const ParseError& e)
            {
                if (errors_ == nullptr) throw;
                errors_->push_back(e);
            }
            return shader;
        }

    private:
        std::vector<const Token*> tokens_;
        Token eof_;
        size_t pos_ = 0;
        std::vector<ParseError>* errors_;

        // ------------------------------------------------------------------ cursor

        [[nodiscard]] const Token& Peek(const size_t ahead = 0) const
        {
            return *tokens_[std::min(pos_ + ahead, tokens_.size() - 1)];
        }

        const Token& Next()
        {
            const Token& token = Peek();
            if (!IsEnd()) ++pos_;
            return token;
        }

        [[nodiscard]] bool IsEnd() const
        {
            return Peek().type == TokenType::kEndOfFile;
        }

        [[nodiscard]] bool PeekType(const TokenType type) const
        {
            return Peek().type == type;
        }

        [[nodiscard]] bool PeekWord(const std::string_view word) const
        {
            return IsWord(Peek()) && EqualsIgnoreCase(Peek().value, word);
        }

        bool Accept(const TokenType type)
        {
            if (!PeekType(type)) return false;
            Next();
            return true;
        }

        [[noreturn]] static void Fail(const Token& token, const std::string& message)
        {
            throw ParseError(token.line, token.pos + 1, message);
        }

        const Token& Expect(const TokenType type, const std::string& what)
        {
            if (!PeekType(type))
            {
                Fail(Peek(), "expected " + what + ", got " + Describe(Peek()) + ".");
            }
            return Next();
        }

        void ExpectWord(const std::string_view word)
        {
            if (!PeekWord(word))
            {
                Fail(Peek(), "expected " + std::string(word) + ", got " + Describe(Peek()) + ".");
            }
            Next();
        }

        const Token& ExpectIdentifier(const std::string& what)
        {
            if (!IsWord(Peek()))
            {
                Fail(Peek(), "expected " + what + ", got " + Describe(Peek()) + ".");
            }
            return Next();
        }

        std::string ExpectString(const std::string& what)
        {
            return Expect(TokenType::kStringLiteral, what + " string").value;
        }

        void ExpectOperator(const std::string_view op)
        {
            if (!PeekType(TokenType::kOperator) || Peek().value != op)
            {
                Fail(Peek(), "expected '" + std::string(op) + "', got " + Describe(Peek()) + ".");
            }
            Next();
        }

        /// <summary>Rebuilds the source text of a token range.</summary>
        /// <remarks>Whitespace between tokens is collapsed to a single space.</remarks>
        /// <param name="from">Index of the first token.</param>
        /// <param name="to">Index one past the last token.</param>
        /// <returns>The text of tokens [from, to).</returns>
        [[nodiscard]] std::string RawText(const size_t from, const size_t to) const
        {
            std::string result;
            for (size_t k = from; k < to && k < tokens_.size(); ++k)
            {
                const Token& token = *tokens_[k];
                if (k > from)
                {
                    const Token& previous = *tokens_[k - 1];
                    if (token.offset > previous.offset + previous.length) result += ' ';
                }
                result += token.type == TokenType::kStringLiteral ? "\"" + token.value + "\"" : token.value;
            }
            return result;
        }

        /// <summary>Skips a balanced <c>{ ... }</c> block starting at the current <c>{</c>.</summary>
        void SkipBlock()
        {
            Expect(TokenType::kBlockBegin, "'{'");
            for (int depth = 1; depth > 0;)
            {
                if (IsEnd()) Fail(Peek(), "expected '}', got end of file.");
                const Token& token = Next();
                if (token.type == TokenType::kBlockBegin) ++depth;
                if (token.type == TokenType::kBlockEnd) --depth;
            }
        }

        // ------------------------------------------------------------------ error recovery

        static int Depth(const Token& token)
        {
            switch (token.type)
            {
            case TokenType::kBlockBegin:
            case TokenType::kBracketBegin:
                return 1;
            case TokenType::kBlockEnd:
            case TokenType::kBracketEnd:
                return -1;
            default:
                return 0;
            }
        }

        /// <summary>Parses block items until <c>}</c> or end of file.</summary>
        /// <remarks>In TryParse mode, an item that fails is recorded and skipped.</remarks>
        /// <param name="block">Block name used in error messages.</param>
        /// <param name="parse_item">Parses one item at the current position.</param>
        /// <param name="is_item_start">Optional check for where the next item starts after an error.</param>
        void ParseItems(const std::string_view block, const std::function<void()>& parse_item,
                        const std::function<bool(size_t, uint32_t)>& is_item_start = nullptr)
        {
            while (!PeekType(TokenType::kBlockEnd) && !IsEnd())
            {
                const size_t item_start = pos_;
                try
                {
                    parse_item();
                }
                catch (const ParseError& e)
                {
                    if (errors_ == nullptr) throw;
                    errors_->push_back(e);
                    Resync(item_start, e.line, is_item_start);
                }
            }

            if (IsEnd())
            {
                Fail(Peek(), "expected '}' to close " + std::string(block) + ", got end of file.");
            }
        }

        /// <summary>Skips past the rest of a broken item.</summary>
        /// <remarks>Stops at the next item start or at the <c>}</c> closing the enclosing block.</remarks>
        /// <param name="item_start">Index of the broken item's first token.</param>
        /// <param name="error_line">Line of the error.</param>
        /// <param name="is_item_start">Optional item start check; defaults to any keyword.</param>
        void Resync(const size_t item_start, const uint32_t error_line,
                    const std::function<bool(size_t, uint32_t)>& is_item_start)
        {
            int depth = 0;
            for (size_t k = item_start; k < pos_; ++k) depth += Depth(*tokens_[k]);

            if (pos_ == item_start && !IsEnd())
            {
                depth += Depth(Next());
            }

            while (!IsEnd())
            {
                const Token& token = Peek();
                if (depth <= 0)
                {
                    if (token.type == TokenType::kBlockEnd) return;
                    const bool starts_item = is_item_start
                        ? is_item_start(pos_, error_line)
                        : token.type == TokenType::kKeyword && !token.value.starts_with("END");
                    if (starts_item) return;
                }
                depth += Depth(Next());
            }
        }

        // ------------------------------------------------------------------ values

        float ParseNumber(const std::string& what)
        {
            const Token& token = Peek();
            float value;
            if (token.type != TokenType::kNumberLiteral || !TryParseFloat(token.value, &value))
            {
                Fail(token, "expected " + what + " number, got " + Describe(token) + ".");
            }
            Next();
            return value;
        }

        int ParseInteger(const std::string& what, const int min, const int max)
        {
            const Token& token = Peek();
            float value;
            if (token.type != TokenType::kNumberLiteral || !TryParseFloat(token.value, &value) ||
                value != static_cast<float>(static_cast<int>(value)))
            {
                Fail(token, "expected " + what + " integer, got " + Describe(token) + ".");
            }
            if (value < static_cast<float>(min) || value > static_cast<float>(max))
            {
                Fail(token, what + " must be between " + std::to_string(min) + " and " + std::to_string(max) +
                     ", got " + token.value + ".");
            }
            Next();
            return static_cast<int>(value);
        }

        std::optional<std::string> TryPropertyReference()
        {
            if (!PeekType(TokenType::kAttributeBegin)) return std::nullopt;
            Next();
            std::string name = ExpectIdentifier("property name").value;
            Expect(TokenType::kAttributeEnd, "']'");
            return name;
        }

        template <typename E, size_t N>
        Value<E> ParseEnum(const tables::Entry<E> (&table)[N], const std::string& what)
        {
            if (auto property = TryPropertyReference()) return Value<E>::FromProperty(std::move(*property));

            const Token& token = Peek();
            if (IsWord(token) || token.type == TokenType::kNumberLiteral)
            {
                if (const auto value = tables::Find<E>(table, token.value))
                {
                    Next();
                    return *value;
                }
            }

            Fail(token, "expected " + what + " (" + tables::Names<E>(table) + " or [property]), got " +
                 Describe(token) + ".");
        }

        Value<float> ParseFloatValue(const std::string& what)
        {
            if (auto property = TryPropertyReference()) return Value<float>::FromProperty(std::move(*property));
            return ParseNumber(what);
        }

        Value<uint8_t> ParseByteValue(const std::string& what)
        {
            if (auto property = TryPropertyReference()) return Value<uint8_t>::FromProperty(std::move(*property));
            return static_cast<uint8_t>(ParseInteger(what, 0, 255));
        }

        std::vector<float> ParseTuple(const std::string& what)
        {
            Expect(TokenType::kBracketBegin, "'(' to start " + what);
            std::vector<float> values;
            do
            {
                values.push_back(ParseNumber(what + " component"));
            }
            while (Accept(TokenType::kComma));
            Expect(TokenType::kBracketEnd, "')' to close " + what);
            return values;
        }

        std::optional<uint8_t> TryRenderTarget()
        {
            if (!PeekType(TokenType::kNumberLiteral)) return std::nullopt;
            return static_cast<uint8_t>(ParseInteger("render target", 0, 7));
        }

        // ------------------------------------------------------------------ blocks

        void ParseTags(std::optional<Tags>* tags)
        {
            Next(); // Tags
            Expect(TokenType::kBlockBegin, "'{' after Tags");
            if (!tags->has_value()) tags->emplace();
            while (!Accept(TokenType::kBlockEnd))
            {
                std::string key = ExpectString("tag name");
                ExpectOperator("=");
                std::string value = ExpectString("tag value");
                (*tags)->insert_or_assign(std::move(key), std::move(value));
            }
        }

        void ParsePackageRequirements(std::optional<PackageRequirements>* requirements)
        {
            Next(); // PackageRequirements
            Expect(TokenType::kBlockBegin, "'{' after PackageRequirements");
            if (!requirements->has_value()) requirements->emplace();
            while (!Accept(TokenType::kBlockEnd))
            {
                std::string package = ExpectString("package name");
                std::string version;
                if (PeekType(TokenType::kOperator) && Peek().value == ":")
                {
                    Next();
                    version = ExpectString("package version");
                }
                (*requirements)->insert_or_assign(std::move(package), std::move(version));
            }
        }

        ShaderProgram ParseProgram()
        {
            const Token& begin = Next();
            ShaderProgram program;
            program.line = begin.line;
            if (begin.value.starts_with("HLSL")) program.language = ShaderProgram::Language::HLSL;
            else if (begin.value.starts_with("GLSL")) program.language = ShaderProgram::Language::GLSL;
            else program.language = ShaderProgram::Language::CG;

            program.program = Expect(TokenType::kProgram, "program source").value;
            Expect(TokenType::kKeyword, "END keyword"); // the Lexer guarantees the matching END keyword
            return program;
        }

        // ------------------------------------------------------------------ commands

        void ParseCommand(std::optional<Commands>* commands)
        {
            const Token& keyword = Next();
            const CommandType type = *tables::Find<CommandType>(tables::kCommandType, keyword.value);
            std::unique_ptr<Command> command;

            switch (type)
            {
            case CommandType::AlphaToMask:
                {
                    auto c = std::make_unique<AlphaToMask>();
                    c->state = ParseEnum(tables::kAlphaToMask, "AlphaToMask state");
                    command = std::move(c);
                    break;
                }
            case CommandType::Blend:
                {
                    auto c = std::make_unique<Blend>();
                    c->target = TryRenderTarget();
                    if (PeekWord("Off"))
                    {
                        Next();
                        c->state = false;
                    }
                    else
                    {
                        c->state = true;
                        c->src_factor = ParseEnum(tables::kBlendFactor, "source blend factor");
                        c->dst_factor = ParseEnum(tables::kBlendFactor, "destination blend factor");
                        if (Accept(TokenType::kComma))
                        {
                            c->separate_alpha = true;
                            c->alpha_src_factor = ParseEnum(tables::kBlendFactor, "source alpha blend factor");
                            c->alpha_dst_factor = ParseEnum(tables::kBlendFactor, "destination alpha blend factor");
                        }
                        else
                        {
                            c->alpha_src_factor = c->src_factor;
                            c->alpha_dst_factor = c->dst_factor;
                        }
                    }
                    command = std::move(c);
                    break;
                }
            case CommandType::BlendOp:
                {
                    auto c = std::make_unique<BlendOp>();
                    c->target = TryRenderTarget();
                    c->operation = ParseEnum(tables::kBlendOp, "blend operation");
                    if (Accept(TokenType::kComma))
                    {
                        c->alpha_operation = ParseEnum(tables::kBlendOp, "alpha blend operation");
                    }
                    command = std::move(c);
                    break;
                }
            case CommandType::ColorMask:
                {
                    auto c = std::make_unique<ColorMask>();
                    c->channels = ParseColorMaskChannels();
                    c->target = TryRenderTarget();
                    command = std::move(c);
                    break;
                }
            case CommandType::Conservative:
                {
                    auto c = std::make_unique<Conservative>();
                    c->enabled = ParseEnum(tables::kConservative, "Conservative value");
                    command = std::move(c);
                    break;
                }
            case CommandType::Cull:
                {
                    auto c = std::make_unique<Cull>();
                    c->state = ParseEnum(tables::kCull, "cull mode");
                    command = std::move(c);
                    break;
                }
            case CommandType::Offset:
                {
                    auto c = std::make_unique<Offset>();
                    c->factor = ParseFloatValue("Offset factor");
                    Expect(TokenType::kComma, "',' between Offset factor and units");
                    c->units = ParseFloatValue("Offset units");
                    command = std::move(c);
                    break;
                }
            case CommandType::Stencil:
                command = ParseStencil();
                break;
            case CommandType::ZClip:
                {
                    auto c = std::make_unique<ZClip>();
                    c->enabled = ParseEnum(tables::kZClip, "ZClip value");
                    command = std::move(c);
                    break;
                }
            case CommandType::ZTest:
                {
                    auto c = std::make_unique<ZTest>();
                    c->operation = ParseEnum(tables::kZTest, "ZTest comparison");
                    command = std::move(c);
                    break;
                }
            case CommandType::ZWrite:
                {
                    auto c = std::make_unique<ZWrite>();
                    c->state = ParseEnum(tables::kZWrite, "ZWrite state");
                    command = std::move(c);
                    break;
                }
            case CommandType::Fog:
                command = ParseFog();
                break;
            }

            if (!commands->has_value()) commands->emplace();
            (*commands)->push_back(std::move(command));
        }

        Value<ColorMask::Channels> ParseColorMaskChannels()
        {
            if (auto property = TryPropertyReference())
            {
                return Value<ColorMask::Channels>::FromProperty(std::move(*property));
            }

            const Token& token = Peek();
            if (token.type == TokenType::kNumberLiteral && token.value == "0")
            {
                Next();
                return ColorMask::Channels::Zero;
            }

            uint8_t mask = 0;
            bool valid = token.type == TokenType::kIdentifier && !token.value.empty();
            for (const char c : token.value)
            {
                if (!valid) break;
                switch (std::toupper(static_cast<unsigned char>(c)))
                {
                case 'R': mask |= static_cast<uint8_t>(ColorMask::Channels::R);
                    break;
                case 'G': mask |= static_cast<uint8_t>(ColorMask::Channels::G);
                    break;
                case 'B': mask |= static_cast<uint8_t>(ColorMask::Channels::B);
                    break;
                case 'A': mask |= static_cast<uint8_t>(ColorMask::Channels::A);
                    break;
                default: valid = false;
                }
            }

            if (!valid)
            {
                Fail(token, "expected color mask (0, or a combination of R, G, B, A, or [property]), got " +
                     Describe(token) + ".");
            }

            Next();
            return static_cast<ColorMask::Channels>(mask);
        }

        std::unique_ptr<Command> ParseStencil()
        {
            auto stencil = std::make_unique<Stencil>();
            Expect(TokenType::kBlockBegin, "'{' after Stencil");
            while (!Accept(TokenType::kBlockEnd))
            {
                const Token& key = Peek();
                if (!IsWord(key))
                {
                    Fail(key, "expected stencil operation name or '}', got " + Describe(key) + ".");
                }

                const std::string& k = key.value;
                Next();
                auto is = [&](const std::string_view name) { return EqualsIgnoreCase(k, name); };
                auto cmp = [&] { return ParseEnum(tables::kStencilComparison, "stencil comparison"); };
                auto op = [&] { return ParseEnum(tables::kStencilOperation, "stencil operation"); };

                if (is("Ref")) stencil->ref = ParseByteValue("stencil Ref");
                else if (is("ReadMask")) stencil->read_mask = ParseByteValue("stencil ReadMask");
                else if (is("WriteMask")) stencil->write_mask = ParseByteValue("stencil WriteMask");
                else if (is("Comp")) stencil->comparison_operation = cmp();
                else if (is("Pass")) stencil->pass_operation = op();
                else if (is("Fail")) stencil->fail_operation = op();
                else if (is("ZFail")) stencil->z_fail_operation = op();
                else if (is("CompBack")) stencil->comparison_operation_back = cmp();
                else if (is("PassBack")) stencil->pass_operation_back = op();
                else if (is("FailBack")) stencil->fail_operation_back = op();
                else if (is("ZFailBack")) stencil->z_fail_operation_back = op();
                else if (is("CompFront")) stencil->comparison_operation_front = cmp();
                else if (is("PassFront")) stencil->pass_operation_front = op();
                else if (is("FailFront")) stencil->fail_operation_front = op();
                else if (is("ZFailFront")) stencil->z_fail_operation_front = op();
                else
                {
                    Fail(key, "unknown stencil operation '" + k +
                         "', expected Ref, ReadMask, WriteMask, Comp, Pass, Fail, ZFail "
                         "(or the Back/Front variants).");
                }
            }
            return stencil;
        }

        std::unique_ptr<Command> ParseFog()
        {
            auto fog = std::make_unique<Fog>();
            Expect(TokenType::kBlockBegin, "'{' after Fog");
            while (!Accept(TokenType::kBlockEnd))
            {
                const Token& key = Peek();
                if (PeekWord("Mode"))
                {
                    Next();
                    fog->mode = ParseEnum(tables::kFogMode, "fog mode");
                }
                else if (PeekWord("Color"))
                {
                    Next();
                    if (auto property = TryPropertyReference())
                    {
                        fog->color = Value<std::array<float, 4>>::FromProperty(std::move(*property));
                        continue;
                    }
                    const Token& at = Peek();
                    const auto values = ParseTuple("fog color");
                    if (values.size() != 4) Fail(at, "fog color must have 4 components.");
                    fog->color = std::array<float, 4>{values[0], values[1], values[2], values[3]};
                }
                else if (PeekWord("Density"))
                {
                    Next();
                    fog->density = ParseFloatValue("fog density");
                }
                else if (PeekWord("Range"))
                {
                    Next();
                    auto range_near = ParseFloatValue("fog range near");
                    Expect(TokenType::kComma, "',' in fog Range");
                    auto range_far = ParseFloatValue("fog range far");
                    fog->range = std::make_pair(range_near, range_far);
                }
                else
                {
                    Fail(key, "expected Mode, Color, Density, Range or '}' in Fog, got " + Describe(key) + ".");
                }
            }
            return fog;
        }

        // ------------------------------------------------------------------ passes

        std::unique_ptr<Pass> ParsePass()
        {
            Next(); // Pass
            auto pass = std::make_unique<Pass>();
            Expect(TokenType::kBlockBegin, "'{' after Pass");
            ParseItems("Pass", [&]
            {
                const Token& token = Peek();
                if (PeekWord("Name"))
                {
                    Next();
                    pass->name = ExpectString("pass name");
                }
                else if (PeekWord("Tags")) ParseTags(&pass->tags);
                else if (PeekWord("PackageRequirements")) ParsePackageRequirements(&pass->package_requirements);
                else if (IsCommand(token)) ParseCommand(&pass->commands);
                else if (IsIncludeBegin(token)) pass->includes.push_back(ParseProgram());
                else if (IsProgramBegin(token))
                {
                    if (pass->shader_program.has_value())
                    {
                        Fail(token, "a Pass can only contain one shader program.");
                    }
                    pass->shader_program = ParseProgram();
                }
                else
                {
                    Fail(token, "unexpected " + Describe(token) + " in Pass, expected Name, Tags, "
                         "PackageRequirements, a render state command, or a shader program.");
                }
            });
            Expect(TokenType::kBlockEnd, "'}'");
            return pass;
        }

        std::unique_ptr<GrabPass> ParseGrabPass()
        {
            Next(); // GrabPass
            auto grab_pass = std::make_unique<GrabPass>();
            Expect(TokenType::kBlockBegin, "'{' after GrabPass");
            ParseItems("GrabPass", [&]
            {
                const Token& token = Peek();
                if (token.type == TokenType::kStringLiteral) grab_pass->target_texture = Next().value;
                else if (PeekWord("Name"))
                {
                    Next();
                    grab_pass->name = ExpectString("pass name");
                }
                else if (PeekWord("Tags")) ParseTags(&grab_pass->tags);
                else
                {
                    Fail(token, "unexpected " + Describe(token) +
                         " in GrabPass, expected a texture name, Name or Tags.");
                }
            });
            Expect(TokenType::kBlockEnd, "'}'");
            return grab_pass;
        }

        // ------------------------------------------------------------------ sub shaders

        SubShader ParseSubShader()
        {
            Next(); // SubShader
            SubShader sub_shader;
            uint8_t pass_order = 0;
            Expect(TokenType::kBlockBegin, "'{' after SubShader");
            ParseItems("SubShader", [&]
            {
                const Token& token = Peek();
                if (PeekWord("Tags")) ParseTags(&sub_shader.tags);
                else if (PeekWord("LOD"))
                {
                    Next();
                    sub_shader.lod = ParseInteger("LOD", 0, std::numeric_limits<int>::max());
                }
                else if (PeekWord("PackageRequirements"))
                {
                    ParsePackageRequirements(&sub_shader.package_requirements);
                }
                else if (PeekWord("Pass"))
                {
                    auto pass = ParsePass();
                    pass->order = pass_order++;
                    sub_shader.passes.push_back(std::move(pass));
                }
                else if (PeekWord("GrabPass"))
                {
                    auto grab_pass = ParseGrabPass();
                    grab_pass->order = pass_order++;
                    sub_shader.passes.push_back(std::move(grab_pass));
                }
                else if (PeekWord("UsePass"))
                {
                    Next();
                    auto use_pass = std::make_unique<UsePass>();
                    use_pass->target_pass = ExpectString("UsePass shader/pass name");
                    use_pass->order = pass_order++;
                    sub_shader.passes.push_back(std::move(use_pass));
                }
                else if (IsCommand(token)) ParseCommand(&sub_shader.commands);
                else if (IsIncludeBegin(token)) sub_shader.includes.push_back(ParseProgram());
                else if (IsProgramBegin(token)) sub_shader.shader_programs.push_back(ParseProgram());
                else
                {
                    Fail(token, "unexpected " + Describe(token) + " in SubShader, expected Tags, LOD, Pass, "
                         "GrabPass, UsePass, PackageRequirements, a render state command, or a shader program.");
                }
            });
            Expect(TokenType::kBlockEnd, "'}'");
            return sub_shader;
        }

        /// <summary>Parses a Category block, which groups sub shaders and shares its render state, tags and includes.</summary>
        /// <remarks>The shared values are copied into each sub shader; the sub shader's own values take precedence.</remarks>
        /// <param name="shader">Shader that receives the sub shaders.</param>
        void ParseCategory(ShaderLabObject& shader)
        {
            Next(); // Category
            const size_t first = shader.sub_shaders.size();
            std::optional<Commands> commands;
            std::optional<Tags> tags;
            std::optional<int> lod;
            ShaderPrograms includes;

            Expect(TokenType::kBlockBegin, "'{' after Category");
            ParseItems("Category", [&]
            {
                const Token& token = Peek();
                if (PeekWord("SubShader")) shader.sub_shaders.push_back(ParseSubShader());
                else if (PeekWord("Category")) ParseCategory(shader);
                else if (PeekWord("Tags")) ParseTags(&tags);
                else if (PeekWord("LOD"))
                {
                    Next();
                    lod = ParseInteger("LOD", 0, std::numeric_limits<int>::max());
                }
                else if (IsCommand(token)) ParseCommand(&commands);
                else if (IsIncludeBegin(token)) includes.push_back(ParseProgram());
                else
                {
                    Fail(token, "unexpected " + Describe(token) + " in Category, expected SubShader, Tags, LOD, "
                         "a render state command, or an include block.");
                }
            });
            Expect(TokenType::kBlockEnd, "'}'");

            for (size_t i = first; i < shader.sub_shaders.size(); ++i)
            {
                SubShader& sub_shader = shader.sub_shaders[i];
                if (commands.has_value())
                {
                    Commands merged;
                    for (const auto& command : *commands) merged.push_back(command->Clone());
                    if (sub_shader.commands.has_value())
                    {
                        for (auto& command : *sub_shader.commands) merged.push_back(std::move(command));
                    }
                    sub_shader.commands = std::move(merged);
                }
                if (tags.has_value())
                {
                    if (!sub_shader.tags.has_value()) sub_shader.tags.emplace();
                    sub_shader.tags->insert(tags->begin(), tags->end()); // existing keys are kept
                }
                if (lod.has_value() && !sub_shader.lod.has_value()) sub_shader.lod = lod;
                sub_shader.includes.insert(sub_shader.includes.begin(), includes.begin(), includes.end());
            }
        }

        // ------------------------------------------------------------------ properties

        PropertyDecl ParsePropertyDecl()
        {
            PropertyDecl decl;

            while (Accept(TokenType::kAttributeBegin))
            {
                if (!decl.attributes.has_value()) decl.attributes.emplace();
                do
                {
                    PropertyAttribute attribute;
                    attribute.name = ExpectIdentifier("attribute name").value;
                    if (PeekType(TokenType::kBracketBegin))
                    {
                        const Token& open = Next();
                        const size_t args_start = pos_;
                        for (int depth = 1; depth > 0;)
                        {
                            if (IsEnd()) Fail(open, "unterminated attribute arguments.");
                            const Token& token = Next();
                            if (token.type == TokenType::kBracketBegin) ++depth;
                            if (token.type == TokenType::kBracketEnd) --depth;
                        }
                        attribute.arguments = RawText(args_start, pos_ - 1);
                    }
                    decl.attributes->push_back(std::move(attribute));
                }
                while (Accept(TokenType::kComma));
                Expect(TokenType::kAttributeEnd, "']' to close attribute");
            }

            decl.name = ExpectIdentifier("property name").value;
            Expect(TokenType::kBracketBegin, "'(' after property name");
            decl.display_name = ExpectString("property display name");
            Expect(TokenType::kComma, "',' after property display name");

            const size_t type_start = pos_;
            const Token& type = Peek();
            if (!IsWord(type)) Fail(type, "expected property type, got " + Describe(type) + ".");
            auto is = [&](const std::string_view name) { return EqualsIgnoreCase(type.value, name); };
            if (is("Float")) decl.property_type = PropertyDecl::Type::Float;
            else if (is("Int") || is("Integer")) decl.property_type = PropertyDecl::Type::Integer;
            else if (is("Range")) decl.property_type = PropertyDecl::Type::Range;
            else if (is("Color")) decl.property_type = PropertyDecl::Type::Color;
            else if (is("Vector")) decl.property_type = PropertyDecl::Type::Vector;
            else if (is("2D")) decl.property_type = PropertyDecl::Type::Texture2D;
            else if (is("2DArray")) decl.property_type = PropertyDecl::Type::Texture2DArray;
            else if (is("3D")) decl.property_type = PropertyDecl::Type::Texture3D;
            else if (is("Cube")) decl.property_type = PropertyDecl::Type::Cubemap;
            else if (is("CubeArray")) decl.property_type = PropertyDecl::Type::CubemapArray;
            else if (is("Any")) decl.property_type = PropertyDecl::Type::TextureAny;
            else
            {
                Fail(type, "unknown property type '" + type.value + "', expected Float, Int, Integer, Range, "
                     "Color, Vector, 2D, 2DArray, 3D, Cube, CubeArray or Any.");
            }
            Next();

            if (decl.property_type == PropertyDecl::Type::Range)
            {
                const Token& at = Peek();
                const auto range = ParseTuple("Range");
                if (range.size() != 2) Fail(at, "Range must have exactly 2 values (min, max).");
                decl.range = std::make_pair(range[0], range[1]);
            }
            decl.type = RawText(type_start, pos_);

            Expect(TokenType::kBracketEnd, "')' after property type");
            ExpectOperator("=");

            const bool is_texture =
                decl.property_type == PropertyDecl::Type::Texture2D ||
                decl.property_type == PropertyDecl::Type::Texture2DArray ||
                decl.property_type == PropertyDecl::Type::Texture3D ||
                decl.property_type == PropertyDecl::Type::Cubemap ||
                decl.property_type == PropertyDecl::Type::CubemapArray ||
                decl.property_type == PropertyDecl::Type::TextureAny;

            if (is_texture)
            {
                decl.default_value = ExpectString("default texture name");
                if (PeekType(TokenType::kBlockBegin))
                {
                    SkipBlock(); // `{}` or legacy texture options such as `{ TexGen CubeReflect }`
                }
            }
            else if (PeekType(TokenType::kBracketBegin))
            {
                const size_t start = pos_;
                decl.default_numbers = ParseTuple("default value");
                decl.default_value = RawText(start, pos_);
            }
            else
            {
                decl.default_value = Peek().value;
                decl.default_numbers.push_back(ParseNumber("default value"));
            }

            return decl;
        }

        void ParseProperties(ShaderLabObject& shader)
        {
            Next(); // Properties
            Expect(TokenType::kBlockBegin, "'{' after Properties");
            if (!shader.properties.has_value()) shader.properties.emplace();

            // After an error, the next declaration is the next line starting with an attribute or `name (`.
            auto is_decl_start = [&](const size_t index, const uint32_t error_line)
            {
                const Token& token = *tokens_[index];
                if (token.line <= error_line) return false;
                if (token.type == TokenType::kAttributeBegin) return true;
                return IsWord(token) && index + 1 < tokens_.size() &&
                    tokens_[index + 1]->type == TokenType::kBracketBegin;
            };

            ParseItems("Properties", [&] { shader.properties->push_back(ParsePropertyDecl()); }, is_decl_start);
            Expect(TokenType::kBlockEnd, "'}'");
        }

        // ------------------------------------------------------------------ shader

        void ParseShaderItem(ShaderLabObject& shader)
        {
            const Token& token = Peek();
            if (PeekWord("Properties")) ParseProperties(shader);
            else if (PeekWord("SubShader")) shader.sub_shaders.push_back(ParseSubShader());
            else if (PeekWord("Category")) ParseCategory(shader);
            else if (IsIncludeBegin(token)) shader.includes.push_back(ParseProgram());
            else if (PeekWord("CustomEditorForRenderPipeline"))
            {
                Next();
                std::string editor = ExpectString("custom editor class name");
                std::string pipeline = ExpectString("render pipeline asset type");
                shader.custom_editors_for_render_pipeline.insert_or_assign(std::move(pipeline), std::move(editor));
            }
            else if (PeekWord("CustomEditor"))
            {
                Next();
                // `=` is not Unity syntax, but older versions of this parser expected it.
                if (PeekType(TokenType::kOperator) && Peek().value == "=") Next();
                shader.custom_editor = ExpectString("custom editor class name");
            }
            else if (PeekWord("Dependency"))
            {
                Next();
                std::string name = ExpectString("dependency name");
                ExpectOperator("=");
                shader.dependencies.insert_or_assign(std::move(name), ExpectString("dependency shader name"));
            }
            else if (PeekWord("Fallback"))
            {
                Next();
                if (PeekWord("Off"))
                {
                    Next();
                    shader.fallback.reset();
                }
                else
                {
                    shader.fallback = ExpectString("fallback shader name (or Off)");
                }
            }
            else
            {
                Fail(token, "unexpected " + Describe(token) + " in Shader, expected Properties, SubShader, "
                     "Category, Fallback, CustomEditor, CustomEditorForRenderPipeline, Dependency, "
                     "or an include block.");
            }
        }
    };
}

namespace sl_parser
{
    ShaderLabObject Parser::Parse(const Tokens& tokens)
    {
        return ParserImpl(tokens, nullptr).ParseShader();
    }

    ParseResult Parser::TryParse(const Tokens& tokens)
    {
        ParseResult result;
        result.shader = ParserImpl(tokens, &result.errors).ParseShader();
        return result;
    }

    ShaderLabObject Parser::ParseSource(const std::string& source)
    {
        return Parse(Lexer::Tokenize(source));
    }
}
