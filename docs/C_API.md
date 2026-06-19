# DasherCore C API

## Overview

The DasherCore C API (`src/dasher.h` / `src/CAPI.cpp`) wraps the C++ predictive text engine so that any language — Swift, Kotlin, C#, JavaScript (via WASM), Rust — can integrate Dasher without touching C++ directly.

The API is designed around a single opaque session handle (`dasher_ctx`) with a frame-based rendering loop: you feed pointer input, call `dasher_frame()` to advance the engine, and receive an array of draw commands to render with your platform's canvas API.

**Header:** `src/dasher.h`  
**Implementation:** `src/CAPI.cpp`  
**Tests:** `tests/test_capi.cpp`

## Architecture

```
Frontend (Swift / Kotlin / C# / ...)
         │
    dasher.h  (C API)
         │
    CAPI.cpp
         │
    ┌────────────────────────────────────────────┐
    │  dasher_ctx::Interface                     │
    │    extends CDashIntfScreenMsgs             │
    │      extends CDashIntfSettings             │
    │        extends CDasherInterfaceBase        │
    └────────────────────────────────────────────┘
         │                    │
    CommandScreen        PointerInput
    (captures draw       (wraps pointer
     commands into       position into
     int32 arrays)       Dasher input system)
```

### Design Principles

- **Single opaque handle** — one `dasher_ctx*` per session
- **Not thread-safe** — one thread per context
- **Zero-copy rendering** — draw commands returned as raw `int32` arrays (6 ints per command), valid only until the next `dasher_frame()` call
- **Ephemeral string pointers** — all `const char*` returns are valid only until the next API call on the same context
- **Null-safe** — every function handles `NULL ctx` gracefully (returns empty/zero, never crashes)

## Building

The C API is built with CMake:

```bash
mkdir build && cd build
cmake .. -DBUILD_CAPI=ON -DBUILD_TESTS=ON -DTEST_DATA_DIR=/path/to/Data
cmake --build .
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_CAPI` | ON | Build the C API shared library (`libdasher`) |
| `BUILD_TESTS` | ON | Build unit tests (requires `BUILD_CAPI=ON`) |
| `TEST_DATA_DIR` | (none) | Compile-time path to `Data/` directory for tests |

### Targets

| Target | Type | Output |
|--------|------|--------|
| `DasherCore` | Static library | Core C++ engine |
| `dasher` | Shared library | C API (`src/CAPI.cpp`) |
| `dasher_test` | Executable | Unit tests |

### Code Generation

`settings_manifest.json` is processed by `Scripts/generate_parameters.py` at configure time to auto-generate `src/DasherCore/Parameters.cpp`. Python 3 is required.

## Quick Start

```c
#include "dasher.h"

// 1. Create session
char* error = NULL;
dasher_ctx* ctx = dasher_create("/path/to/Data", "/path/to/user_dir", &error);
if (!ctx) {
    fprintf(stderr, "Failed: %s\n", error);
    return 1;
}

// 2. Set screen size (triggers engine initialization)
dasher_set_screen_size(ctx, 800, 600);

// 3. Configure (optional)
dasher_set_speed_percent(ctx, 120);
dasher_set_alphabet_id(ctx, "English");
dasher_set_locale(ctx, "de");

// 4. Main loop
while (running) {
    dasher_mouse_move(ctx, pointer_x, pointer_y);

    int* cmds;    int cmd_count;
    char** strs;  int str_count;
    dasher_frame(ctx, time_ms, &cmds, &cmd_count, &strs, &str_count);

    // Render draw commands
    for (int i = 0; i < cmd_count; i += 6) {
        render_command(cmds[i], cmds[i+1], cmds[i+2],
                       cmds[i+3], cmds[i+4], cmds[i+5], strs);
    }
}

// 5. Cleanup
dasher_save_settings(ctx);
dasher_destroy(ctx);
```

## Session Lifecycle

### `dasher_create`

```c
dasher_ctx* dasher_create(const char* data_dir, const char* user_dir, char** out_error);
```

Creates a new Dasher session.

- **`data_dir`** — Path to DasherCore's `Data/` directory (alphabets, colours, training files). Must be readable.
- **`user_dir`** — Writable directory for settings. If `NULL`, `data_dir` is used. Settings are stored in `<user_dir>/dasher_settings.xml`.
- **`out_error`** — If not `NULL`, set to a human-readable error string on failure. Do NOT free. Valid until next API call.
- **Returns** — Session handle, or `NULL` on failure.

