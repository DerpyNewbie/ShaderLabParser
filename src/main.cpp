// Dumps the parse tree of a ShaderLab file.
// Usage: shader_lab_parser_test [--tokens] [file.shader]   (default file: test.shader)

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "Format.h"
#include "Lexer.h"
#include "Parser.h"

namespace
{
    using namespace sl_parser;

    void PrintTags(const std::optional<Tags>& tags, const std::string& indent)
    {
        if (!tags.has_value()) return;
        std::cout << indent << "tags: " << tags->size() << std::endl;
        for (const auto& [key, value] : *tags)
        {
            std::cout << indent << "  " << key << " = " << value << std::endl;
        }
    }

    void PrintCommands(const std::optional<Commands>& commands, const std::string& indent)
    {
        if (!commands.has_value()) return;
        std::cout << indent << "commands: " << commands->size() << std::endl;
        for (const auto& command : *commands)
        {
            std::cout << indent << "  " << FormatCommand(*command) << std::endl;
        }
    }

    void PrintPackageRequirements(const std::optional<PackageRequirements>& requirements, const std::string& indent)
    {
        if (!requirements.has_value()) return;
        std::cout << indent << "package requirements: " << requirements->size() << std::endl;
        for (const auto& [package, version] : *requirements)
        {
            std::cout << indent << "  " << package << (version.empty() ? "" : ": " + version) << std::endl;
        }
    }

    void PrintProgram(const ShaderProgram& program, const std::string& label, const std::string& indent)
    {
        std::cout << indent << label << " (" << to_string(program.language) << ", line " << program.line << "):"
            << std::endl;
        std::cout << "------ SHADER PROGRAM BEGIN ------" << program.program << "------ SHADER PROGRAM END ------"
            << std::endl;
    }

    void PrintPrograms(const ShaderPrograms& programs, const std::string& label, const std::string& indent)
    {
        for (const auto& program : programs) PrintProgram(program, label, indent);
    }
}

int main(const int argc, char** argv)
{
    bool print_tokens = false;
    std::string path = "test.shader";
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--tokens") print_tokens = true;
        else path = argv[i];
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    const std::string program = buffer.str();

    Tokens tokens;
    try
    {
        tokens = Lexer::Tokenize(program);
    }
    catch (const ParseError& e)
    {
        std::cerr << path << ":" << e.what() << std::endl;
        return 1;
    }

    if (print_tokens)
    {
        std::cout << "------      Tokens      ------" << std::endl;
        for (const auto& token : tokens)
        {
            std::cout << token.line << ":" << token.pos + 1 << " " << to_string(token.type) << " "
                << (token.type == TokenType::kNewLine ? "\\n" : token.value) << std::endl;
        }
        std::cout << "------    End Tokens    ------" << std::endl;
    }

    const auto result = Parser::TryParse(tokens);
    const auto& shader = result.shader;

    std::cout << "------   Parse Result   ------" << std::endl;
    std::cout << "shader name: " << shader.name << std::endl;

    const auto properties = shader.properties.value_or(Properties{});
    std::cout << "properties: " << properties.size() << std::endl;
    for (const auto& property : properties)
    {
        std::cout << "  ";
        for (const auto& attribute : property.attributes.value_or(PropertyAttributes{}))
        {
            std::cout << "[" << attribute.name;
            if (attribute.arguments.has_value()) std::cout << "(" << *attribute.arguments << ")";
            std::cout << "]";
        }
        std::cout << property.name << ", " << property.display_name << ", " << property.type << ", "
            << property.default_value << std::endl;
    }

    PrintPrograms(shader.includes, "include", "");

    std::cout << "sub shaders: " << shader.sub_shaders.size() << std::endl;
    for (const auto& sub_shader : shader.sub_shaders)
    {
        if (sub_shader.lod.has_value()) std::cout << "  lod: " << *sub_shader.lod << std::endl;
        PrintTags(sub_shader.tags, "  ");
        PrintCommands(sub_shader.commands, "  ");
        PrintPackageRequirements(sub_shader.package_requirements, "  ");
        PrintPrograms(sub_shader.includes, "include", "  ");
        PrintPrograms(sub_shader.shader_programs, "shader program", "  ");

        std::cout << "  passes: " << sub_shader.passes.size() << std::endl;
        for (const auto& ordered_pass : sub_shader.passes)
        {
            std::cout << "    " << static_cast<int>(ordered_pass->order) << ": "
                << to_string(ordered_pass->GetPassType()) << std::endl;

            switch (ordered_pass->GetPassType())
            {
            case PassType::Definition:
                {
                    const auto* pass = dynamic_cast<const Pass*>(ordered_pass.get());
                    if (pass->name.has_value()) std::cout << "      name: " << *pass->name << std::endl;
                    PrintTags(pass->tags, "      ");
                    PrintCommands(pass->commands, "      ");
                    PrintPackageRequirements(pass->package_requirements, "      ");
                    PrintPrograms(pass->includes, "include", "      ");
                    if (pass->shader_program.has_value())
                    {
                        PrintProgram(*pass->shader_program, "shader program", "      ");
                    }
                    break;
                }
            case PassType::Use:
                {
                    const auto* use_pass = dynamic_cast<const UsePass*>(ordered_pass.get());
                    std::cout << "      use: " << use_pass->target_pass << std::endl;
                    break;
                }
            case PassType::Grab:
                {
                    const auto* grab_pass = dynamic_cast<const GrabPass*>(ordered_pass.get());
                    std::cout << "      grab: " << grab_pass->target_texture.value_or("(default _GrabTexture)")
                        << std::endl;
                    if (grab_pass->name.has_value()) std::cout << "      name: " << *grab_pass->name << std::endl;
                    PrintTags(grab_pass->tags, "      ");
                    break;
                }
            }
        }
    }

    if (shader.custom_editor.has_value()) std::cout << "custom editor: " << *shader.custom_editor << std::endl;
    for (const auto& [pipeline, editor] : shader.custom_editors_for_render_pipeline)
    {
        std::cout << "custom editor for " << pipeline << ": " << editor << std::endl;
    }
    for (const auto& [name, target] : shader.dependencies)
    {
        std::cout << "dependency: " << name << " = " << target << std::endl;
    }
    std::cout << "fallback: " << shader.fallback.value_or("(none)") << std::endl;
    std::cout << "------ End Parse Result ------" << std::endl;

    for (const auto& error : result.errors)
    {
        std::cerr << path << ":" << error.what() << std::endl;
    }
    return result.Succeeded() ? 0 : 1;
}
