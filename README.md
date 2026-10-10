# ShaderLabParser

A ShaderLab (Unity `.shader`) syntax parser written in C++20.

## Usage

```cpp
#include "Format.h"
#include "Parser.h"

// Throws sl_parser::ParseError ("line L:C: message") on the first error.
sl_parser::ShaderLabObject shader = sl_parser::Parser::ParseSource(source);

// Or collect every error and keep whatever parsed successfully.
auto result = sl_parser::Parser::TryParse(sl_parser::Lexer::Tokenize(source));
for (const auto& error : result.errors) std::cerr << error.what() << '\n';

for (const auto& sub_shader : shader.sub_shaders)
    if (sub_shader.commands)
        for (const auto& command : *sub_shader.commands)
            std::cout << sl_parser::FormatCommand(*command) << '\n'; // e.g. "Blend SrcAlpha OneMinusSrcAlpha"
```

With CMake:

```cmake
find_package(shader_lab_parser CONFIG REQUIRED)
target_link_libraries(main PRIVATE shader_lab_parser::shader_lab_parser)
```

## Supported syntax

- `Shader`, `Properties` (all property types, attributes with arguments, all default forms), `SubShader`,
  `Category`, `Pass`, `GrabPass`, `UsePass`, `Fallback` (including `Fallback Off`), `CustomEditor`,
  `CustomEditorForRenderPipeline`, `Dependency`, `PackageRequirements`, `Tags`, `LOD`, `Name`.
- Render state commands in `SubShader`, `Pass` and `Category`: `AlphaToMask`, `Blend`, `BlendOp`, `ColorMask`,
  `Conservative`, `Cull`, `Offset`, `Stencil`, `ZClip`, `ZTest`, `ZWrite` and legacy `Fog`.
  Arguments can be material property references, e.g. `Cull [_CullMode]` (see `sl_parser::Value<T>`).
- `CGPROGRAM` / `HLSLPROGRAM` / `GLSLPROGRAM` and the matching `*INCLUDE` blocks at Shader, SubShader and Pass
  level. Program source is kept verbatim.
- Keywords and values are case-insensitive, as in Unity, except program delimiters such as `CGPROGRAM` and `ENDCG`,
  which are case-sensitive (also as in Unity). Tabs, CRLF line endings and a UTF-8 BOM are handled.

Not supported: legacy fixed-function commands (`Lighting`, `Material`, `SetTexture`, `AlphaTest`, ...). They are
reported as errors.

`Category` is flattened: its commands, tags, LOD and includes are copied into each sub shader it contains, and
the sub shader's own values take precedence.

## Building and testing

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/shader_lab_parser_test path/to/file.shader   # dump the parse tree (--tokens to print tokens)
```

Tests and the dump tool are built when this is the top-level project (`SHADER_LAB_PARSER_BUILD_TESTS`).
