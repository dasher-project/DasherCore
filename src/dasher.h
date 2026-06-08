#ifndef DASHER_H
#define DASHER_H

// Dasher C API — public interface for the DasherCore engine.
//
// DasherCore is a C++ engine that computes a zooming predictive text interface.
// This C API wraps it so that any language (C#, Kotlin, Swift, JS via WASM, Rust)
// can use Dasher without touching C++.
//
// Usage:
//   dasher_ctx* ctx = dasher_create("/path/to/data");
//   dasher_set_screen_size(ctx, 800, 600);
//   while (running) {
//       dasher_mouse_move(ctx, x, y);
//       int* cmds; int cmd_count; char** strs; int str_count;
//       dasher_frame(ctx, time_ms, &cmds, &cmd_count, &strs, &str_count);
//       // render cmds/strs with your canvas API
//       // buffers are valid until next dasher_frame() call
//   }
//   dasher_destroy(ctx);
//
// Thread safety: a dasher_ctx is NOT thread-safe. One thread per context.

#include <stdint.h>

#ifdef _WIN32
#  define DASHER_API __declspec(dllexport)
#else
#  define DASHER_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Opaque session handle.
typedef struct dasher_ctx dasher_ctx;

// Create a new Dasher session.
// data_dir: path to DasherCore's Data/ directory (alphabets, colours, training).
//   Must be readable. On mobile platforms this is the app bundle's read-only data.
// user_dir: writable directory for settings and user data. If NULL, data_dir is used.
// out_error: if not NULL, set to a human-readable error string on failure.
//   Caller must NOT free the string. Valid until next API call.
// Returns NULL on failure.
DASHER_API dasher_ctx* dasher_create(const char* data_dir, const char* user_dir,
                                      char** out_error);

// Destroy a session and free all resources.
DASHER_API void dasher_destroy(dasher_ctx* ctx);

// Tell Dasher the canvas dimensions. Call on init and when the window resizes.
DASHER_API void dasher_set_screen_size(dasher_ctx* ctx, int width, int height);

// Feed pointer coordinates. Call on mouse/touch/eyetracker movement.
// Coordinates are in pixels, origin top-left.
DASHER_API void dasher_mouse_move(dasher_ctx* ctx, float x, float y);

// Signal pointer press (starts Dasher zooming).
DASHER_API void dasher_mouse_down(dasher_ctx* ctx);

// Signal pointer release (pauses Dasher zooming).
DASHER_API void dasher_mouse_up(dasher_ctx* ctx);

// Send a key event (for switch access, keyboard, or button input).
// key values: 0=Start/Stop, 1-4=Buttons, 100=Primary, 101=Secondary, 102=Tertiary.
// pressed: 1 for key down, 0 for key up.
// The active input filter determines how keys are interpreted.
DASHER_API void dasher_key_event(dasher_ctx* ctx, int key, int pressed);

// Advance one frame and get draw commands.
//
// Returns pointers into internal buffers — valid only until the next
// dasher_frame() call on this context. Do NOT free them.
//
// Command format: each command is 6 ints: [opcode, a, b, c, d, argb]
//
//   0: Clear screen          — argb = background colour
//   1: Circle                — a=x, b=y, c=radius, d=1 filled / 0 outline, argb
//   2: Line                  — a=x1, b=y1, c=x2, d=y2, argb
//   3: Rectangle outline     — a=x1, b=y1, c=x2, d=y2, argb
//   4: Rectangle filled      — a=x1, b=y1, c=x2, d=y2, argb
//   5: Text                  — a=x, b=y, c=fontSize, d=stringIndex, argb
//
// For opcode 5, d is an index into the strings array.
//
// argb format: (alpha << 24) | (red << 16) | (green << 8) | blue
DASHER_API void dasher_frame(dasher_ctx* ctx, int64_t time_ms,
                              int** out_commands, int* out_command_count,
                              char*** out_strings, int* out_string_count);

// Get/set output text (characters entered so far).
// Returned pointer is valid until the next API call on this context.
DASHER_API const char* dasher_get_output_text(dasher_ctx* ctx);
DASHER_API void dasher_reset_output_text(dasher_ctx* ctx);

// Get/set current alphabet (e.g. "English with limited punctuation").
// Returned pointer is valid until the next API call on this context.
DASHER_API const char* dasher_get_alphabet_id(dasher_ctx* ctx);
DASHER_API void dasher_set_alphabet_id(dasher_ctx* ctx, const char* alphabet_id);