### `dasher_destroy`

```c
void dasher_destroy(dasher_ctx* ctx);
```

Destroys a session and frees all resources. Safe to call with `NULL`.

### `dasher_set_screen_size`

```c
void dasher_set_screen_size(dasher_ctx* ctx, int width, int height);
```

Sets the canvas dimensions. **Must be called before `dasher_frame()`** — it triggers engine initialization (`Realize()`). Call again on window resize.

## Input

### Pointer Input

```c
void dasher_mouse_move(dasher_ctx* ctx, float x, float y);
void dasher_mouse_down(dasher_ctx* ctx);
void dasher_mouse_up(dasher_ctx* ctx);
```

- **`mouse_move`** — Feed pointer coordinates (mouse, touch, eyetracker). Origin top-left, pixels. Values are clamped to screen bounds.
- **`mouse_down`** — Signal pointer press (starts Dasher zooming).
- **`mouse_up`** — Signal pointer release (pauses Dasher zooming).

### Key Input

```c
void dasher_key_event(dasher_ctx* ctx, int key, int pressed);
```

For switch access, keyboard, or button input. Key values:

| Key | Value |
|-----|-------|
| Start/Stop | 0 |
| Button 1–4 | 1–4 |
| Primary | 100 |
| Secondary | 101 |
| Tertiary | 102 |

`pressed`: 1 for key down, 0 for key up.

## Rendering

### `dasher_frame`

```c
void dasher_frame(dasher_ctx* ctx, int64_t time_ms,
                  int** out_commands, int* out_command_count,
                  char*** out_strings, int* out_string_count);
```

Advances one frame and returns draw commands.

- **`time_ms`** — Current time in milliseconds. If <= 0, treated as 0.
- **`out_commands`** — Set to internal command buffer (array of `int`). Valid until next `dasher_frame()`. Do NOT free.
- **`out_command_count`** — Set to **total number of ints** (not number of commands — divide by 6 for command count).
- **`out_strings`** — Array of `char*` string pointers. Valid until next `dasher_frame()`.
- **`out_string_count`** — Number of strings.

### Draw Command Format

Each command is 6 `int32_t` values: `[opcode, a, b, c, d, argb]`

| Opcode | Name | Fields | Description |
|--------|------|--------|-------------|
| 0 | Clear screen | argb = background colour | Fill entire canvas |
| 1 | Circle | a=x, b=y, c=radius, d=1 filled / 0 outline, argb | Circle shape |
| 2 | Line | a=x1, b=y1, c=x2, d=y2, argb | Line segment |
| 3 | Rectangle outline | a=x1, b=y1, c=x2, d=y2, argb | Rectangle outline |
| 4 | Rectangle filled | a=x1, b=y1, c=x2, d=y2, argb | Filled rectangle |
| 5 | Text | a=x, b=y, c=fontSize, d=stringIndex, argb | Render string from strings array |

**ARGB format:** `(alpha << 24) | (red << 16) | (green << 8) | blue`

### Rendering Example

```c
for (int i = 0; i < cmd_count; i += 6) {
    int opcode = cmds[i+0];
    int a = cmds[i+1], b = cmds[i+2], c = cmds[i+3], d = cmds[i+4];
    int argb = cmds[i+5];

    switch (opcode) {
        case 0: fill_background(argb); break;
        case 1: draw_circle(a, b, c, d == 1, argb); break;
        case 2: draw_line(a, b, c, d, argb); break;
        case 3: draw_rect_outline(a, b, c, d, argb); break;
        case 4: draw_rect_filled(a, b, c, d, argb); break;
        case 5: draw_text(a, b, c, strs[d], argb); break;
    }
}
```

### Color Utilities

```c
int dasher_color_argb(int alpha, int red, int green, int blue);  // Create ARGB
int dasher_color_rgb(int red, int green, int blue);              // Create opaque (alpha=255)
int dasher_color_get_alpha(int argb);                            // Extract alpha
int dasher_color_get_red(int argb);                              // Extract red
int dasher_color_get_green(int argb);                            // Extract green
int dasher_color_get_blue(int argb);                             // Extract blue
```

## Output Text

```c
const char* dasher_get_output_text(dasher_ctx* ctx);    // Current buffer
void dasher_reset_output_text(dasher_ctx* ctx);          // Clear text, keep model position
void dasher_reset(dasher_ctx* ctx);                      // Clear text AND reset model to start
```

