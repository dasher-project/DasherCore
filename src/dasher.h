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
// data_dir must contain DasherCore's Data/ directory (alphabets, colours, training).
// Returns NULL on failure.
DASHER_API dasher_ctx* dasher_create(const char* data_dir);

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

#ifdef __cplusplus
}
#endif

#endif // DASHER_H
