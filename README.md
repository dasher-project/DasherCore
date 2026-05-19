# The Dasher Text Entry System

Dasher is a zooming predictive text entry system, designed for situations where keyboard input is impractical (accessibility, PDAs, eyetrackers, switch access). It is usable with highly limited amounts of physical input while still allowing high rates of text entry.

## DasherCore

DasherCore is the platform-independent C++ engine. It handles language modelling, node tree management, rendering, and text output. Platform frontends (Windows, Android, GTK, iOS, web) link against DasherCore and provide only the UI and input handling.

This repository includes a **C API** (`dasher.h` / `CAPI.cpp`) that exposes DasherCore as a flat C shared library (`dasher.dll` / `libdasher.so`), making it accessible from C#, Kotlin, Swift, JS/WASM, Rust, or any language that can call C functions.

## Build Instructions

### Prerequisites

- CMake 3.12+
- C++17 compiler (MSVC, GCC, Clang)
- Git (for pugixml submodule)

### Quick Build

```bash
git clone --recursive https://github.com/dasher-project/DasherCore.git
cd DasherCore
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output:
- `build/DasherCore.lib` (or `.a`) — static library
- `build/bin/dasher.dll` (or `.so`/`.dylib`) — C API shared library

### Windows (MSVC)

A `build.cmd` is provided that sets up the MSVC environment:

```
build.cmd
```

This configures with Ninja and builds both DasherCore and the C API DLL.

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_CAPI` | `ON` | Build the C API shared library (`dasher.dll` / `libdasher.so`) |

To disable the C API: `cmake -B build -DBUILD_CAPI=OFF`

## C API

The C API lives in `src/dasher.h` (public header) and `src/CAPI.cpp` (implementation). It compiles into `dasher.dll` / `libdasher.so`.

### Design Principles

- **Platform-agnostic**: No OS-specific types or headers in the public API. All input/output is plain C types (`float`, `int`, `char*`).
- **No callbacks**: The API is a pull model. You call `dasher_frame()` to get draw commands. No function pointer registrations.
- **Zero allocation for callers**: All returned pointers point into internal buffers valid until the next API call. No `free()` needed.
- **Single header**: Include `dasher.h` and link against `dasher.dll` / `libdasher.so`. That's it.

### Session Lifecycle

```c
#include "dasher.h"

dasher_ctx* ctx = dasher_create("/path/to/DasherCore/Data");
dasher_set_screen_size(ctx, 800, 600);

while (running) {
    dasher_mouse_move(ctx, mouse_x, mouse_y);

    int* cmds;    int cmd_count;
    char** strs;  int str_count;
    dasher_frame(ctx, timestamp_ms, &cmds, &cmd_count, &strs, &str_count);

    // render cmds/strs with your canvas API
}

dasher_destroy(ctx);
```

### Input

The API takes `(float x, float y)` coordinates. Whether they came from a mouse, eyetracker, touchscreen, or switch scanning system is the frontend's problem. DasherCore doesn't know about OS-specific input devices.

- `dasher_mouse_move(ctx, x, y)` — update pointer position
- `dasher_mouse_down(ctx)` — start zooming (equivalent to holding the mouse button)
- `dasher_mouse_up(ctx)` — pause zooming

### Draw Commands

`dasher_frame()` returns a flat array of 6-int records: `[opcode, a, b, c, d, argb]`.

| Op | Meaning | Fields |
|----|---------|--------|
| 0 | Clear screen | argb = background colour |
| 1 | Circle | a=x, b=y, c=radius, d=1 filled / 0 outline, argb |
| 2 | Line segment | a=x1, b=y1, c=x2, d=y2, argb |
| 3 | Rectangle outline | a=x1, b=y1, c=x2, d=y2, argb |
| 4 | Rectangle filled | a=x1, b=y1, c=x2, d=y2, argb |
| 5 | Text | a=x, b=y, c=fontSize, d=stringIndex, argb |

For opcode 5, `d` is an index into the strings array. The `argb` format is `(alpha << 24) | (red << 16) | (green << 8) | blue`.

All returned pointers are valid only until the next `dasher_frame()` call on the same context. Do not free them.

### Text Output

```c
const char* text = dasher_get_output_text(ctx);   // valid until next API call
dasher_reset_output_text(ctx);                     // clear the buffer
```

### Settings