// Get/set language model ID (0=default PPM, 2=bigram, 3=word, 4=mixed, 5=Japanese).
DASHER_API int dasher_get_language_model_id(dasher_ctx* ctx);
DASHER_API void dasher_set_language_model_id(dasher_ctx* ctx, int model_id);

// Get/set speed as a percentage (100 = default, range 20-400).
DASHER_API int dasher_get_speed_percent(dasher_ctx* ctx);
DASHER_API void dasher_set_speed_percent(dasher_ctx* ctx, int percent);

// Get/set boolean parameters by enum key.
// See DasherCore/Parameters.h for BP_* enum values.
DASHER_API int dasher_get_bool_parameter(dasher_ctx* ctx, int key);
DASHER_API void dasher_set_bool_parameter(dasher_ctx* ctx, int key, int value);

// Get/set long integer parameters by enum key.
// See DasherCore/Parameters.h for LP_* enum values.
DASHER_API long dasher_get_long_parameter(dasher_ctx* ctx, int key);
DASHER_API void dasher_set_long_parameter(dasher_ctx* ctx, int key, long value);

// Get/set string parameters by enum key.
// See DasherCore/Parameters.h for SP_* enum values.
// Returned pointer is valid until the next API call on this context.
DASHER_API const char* dasher_get_string_parameter(dasher_ctx* ctx, int key);
DASHER_API void dasher_set_string_parameter(dasher_ctx* ctx, int key, const char* value);

// Color utility functions for working with ARGB colors used in draw commands.
// ARGB format: (alpha << 24) | (red << 16) | (green << 8) | blue
// All color components should be in range 0-255.

// Create an ARGB color value from individual components.
DASHER_API int dasher_color_argb(int alpha, int red, int green, int blue);

// Create an opaque RGB color value (alpha = 255).
DASHER_API int dasher_color_rgb(int red, int green, int blue);

// Extract individual color components from an ARGB value.
DASHER_API int dasher_color_get_alpha(int argb);
DASHER_API int dasher_color_get_red(int argb);
DASHER_API int dasher_color_get_green(int argb);
DASHER_API int dasher_color_get_blue(int argb);

// ── Parameter introspection ───────────────────────────────────────────────
//
// The DasherCore engine has a self-describing parameter schema. Frontends
// can use these functions to build settings UIs dynamically.
//
// Parameter types: 0=bool, 1=long, 2=string, -1=invalid
// UI control types: 0=none, 1=switch, 2=slider, 3=step, 4=enum, 5=textField

typedef struct dasher_parameter_info {
    int key;                // BP_*/LP_*/SP_* enum value
    const char* name;       // human-readable name (valid until next call)
    const char* desc;       // human-readable description (valid until next call)
    int type;               // parameter type (0=bool, 1=long, 2=string)
    int ui_type;            // suggested UI control type
    long min_val;           // minimum value (for numeric controls)
    long max_val;           // maximum value (for numeric controls)
    long step;              // step size (for step/slider controls)
    int advanced;           // 1 if this is an advanced setting
    const char* group;      // category group ("Input", "Language", "Appearance", "Speed", "Output")
    const char* subgroup;   // input filter class (e.g. "CSmoothingFilter") for contextual UI
} dasher_parameter_info;

// Returns the number of parameters in the schema.
DASHER_API int dasher_get_parameter_count(void);

// Fill out info for the parameter at the given index (0..count-1).
// Returns 0 on success, -1 if index out of range.
DASHER_API int dasher_get_parameter_info(int index, dasher_parameter_info* out);

// Get the number of enum values for a parameter (only valid for enum-type).
DASHER_API int dasher_get_parameter_enum_count(int key);

// Get the display name and integer value for an enum entry.
// name pointer is valid until the next API call.
DASHER_API const char* dasher_get_parameter_enum_name(int key, int index);
DASHER_API int dasher_get_parameter_enum_value(int key, int index);

// Get permitted string values for a string parameter (e.g. alphabet list, palette list).
// Returns the count; copies up to max_out pointers into out_names.
// Pointers are valid until the next API call.
DASHER_API int dasher_get_parameter_string_values(dasher_ctx* ctx, int key, const char** out_names, int max_out);

// ── Colour palettes ──────────────────────────────────────────────────────