### Output Callback

```c
typedef void (*dasher_output_callback)(int event_type, const char* text, void* user_data);
void dasher_set_output_callback(dasher_ctx* ctx, dasher_output_callback callback, void* user_data);
```

Receives real-time text events without polling. Event types:

| Type | Meaning |
|------|---------|
| 0 | Text output (insertion) |
| 1 | Text delete (backspace) |

The callback fires on the thread calling `dasher_frame()`.

### Message Callback

```c
typedef void (*dasher_message_callback)(int message_type, const char* text, void* user_data);
void dasher_set_message_callback(dasher_ctx* ctx, dasher_message_callback callback, void* user_data);
```

Receives engine messages (warnings, errors, info). When registered, default canvas-message rendering is suppressed — the frontend handles display. Message types:

| Type | Meaning |
|------|---------|
| 0 | Informational (non-modal) |
| 1 | Warning (modal, pauses entry) |

## Alphabets

```c
int         dasher_get_alphabet_count(dasher_ctx* ctx);
const char* dasher_get_alphabet_name(dasher_ctx* ctx, int index);
const char* dasher_get_alphabet_id(dasher_ctx* ctx);
void        dasher_set_alphabet_id(dasher_ctx* ctx, const char* alphabet_id);
```

- Alphabet IDs are human-readable names like `"English with limited punctuation"`.
- Setting a new alphabet clears the edit buffer.
- If called before `dasher_set_screen_size`, the alphabet is stored and applied after initialization.

## Language Models

```c
int         dasher_get_language_model_count(void);             // Number of registered LMs
int         dasher_get_language_model_id_at(int index);        // ID by index (0..count-1)
const char* dasher_get_language_model_name(int id);            // Display name
const char* dasher_get_language_model_description(int id);     // Description
int         dasher_get_language_model_id(dasher_ctx* ctx);     // Current active LM
void        dasher_set_language_model_id(dasher_ctx* ctx, int model_id);  // Switch LM
int         dasher_get_language_model_param_count(int id);     // LM-specific params
int         dasher_get_language_model_param_key(int id, int index);
```

### Built-in Language Models

| ID | Name | Description |
|----|------|-------------|
| 0 | PPM | Prediction by Partial Match (default) |
| 2 | Word | Word-level language model |
| 3 | Mixture | PPM + Dictionary blend |
| 4 | CTW | Context Tree Weighting |

Note: ID 1 is unused (historical). IDs 5+ are available for external LMs.

### LM-Specific Parameters

Each LM exposes relevant tuning parameters via `dasher_get_language_model_param_count/key`:

| LM | Parameters |
|----|------------|
| PPM | `LP_LM_ALPHA`, `LP_LM_BETA`, `LP_LM_MAX_ORDER`, `LP_LM_EXCLUSION`, `LP_LM_UPDATE_EXCLUSION` |
| Word | `LP_LM_WORD_ALPHA`, `LP_LM_MAX_ORDER` |
| Mixture | All PPM params + `LP_LM_MIXTURE`, `LP_LM_WORD_ALPHA` |
| CTW | `LP_LM_MAX_ORDER` |

See [LM_REGISTRY.md](LM_REGISTRY.md) for details on registering custom LMs.

## Speed Control

```c
int  dasher_get_speed_percent(dasher_ctx* ctx);
void dasher_set_speed_percent(dasher_ctx* ctx, int percent);
```

- Range: 20–400 (clamped). Default: 100.
- Internally maps to `LP_MAX_BITRATE`: `bitrate = percent / 100.0 * 160`

## Parameters

DasherCore has a self-describing parameter schema with 86 parameters across three types:

| Type | Prefix | Count | Accessors |
|------|--------|-------|-----------|
| Boolean | `BP_*` | 27 | `dasher_get/set_bool_parameter` |
| Long | `LP_*` | 46 | `dasher_get/set_long_parameter` |
| String | `SP_*` | 13 | `dasher_get/set_string_parameter` |

### Generic Parameter Access

```c
int         dasher_get_bool_parameter(dasher_ctx* ctx, int key);
void        dasher_set_bool_parameter(dasher_ctx* ctx, int key, int value);
long        dasher_get_long_parameter(dasher_ctx* ctx, int key);
void        dasher_set_long_parameter(dasher_ctx* ctx, int key, long value);
const char* dasher_get_string_parameter(dasher_ctx* ctx, int key);
void        dasher_set_string_parameter(dasher_ctx* ctx, int key, const char* value);
int         dasher_find_parameter_key(const char* enum_key_name);
```