```c
// Alphabet
const char* alph = dasher_get_alphabet_id(ctx);
dasher_set_alphabet_id(ctx, "English with limited punctuation");

// Speed (100 = default, range 20-400)
int speed = dasher_get_speed_percent(ctx);
dasher_set_speed_percent(ctx, 150);

// Generic parameter access (see Parameters.h for enum values)
int val = dasher_get_bool_parameter(ctx, BP_START_MOUSE);
dasher_set_bool_parameter(ctx, BP_START_MOUSE, 1);

long bitrate = dasher_get_long_parameter(ctx, LP_MAX_BITRATE);
dasher_set_long_parameter(ctx, LP_MAX_BITRATE, 200);

const char* font = dasher_get_string_parameter(ctx, SP_DASHER_FONT);
dasher_set_string_parameter(ctx, SP_DASHER_FONT, "Arial");
```

### data_dir

`dasher_create(data_dir)` expects a path to DasherCore's `Data/` directory (or a parent of it). This directory must contain:

- `Data/alphabets/` — alphabet XML files
- `Data/colours/` — colour scheme XML files
- `Data/training/` — language model training data

The built-in `FileUtils` uses `std::filesystem` to locate these files. It searches `{data_dir, data_dir/Data}` for matching patterns like `alphabet.*.xml`.

### Thread Safety

A `dasher_ctx` is **not** thread-safe. One thread per context. If you need multiple Dasher instances (e.g. multiple windows), create one `dasher_ctx` per thread.

This is a fundamental limitation of DasherCore's C++ internals, which use mutable state extensively. A future Rust port could address this at the type level, but for now this is the contract.

### Architecture

```
┌─────────────────────────────────────┐
│  Platform Frontend (C#/Kotlin/Swift) │
│  - Canvas rendering                  │
│  - Input device handling             │
│  - UI chrome                         │
├─────────────────────────────────────┤
│  dasher.dll / libdasher.so          │
│  (C API — dasher.h)                  │
├─────────────────────────────────────┤
│  DasherCore (C++17 static library)  │
│  - Language models (PPM, word, etc.) │
│  - Node tree + rendering             │
│  - Settings + alphabets              │
│  - Command-buffer screen             │
│  - Pointer input device              │
└─────────────────────────────────────┘
```

The C API includes its own `CommandScreen` (serialises draw calls into the `[op, a, b, c, d, argb]` command buffer) and `PointerInput` (feeds screen coordinates into DasherCore). These are internal to the DLL — frontends never see C++ types.

## DasherCore Improvements (TODO)

Areas where DasherCore's C++ could be improved, noted during C API development:

- **`Realize()` and `NewFrame()` are protected** — the C API works around this via a subclass `DoRealize()`/`DoNewFrame()` wrapper. These should be public (or at minimum `internal`/`friend`) since any embedding needs them.
- **`std::filesystem` in `FileUtils`** — the built-in `FileUtils::ScanFiles` searches `{cwd, cwd/Data}`. This works but is fragile. A `data_dir` parameter would be cleaner (the C API works around it by calling `std::filesystem::current_path(data_dir)` before creating the engine).
- **`ColorPalette::Color` is a complex struct** — the command buffer protocol uses flat `int32_t argb` values, which is much simpler for frontends. The internal `Color` struct with named fields, hex string constructors, and lerp methods is overengineered for a rendering protocol.
- **Parameter enum is a single flat `enum Parameter`** — mixing `BP_*`, `LP_*`, `SP_*` in one enum loses type safety. A Rust port would use separate types.
- **`CDasherScreen::Label` lifetime management** — labels are heap-allocated by `MakeLabel()` and the caller must delete them. A RAII or arena approach would prevent leaks.
- **`FileLogger` / `UserLog` system** — pulls in Windows-specific headers (`WinCommon.h`) even when not on Windows. Should be behind a platform guard.
- **No tests** — DasherCore has no unit test infrastructure. The C API makes this even more important since frontends need a contract they can verify.

## Frontends

| Platform | Repo | Status |
|----------|------|--------|
| Windows (Avalonia) | [Dasher-Windows](https://github.com/dasher-project/Dasher-Windows) | In development |
| Android | [Dasher-Mobile](https://github.com/dasher-project/Dasher-Mobile) | Working (uses forked DasherCore) |
| GTK/Linux | [Dasher-GTK](https://github.com/dasher-project/Dasher-GTK) | Working |
| Web | [dasher-web](https://github.com/dasher-project/dasher-web) | In development |

## License

Dasher was originally built by Inference Group and released under GPL. This version is licensed under the MIT License.

[Read more about the relicensing process.](./LICENSE_NOTES.md)

## Support and Feedback

File bug reports in the issues of this repository.

The Dasher website: https://github.com/dasher-project
