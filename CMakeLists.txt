cmake_minimum_required(VERSION 4.1)
project(shader_lab_parser_test)

set(CMAKE_CXX_STANDARD 20)

configure_file(test.shader test.shader COPYONLY)

add_executable(shader_lab_parser_test
        src/main.cpp
        src/Lexer.cpp
        src/Lexer.h
        src/Parser.cpp
        src/Parser.h
        src/Token.h
        src/ShaderLab.h
        src/Command.h
        src/TokenHelper.cpp
        src/TokenHelper.h
)

project(shader_lab_parser VERSION 1.0.0 DESCRIPTION "ShaderLab syntax parser written in cpp.")
add_library(shader_lab_parser
        src/Lexer.cpp
        src/Parser.cpp
        src/TokenHelper.cpp
)

set_target_properties(shader_lab_parser PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(shader_lab_parser PROPERTIES PUBLIC_HEADER include/shader_lab_parser.h)

target_include_directories(shader_lab_parser PRIVATE .)
include(GNUInstallDirs)
install(TARGETS shader_lab_parser
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)