Parameter keys are defined in `src/DasherCore/Parameters.h`. Use `dasher_find_parameter_key` to look up keys by name (e.g. `"LP_LANGUAGE_MODEL_ID"`).

### Parameter Introspection

Frontends can build settings UIs dynamically:

```c
typedef struct dasher_parameter_info {
    int key;                // BP_*/LP_*/SP_* enum value
    const char* name;       // human-readable name
    const char* desc;       // human-readable description
    int type;               // 0=bool, 1=long, 2=string
    int ui_type;            // 0=none, 1=switch, 2=slider, 3=step, 4=enum, 5=textField
    long min_val;           // minimum value (numeric)
    long max_val;           // maximum value (numeric)
    long step;              // step size
    int advanced;           // 1 if advanced/expert setting
    const char* group;      // "Input", "Language", "Appearance", "Customization", "Output", "Game Mode"
    const char* subgroup;   // filter class (e.g. "CSmoothingFilter")
} dasher_parameter_info;

int dasher_get_parameter_count(void);
int dasher_get_parameter_info(int index, dasher_parameter_info* out);
```

### Enum Values

For parameters with `ui_type == 4` (enum):

```c
int         dasher_get_parameter_enum_count(int key);
const char* dasher_get_parameter_enum_name(int key, int index);
int         dasher_get_parameter_enum_value(int key, int index);
```

### String Values

For string parameters (e.g. alphabet list, palette list):

```c
int dasher_get_parameter_string_values(dasher_ctx* ctx, int key, const char** out_names, int max_out);
```

Returns count; copies up to `max_out` pointers. Pointers valid until next API call.

### Parameter Groups

Parameters are organized into groups for UI presentation:

| Group | Contents |
|-------|----------|
| Input | Input filters, control modes, smoothing, buttons |
| Language | Alphabet, language model, LM tuning, adaptive learning |
| Appearance | Font, node shape, geometry, transparency |
| Customization | Mouse line, colour palette, zoom steps, margins |
| Output | Speed, framerate, clipboard, speech |
| Game Mode | Help path drawing, distance/time settings |

### Settings Schema

Parameters are defined in `settings_manifest.json` and auto-generated into `Parameters.cpp`. Each entry includes:

- Key, storage name, type, default value
- Label and description (localizable)
- UI type hint, min/max/step
- Group and subgroup
- Tier (common / advanced / expert)
- Optional: `dependsOn`, `platformDefaults`, `enumValues`, `persistence`

## Colour Palettes

```c
int         dasher_get_palette_count(dasher_ctx* ctx);
const char* dasher_get_palette_name(dasher_ctx* ctx, int index);
const char* dasher_get_current_palette(dasher_ctx* ctx);
int         dasher_get_palette_preview_colors(dasher_ctx* ctx, int index, int* out_colors);
void        dasher_set_palette(dasher_ctx* ctx, const char* palette_name);
```

`dasher_get_palette_preview_colors` writes 4 ARGB preview colours into `out_colors` (must have room for 4 ints). Returns 0 on success.

### Appearance / dark mode (RFC 0007)

Palettes may declare an `appearance` (`light`/`dark`) and a `companion` (their opposite-appearance partner) in their XML. Use these to follow the OS light/dark preference without hardcoding name pairs per frontend.

```c
int         dasher_get_palette_appearance(dasher_ctx* ctx, int index);   // 0=unspecified, 1=light, 2=dark, -1=oor
const char* dasher_find_companion_palette(dasher_ctx* ctx, const char* palette_name); // NULL if none
int         dasher_set_appearance(dasher_ctx* ctx, int appearance);      // 1=light, 2=dark; 0 on success, -1 if no companion
```

