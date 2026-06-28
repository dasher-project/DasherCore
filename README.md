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
- Python 3.6+ (for parameter code generation; optional — see below)

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

### Parameter Code Generation

DasherCore's parameter definitions (`src/DasherCore/Parameters.cpp`) are generated from a single JSON manifest:

```
settings_manifest.json  →  Scripts/generate_parameters.py  →  src/DasherCore/Parameters.cpp
```

The manifest is the source of truth for all parameter metadata: names, descriptions, types, defaults, ranges, UI hints, input filter associations, and visibility tiers.

**When Python is available** (the default), CMake's `add_custom_command` auto-regenerates `Parameters.cpp` whenever `settings_manifest.json` changes.

**When Python is not available**, the committed `Parameters.cpp` is used as-is. To regenerate manually:

```bash
python3 Scripts/generate_parameters.py
```

See `settings_manifest.json` for the full schema. Key fields per parameter:

| Field | Purpose |
|-------|---------|
| `key` | C++ enum constant (e.g. `BP_AUTOCALIBRATE`) |
| `storageName` | Persistence key in settings XML |
| `label` | Human-readable name for UI |
| `description` | Longer explanation for UI |
| `tier` | `common` / `advanced` / `expert` — progressive disclosure |
| `subgroup` | Input filter class association (e.g. `CSmoothingFilter`) |
| `dependsOn` | Conditional: only show when named param is true |
| `platformDefaults` | Platform-specific defaults (e.g. `"ios": true`) |
| `enumValues` | Dropdown options with C++ expressions |

### Localization (i18n)

All translatable strings live in one place: `settings_manifest.json`. The engine itself always uses English strings (baked into `Parameters.cpp`). Localization is handled at runtime via `dasher_set_locale()`.

**Extract strings for translators:**

```bash
python3 Scripts/extract_strings.py
# → Strings/strings_en.json (201 flat key-value pairs)
```

This produces a simple JSON file that translators edit. Keys use the C++ enum constant names:

```json
{
    "BP_DRAW_MOUSE_LINE.label": "Draw Mouse Line",
    "BP_DRAW_MOUSE_LINE.description": "When enabled, a line is drawn...",
    "LP_GEOMETRY.enum.Old Style": "Old Style"
}
```

A translator creates `strings_de.json` with the same keys and translated values. No special tools required — any text editor works. The C API loads these at runtime via `dasher_set_locale(ctx, "de")`.

**Apply a translation to produce a localized manifest (for C++ platforms):**

```bash
python3 Scripts/extract_strings.py --apply Strings/strings_de.json --locale de
# → settings_manifest_de.json (localized manifest)
python3 Scripts/generate_parameters.py --manifest settings_manifest_de.json
# → Parameters.cpp with German labels/descriptions
cmake --build build
```

This produces a German `Parameters.cpp` where the C API returns German strings via `dasher_parameter_info`. Most platforms should prefer the runtime approach (`dasher_set_locale`) instead.

**Runtime locale approach (recommended for all platforms):**

```bash
# Just bundle Strings/ alongside Data/ in the app package
# The C API loads the right file at runtime:
dasher_set_locale(ctx, "de");  # loads Strings/strings_de.json
```

**Platform-specific approaches:**

| Platform | Approach |
|----------|----------|
| **All platforms** | Use `dasher_set_locale(ctx, "de")` to load `Strings/strings_de.json` at runtime. Engine returns localized names/descriptions via `dasher_get_parameter_info()`. Falls back to English for missing keys. |
| **C++ only** (Windows, Linux) | Alternatively, generate locale-specific `Parameters.cpp` and rebuild. One build per locale. |

The key principle: **`strings_en.json` is the single template for translators**. Platform repos consume it and convert to whatever format they need. DasherCore itself stays format-agnostic.

### C API: Localization

```c
// Set locale — loads strings_{locale}.json from {data_dir}/Strings/
dasher_set_locale(ctx, "de");       // returns 0 on success, -1 if file not found
dasher_set_locale(ctx, "en");       // reset to English (built-in defaults)

const char* loc = dasher_get_locale(ctx);  // "de"

// After setting locale, dasher_get_parameter_info() returns translated strings:
dasher_parameter_info info;
dasher_get_parameter_info(0, &info);
// info.name is now "Mauslinie zeichnen" (if German translation exists)
// Falls back to English if no translation found

// Override individual strings (takes precedence over locale file):
dasher_set_string_override(ctx, "BP_DRAW_MOUSE_LINE.label", "Custom Label");

// Query any localized string directly:
const char* s = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
```

The `Strings/` directory must be bundled alongside `Data/` in the app package. On desktop platforms it's typically `{data_dir}/Strings/strings_de.json`. On mobile platforms it goes in the app bundle alongside the `Data/` folder.

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

dasher_ctx* ctx = dasher_create("/path/to/DasherCore/Data", NULL, NULL);
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

- **Parameter enum is a single flat `enum Parameter`** — mixing `BP_*`, `LP_*`, `SP_*` in one enum loses type safety. A Rust port would use separate types.
- **`CDasherScreen::Label` lifetime management** — labels are heap-allocated by `MakeLabel()` and the caller must delete them. A RAII or arena approach would prevent leaks.

### Training Data

Training files in `Data/training/` are generated from the **KeithAAC multilingual AAC text datasets** by Keith Vertanen:

> **Multilingual Text For Augmentative And Alternative Communication (AAC)**  
> https://osf.io/bnfdv/wiki?wiki=7pgfj  
> This material is based upon work supported by the National Science Foundation under Grant No. 1750193.

The repository contains automatically translated versions of four conversational datasets:

| Dataset | Description |
|---------|-------------|
| **COMM2** | Statements and questions written in response to ten everyday communication situations |
| **Turk** | Six-turn dialogues about different topics |
| **Imagine** | Messages written by people asked to imagine using a scanning-style AAC interface |
| **DailyDialog** | Multi-turn dialogues about different topics (first 20% of original dataset) |

Translation was performed in July 2025 using Google's LLM translation API into 33 languages. For COMM2, Turk, and Imagine, Llama4 Maverick was used to correct spelling/grammar errors and produce additional variants.

**Licensing:**
- COMM2, Turk, and Imagine datasets: **CC BY 4.0**
- DailyDialog dataset: **CC BY-NC-SA 4.0** (retains original authors' license)

Training files are generated via `Scripts/generate_training.py`, which extracts real conversational utterances from these datasets for each language.

### Alphabets

Alphabet XML files in `Data/alphabets/autoConverted/` are generated from the **WorldAlphabets** project using `Scripts/generate_training.py`. WorldAlphabets provides character sets, keyboard layouts, and frequency data for 344 languages.

## License

Dasher was originally built by Inference Group and released under GPL. This version is licensed under the MIT License.

[Read more about the relicensing process.](./LICENSE_NOTES.md)

## Support and Feedback

File bug reports in the issues of this repository.

The Dasher website: https://github.com/dasher-project
