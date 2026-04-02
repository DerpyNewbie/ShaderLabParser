cmake_minimum_required(VERSION 3.25)

project(shader_lab_parser VERSION 1.0.0 DESCRIPTION "ShaderLab syntax parser written in cpp.")

include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

# Allow users to choose between shared and static
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

configure_file(test.shader test.shader COPYONLY)

add_library(shader_lab_parser
        src/Lexer.cpp
        src/Parser.cpp
        src/TokenHelper.cpp
)

target_include_directories(shader_lab_parser PRIVATE . PUBLIC src)

generate_export_header(shader_lab_parser
        BASE_NAME SL_PARSER
        EXPORT_MACRO_NAME SL_PARSER_EXPORTS
        EXPORT_FILE_NAME sl_exports.h
        STATIC_DEFINE SL_PARSER_STATIC_DEFINE
)

target_compile_definitions(shader_lab_parser PUBLIC $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:SL_PARSER_STATIC_DEFINE>)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(shader_lab_parser_test
        src/main.cpp
)
target_link_libraries(shader_lab_parser_test shader_lab_parser)

set_target_properties(shader_lab_parser PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(shader_lab_parser PROPERTIES PUBLIC_HEADER src/ShaderLab.h)

include(GNUInstallDirs)
install(TARGETS shader_lab_parser
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)