`dasher_set_appearance` switches the current palette to its companion that matches the requested appearance; if the current palette already matches it is a no-op. If no companion is available it returns `-1` and leaves the palette unchanged (the user's explicit choice is respected). Companion lookup is bidirectional, so legacy palettes without metadata are still paired with a dark companion that names them.

Typical frontend usage on an OS appearance change:

```c
dasher_set_appearance(ctx, is_dark ? 2 : 1);
```

## Game Mode

```c
int         dasher_enter_game_mode(dasher_ctx* ctx);       // Returns 0 on success, -1 if no text
void        dasher_leave_game_mode(dasher_ctx* ctx);
int         dasher_game_mode_active(dasher_ctx* ctx);      // 1=on, 0=off
void        dasher_game_set_canvas_text(dasher_ctx* ctx, int enabled);  // Suppress canvas text
const char* dasher_game_get_target_text(dasher_ctx* ctx);  // Target sentence
int         dasher_game_get_correct_count(dasher_ctx* ctx); // Correct symbols (or -1)
int         dasher_game_get_target_length(dasher_ctx* ctx); // Total symbols (or -1)
const char* dasher_game_get_wrong_text(dasher_ctx* ctx);   // Wrong text since last correct
```

Game mode provides a typing tutor where users type a target sentence. The frontend can either:
- Let the engine render game UI on canvas (default)
- Suppress canvas text with `dasher_game_set_canvas_text(ctx, 0)` and render its own UI using the game query functions

## Localization

```c
int         dasher_set_locale(dasher_ctx* ctx, const char* locale);
const char* dasher_get_locale(dasher_ctx* ctx);
void        dasher_set_string_override(dasher_ctx* ctx, const char* key, const char* value);
const char* dasher_get_localized_string(dasher_ctx* ctx, const char* key);
```

- `dasher_set_locale` loads `Data/Strings/strings_{locale}.json`. Returns 0 on success, -1 if not found. `NULL` or `"en"` resets to English defaults.
- `dasher_set_string_override` overrides a specific translatable string by key (e.g. `"BP_DRAW_MOUSE_LINE.label"`). Pass `NULL` value to clear. Overrides take precedence over locale files.
- `dasher_get_localized_string` returns the string for a key (checks overrides first, then locale file). Returns `NULL` if not found.

### String Key Format

Keys follow the pattern: `{PARAM_KEY}.label`, `{PARAM_KEY}.description`, `{PARAM_KEY}.enum.{Enum Label}`

Examples:
- `"BP_DRAW_MOUSE_LINE.label"` → "Draw Mouse Line"
- `"LP_GEOMETRY.enum.Old Style"` → translated enum label

## Persistence

```c
void dasher_save_settings(dasher_ctx* ctx);
```

Saves current settings to the XML file specified at creation (`<user_dir>/dasher_settings.xml`).

## Frontend Integration Examples

### Swift (iOS)

```swift
var error: UnsafeMutablePointer<CChar>?
let ctx = dasher_create(bundlePath, docPath, &error)
dasher_set_screen_size(ctx, 800, 600)
dasher_set_output_callback(ctx, { eventType, text, userData in
    // inject text into UITextView
}, nil)

// In display link callback:
dasher_mouse_move(ctx, Float(touch.x), Float(touch.y))
var cmds: UnsafeMutablePointer<Int32>?; var cmdCount: Int32 = 0
var strs: UnsafeMutablePointer<UnsafePointer<CChar>?>?; var strCount: Int32 = 0
dasher_frame(ctx, Int64(Date().timeIntervalSince1970 * 1000), &cmds, &cmdCount, &strs, &strCount)
// render with UIKit/CoreGraphics
```

### Kotlin (Android)

```kotlin
val ctx = dasher_create(dataDir, userDir, null)
dasher_set_screen_size(ctx, width, height)

// In render loop:
dasher_mouse_move(ctx, x.toFloat(), y.toFloat())
val cmds = IntArray(10000)
val cmdCount = intArrayOf(0)
dasher_frame(ctx, System.currentTimeMillis(), cmds, cmdCount, null, null)
// render with Canvas API
```

## Important Notes

1. **`dasher_set_screen_size` must be called before `dasher_frame`** — it triggers engine initialization.
2. **`out_command_count` is total int count, not command count** — divide by 6 for command count.
3. **String pointers are ephemeral** — copy immediately if you need the value beyond the current API call.
4. **Localization state is global** — changing locale in one context affects all contexts (shared static state).
5. **Speed percent mapping** — 100% = `LP_MAX_BITRATE` of 160, range 20–400%.
6. **Language model ID gap** — IDs are 0, 2, 3, 4 (no ID 1). Historical.
7. **No font rendering** — text width is estimated as `characters * fontSize / 2`. Actual font metrics are the frontend's responsibility.
8. **Polygons are decomposed into line segments** — no filled polygon opcode.
9. **Fully transparent elements are skipped** — commands with alpha=0 are not emitted.
