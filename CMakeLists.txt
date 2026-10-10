cmake_minimum_required(VERSION 3.25)

project(shader_lab_parser VERSION 0.1.0 DESCRIPTION "ShaderLab syntax parser written in cpp.")

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

# Allow users to choose between shared and static
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(SHADER_LAB_PARSER_BUILD_TESTS "Build the tests and the dump tool" ${PROJECT_IS_TOP_LEVEL})

add_library(shader_lab_parser
        src/Format.cpp
        src/Lexer.cpp
        src/Parser.cpp
        src/TokenHelper.cpp
)
add_library(shader_lab_parser::shader_lab_parser ALIAS shader_lab_parser)

set(SHADER_LAB_PARSER_PUBLIC_HEADERS
        src/Command.h
        src/Format.h
        src/Lexer.h
        src/ParseError.h
        src/Parser.h
        src/ShaderLab.h
        src/Token.h
        src/TokenHelper.h
        ${CMAKE_CURRENT_BINARY_DIR}/sl_exports.h
)

target_include_directories(shader_lab_parser PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        # Headers are installed to include/shader_lab_parser; both `"Parser.h"` and
        # `<shader_lab_parser/Parser.h>` work for consumers.
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/shader_lab_parser>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

generate_export_header(shader_lab_parser
        BASE_NAME SL_PARSER
        EXPORT_MACRO_NAME SL_PARSER_EXPORTS
        EXPORT_FILE_NAME sl_exports.h
        STATIC_DEFINE SL_PARSER_STATIC_DEFINE
)

if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(shader_lab_parser PUBLIC SL_PARSER_STATIC_DEFINE)
endif ()

set_target_properties(shader_lab_parser PROPERTIES
        VERSION ${PROJECT_VERSION}
        PUBLIC_HEADER "${SHADER_LAB_PARSER_PUBLIC_HEADERS}"
)

if (MSVC)
    target_compile_options(shader_lab_parser PRIVATE /utf-8)
endif ()

if (SHADER_LAB_PARSER_BUILD_TESTS)
    configure_file(test.shader test.shader COPYONLY)

    add_executable(shader_lab_parser_test src/main.cpp)
    target_link_libraries(shader_lab_parser_test PRIVATE shader_lab_parser)

    enable_testing()
    add_executable(shader_lab_parser_unit_tests tests/ParserTests.cpp)
    target_link_libraries(shader_lab_parser_unit_tests PRIVATE shader_lab_parser)
    target_compile_definitions(shader_lab_parser_unit_tests PRIVATE
            SHADER_LAB_PARSER_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    if (MSVC)
        target_compile_options(shader_lab_parser_unit_tests PRIVATE /utf-8)
    endif ()
    add_test(NAME shader_lab_parser_unit_tests COMMAND shader_lab_parser_unit_tests)
    add_test(NAME shader_lab_parser_dump_sample COMMAND shader_lab_parser_test ${CMAKE_CURRENT_SOURCE_DIR}/test.shader)
endif ()

install(TARGETS shader_lab_parser
        EXPORT shader_lab_parserTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shader_lab_parser
)

install(EXPORT shader_lab_parserTargets
        NAMESPACE shader_lab_parser::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/shader_lab_parser
)

configure_package_config_file(cmake/shader_lab_parserConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/shader_lab_parserConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/shader_lab_parser
)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/shader_lab_parserConfigVersion.cmake
        COMPATIBILITY SameMinorVersion
)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/shader_lab_parserConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/shader_lab_parserConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/shader_lab_parser
)
