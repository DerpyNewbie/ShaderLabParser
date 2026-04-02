#include <fstream>
#include <iostream>
#include <vector>
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"

int main()
{
    std::ifstream ifs;
    ifs.open("test.shader");
    if (!ifs.is_open())
    {
        std::cerr << "Failed to open file" << std::endl;
    }

    if (ifs.peek() != EOF)
    {
        unsigned char bom[3];
        ifs.read(reinterpret_cast<char*>(bom), 3);
        if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
        {
            std::cout << "BOM detected and skipped." << std::endl;
        }
        else
        {
            ifs.seekg(0);
        }
    }

    const auto program = std::string{
        std::istreambuf_iterator(ifs), std::istreambuf_iterator<char>()
    };

    std::cout << program << std::endl;

    std::cout << "------      Tokens      ------" << std::endl;
    const auto tokens = sl_parser::Lexer::Tokenize(program);
    for (const auto& [type, value, line, pos] : tokens)
    {
        std::cout << to_string(type) << " " << value << std::endl;
    }
    std::cout << "------    End Tokens    ------" << std::endl;

    // return 0;

    const auto shader_lab_object = sl_parser::Parser::Parse(tokens);

    std::cout << "------   Parse Result   ------" << std::endl;
    std::cout << "shader name: " << shader_lab_object.name << std::endl;

    auto properties = shader_lab_object.properties.value_or(sl_parser::Properties{});
    std::cout << "properties: " << std::to_string(properties.size()) << std::endl;
    for (const auto& property : properties)
    {
        std::cout << "  " << property.name << ", " << property.display_name << ", " << property.type << std::endl;
    }

    std::cout << "sub shaders: " << std::to_string(shader_lab_object.sub_shaders.size()) << std::endl;
    for (const auto& sub_shader : shader_lab_object.sub_shaders)
    {
        if (sub_shader.lod.has_value())
        {
            std::cout << "  lod: " << sub_shader.lod.value() << std::endl;
        }

        if (sub_shader.tags.has_value())
        {
            std::cout << "  tags: " << sub_shader.tags->size() << std::endl;
            for (const auto& [key, value] : *sub_shader.tags)
            {
                std::cout << "    " << key << ", " << value << std::endl;
            }
        }

        if (sub_shader.commands.has_value())
        {
            std::cout << "  commands: " << sub_shader.commands->size() << std::endl;
            for (const auto& command : *sub_shader.commands)
            {
                std::cout << "    " << command << std::endl;
            }
        }

        std::cout << "  pass: " << sub_shader.passes.size() << std::endl;
        for (const auto& ordered_pass : sub_shader.passes)
        {
            std::cout << "    " << std::to_string(ordered_pass->order) << ": " << to_string(ordered_pass->GetPassType())
                << std::endl;

            switch (ordered_pass->GetPassType())
            {
            case sl_parser::PassType::Definition:
                {
                    auto pass = dynamic_cast<sl_parser::Pass*>(ordered_pass.get());
                    if (pass->name.has_value())
                    {
                        std::cout << "      name: " << *pass->name << std::endl;
                    }

                    if (pass->tags.has_value())
                    {
                        std::cout << "      tags: " << pass->tags->size() << std::endl;
                        for (const auto& [key, value] : *pass->tags)
                        {
                            std::cout << "        " << key << ", " << value << std::endl;
                        }
                    }

                    if (pass->commands.has_value())
                    {
                        std::cout << "      commands: " << pass->commands->size() << std::endl;
                        for (const auto& command : *pass->commands)
                        {
                            std::cout << "        " << command << std::endl;
                        }
                    }

                    if (pass->shader_program.has_value())
                    {
                        std::cout << "      shader program: " << std::endl;
                        std::cout << "------ SHADER PROGRAM BEGIN ------" << std::endl;
                        std::cout << pass->shader_program->program << std::endl;
                        std::cout << "------ SHADER PROGRAM END ------" << std::endl;
                    }

                    break;
                }
            case sl_parser::PassType::Use:
                {
                    auto use_pass = dynamic_cast<sl_parser::UsePass*>(ordered_pass.get());
                    std::cout << "      use: " << use_pass->target_pass << std::endl;
                    break;
                }
            case sl_parser::PassType::Grab:
                {
                    auto grab_pass = dynamic_cast<sl_parser::GrabPass*>(ordered_pass.get());
                    std::cout << "      grab: " << grab_pass->target_texture.value_or("!!EMPTY!!") << std::endl;
                    break;
                }
            }
        }
    }
    std::cout << "------ End Parse Result ------" << std::endl;
}