// Get the number of available colour palettes.
DASHER_API int dasher_get_palette_count(dasher_ctx* ctx);

// Get the name of palette at index (0..count-1). Valid until next API call.
DASHER_API const char* dasher_get_palette_name(dasher_ctx* ctx, int index);

// Get the name of the currently active palette. Valid until next API call.
DASHER_API const char* dasher_get_current_palette(dasher_ctx* ctx);

// Get 4 ARGB preview colours for a palette. out_colors must have room for 4 ints.
// Returns 0 on success, -1 if index out of range.
DASHER_API int dasher_get_palette_preview_colors(dasher_ctx* ctx, int index, int* out_colors);

// Set the active colour palette by name.
DASHER_API void dasher_set_palette(dasher_ctx* ctx, const char* palette_name);

// ── Alphabets ─────────────────────────────────────────────────────────────

// Get the number of available alphabets.
DASHER_API int dasher_get_alphabet_count(dasher_ctx* ctx);

// Get the name of alphabet at index (0..count-1). Valid until next API call.
DASHER_API const char* dasher_get_alphabet_name(dasher_ctx* ctx, int index);

// ── Game Mode ──────────────────────────────────────────────────────────────

// Enter game mode. Returns 0 on success, -1 if no game text available.
DASHER_API int dasher_enter_game_mode(dasher_ctx* ctx);

// Leave game mode.
DASHER_API void dasher_leave_game_mode(dasher_ctx* ctx);

// Check if game mode is currently active. Returns 1 if on, 0 if off.
DASHER_API int dasher_game_mode_active(dasher_ctx* ctx);

// ── Persistence ───────────────────────────────────────────────────────────

// Save current settings to disk.
DASHER_API void dasher_save_settings(dasher_ctx* ctx);

// ── Output callbacks ───────────────────────────────────────────────────────
//
// Register a callback to receive output/delete events in real time.
// This enables Direct Mode (text injection into other apps) and other
// reactive behaviours without polling dasher_get_output_text().
//
// Event types:
//   0 = text output   — text is the string being inserted
//   1 = text delete   — text is the string being removed (backspace)
//
// The callback fires on the thread that calls dasher_frame().

typedef void (*dasher_output_callback)(int event_type, const char* text, void* user_data);

DASHER_API void dasher_set_output_callback(dasher_ctx* ctx, dasher_output_callback callback, void* user_data);

// ── Message callback ───────────────────────────────────────────────────────
//
// Register a callback to receive engine messages (warnings, errors, info).
// These are the same messages that appear as yellow/white text on the canvas.
// Frontends can use this to display messages in native UI (alerts, toasts, etc).
//
// Message types:
//   0 = informational  — non-modal, user can continue writing
//   1 = warning        — modal, text entry is paused until dismissed
//
// The callback fires on the thread that calls dasher_frame() or any API method
// that triggers a message (e.g. dasher_enter_game_mode).

typedef void (*dasher_message_callback)(int message_type, const char* text, void* user_data);

DASHER_API void dasher_set_message_callback(dasher_ctx* ctx, dasher_message_callback callback, void* user_data);

// ── Localization ──────────────────────────────────────────────────────────

// Set the active locale for parameter names, descriptions, and enum labels.
// Looks for strings_{locale}.json in the data_dir/Strings/ directory.
// Pass NULL or "en" to reset to English (built-in defaults).
// Returns 0 on success, -1 if locale file not found.
DASHER_API int dasher_set_locale(dasher_ctx* ctx, const char* locale);

// Get the currently active locale code (e.g. "en", "de", "fr").
// Returned pointer is valid until the next API call.
DASHER_API const char* dasher_get_locale(dasher_ctx* ctx);

// Override a specific translatable string by key.
// Keys are in the format "BP_DRAW_MOUSE_LINE.label",
// "BP_DRAW_MOUSE_LINE.description", or "LP_GEOMETRY.enum.Old Style".
// Overrides take precedence over locale file translations.
// Pass NULL as value to clear an override.
DASHER_API void dasher_set_string_override(dasher_ctx* ctx, const char* key, const char* value);

// Get the localized string for a key (from override or locale file).
// Returns NULL if no translation found.
// Returned pointer is valid until the next API call.
DASHER_API const char* dasher_get_localized_string(dasher_ctx* ctx, const char* key);

#ifdef __cplusplus
}
#endif

#endif // DASHER_H
