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
#define DASHER_API __declspec(dllexport)
#else
#define DASHER_API __attribute__((visibility("default")))
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
DASHER_API dasher_ctx* dasher_create(const char* data_dir, const char* user_dir, char** out_error);

// Destroy a session and free all resources.
DASHER_API void dasher_destroy(dasher_ctx* ctx);

// Enable low-memory mode for memory-constrained environments (e.g. iOS
// keyboard extensions with ~77MB jetsam limit). Loads only the selected
// alphabet and creates only the default input filter. Must be called
// before dasher_set_screen_size().
DASHER_API void dasher_set_low_memory_mode(dasher_ctx* ctx, int enabled);

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
DASHER_API void dasher_frame(dasher_ctx* ctx, int64_t time_ms, int** out_commands, int* out_command_count,
                             char*** out_strings, int* out_string_count);

// Get/set output text (characters entered so far).
// Returned pointer is valid until the next API call on this context.
DASHER_API const char* dasher_get_output_text(dasher_ctx* ctx);
DASHER_API void dasher_reset_output_text(dasher_ctx* ctx);
DASHER_API void dasher_reset(dasher_ctx* ctx);

// Get/set current alphabet (e.g. "English with limited punctuation").
// Returned pointer is valid until the next API call on this context.
DASHER_API const char* dasher_get_alphabet_id(dasher_ctx* ctx);
DASHER_API void dasher_set_alphabet_id(dasher_ctx* ctx, const char* alphabet_id);

// Get/set language model ID (0=default PPM, 2=bigram, 3=word, 4=mixed, 5=Japanese).
DASHER_API int dasher_get_language_model_id(dasher_ctx* ctx);
DASHER_API void dasher_set_language_model_id(dasher_ctx* ctx, int model_id);

// Get available language models (from LMRegistry).
// Models are registered at startup; external LMs (KenLM, ONNX, etc.)
// can be added by calling registerLM before dasher_create.
DASHER_API int dasher_get_language_model_count(void);
DASHER_API int dasher_get_language_model_id_at(int index);
DASHER_API const char* dasher_get_language_model_name(int id);
DASHER_API const char* dasher_get_language_model_description(int id);

// Get the parameter keys relevant to a specific language model.
// Used by frontends to show/hide LM-specific settings (alpha, beta, etc).
DASHER_API int dasher_get_language_model_param_count(int id);
DASHER_API int dasher_get_language_model_param_key(int id, int index);

// Look up a parameter key by its enum name (e.g. "LP_LANGUAGE_MODEL_ID").
// Returns -1 if not found.
DASHER_API int dasher_find_parameter_key(const char* enum_key_name);

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
    int key;              // BP_*/LP_*/SP_* enum value
    const char* name;     // human-readable name (valid until next call)
    const char* desc;     // human-readable description (valid until next call)
    int type;             // parameter type (0=bool, 1=long, 2=string)
    int ui_type;          // suggested UI control type
    long min_val;         // minimum value (for numeric controls)
    long max_val;         // maximum value (for numeric controls)
    long step;            // step size (for step/slider controls)
    int advanced;         // 1 if this is an advanced setting
    const char* group;    // category group ("Input", "Language", "Appearance", "Speed", "Output")
    const char* subgroup; // input filter class (e.g. "CSmoothingFilter") for contextual UI
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

// Enable or disable canvas text rendering in game mode.
// Pass 0 to suppress the on-canvas target/wrong text (when the platform
// renders its own game UI). Pass 1 to re-enable.
DASHER_API void dasher_game_set_canvas_text(dasher_ctx* ctx, int enabled);

// Get the game mode target text (the sentence the user should type).
// Returned pointer is valid until the next API call. Returns "" if not in game mode.
DASHER_API const char* dasher_game_get_target_text(dasher_ctx* ctx);

// Get the number of correct symbols typed so far in the current game chunk.
// Returns -1 if not in game mode.
DASHER_API int dasher_game_get_correct_count(dasher_ctx* ctx);

// Get the total number of symbols in the current target text.
// Returns -1 if not in game mode.
DASHER_API int dasher_game_get_target_length(dasher_ctx* ctx);

// Get any wrong text entered since the last correct symbol.
// Returned pointer is valid until the next API call. Returns "" if not in game mode.
DASHER_API const char* dasher_game_get_wrong_text(dasher_ctx* ctx);

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

// ── Speech callback ─────────────────────────────────────────────────────────
//
// Register a callback for DasherCore's built-in speech features.
// When SupportsSpeech() returns true, DasherCore will call this to:
//   - Speak on stop (BP_SPEAK_ALL_ON_STOP)
//   - Speak words on space (BP_SPEAK_WORDS)
//   - Alphabet TTS actions (fixedTTS, repeatTTS, contextTTS, stopTTS)
//
// interrupt: 1 to interrupt current speech, 0 to queue

typedef void (*dasher_speak_callback)(const char* text, int interrupt, void* user_data);

DASHER_API void dasher_set_speak_callback(dasher_ctx* ctx, dasher_speak_callback callback, void* user_data);

// ── Clipboard callback ───────────────────────────────────────────────────────
//
// Register a callback for DasherCore's clipboard features.
// When SupportsClipboard() returns true, DasherCore will call this to:
//   - Copy on stop (BP_COPY_ALL_ON_STOP)
//   - Copy actions from alphabet nodes (copyToClipboardAction)
//
// The callback fires on the thread that calls dasher_frame().

typedef void (*dasher_clipboard_callback)(const char* text, void* user_data);

DASHER_API void dasher_set_clipboard_callback(dasher_ctx* ctx, dasher_clipboard_callback callback, void* user_data);

// ── Parameter change callback ───────────────────────────────────────────────
//
// Register a callback that fires whenever a DasherCore parameter changes.
// This includes changes made by the engine itself (e.g. AutoSpeedControl
// adjusting LP_MAX_BITRATE) as well as user-initiated changes.
//
// The callback receives the parameter key (same indices used by
// dasher_get_long_parameter / dasher_set_bool_parameter etc.).
// Use dasher_get_parameter_info() to look up the parameter name.
//
// The callback fires on the thread that calls dasher_frame() or any
// API method that changes a parameter.
//
// Key constants:
//   LP_MAX_BITRATE = dasher_find_parameter_key("LP_MAX_BITRATE")
//   BP_AUTO_SPEEDCONTROL = 14

typedef void (*dasher_parameter_callback)(int parameter_key, void* user_data);

DASHER_API void dasher_set_parameter_callback(dasher_ctx* ctx, dasher_parameter_callback callback, void* user_data);

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

// ── Custom actions ─────────────────────────────────────────────────────────
//
// Register a custom action type that can be referenced from control.xml.
// When a control node containing <name key="value" .../> is entered, the
// callback fires with the action name and all XML attributes as parallel
// key/value arrays.
//
// Must be called BEFORE dasher_set_screen_size() for the action to be
// available during initial control.xml parsing. If called after, the action
// will be registered but existing parsed nodes won't include it until the
// control box is rebuilt (e.g. by toggling BP_CONTROL_MODE).
//
// Example control.xml usage:
//   <node label="API">
//     <my_action endpoint="/api/save" method="POST"/>
//   </node>

typedef void (*dasher_action_callback)(const char* name, int attr_count, const char** attr_keys,
                                       const char** attr_values, void* user_data);

DASHER_API void dasher_register_action(dasher_ctx* ctx, const char* name, dasher_action_callback callback,
                                       void* user_data);

// ── Test / diagnostic hooks ────────────────────────────────────────────────
//
// These functions expose internal engine state for testing and golden-output
// validation. They are NOT intended for use in production frontends — the data
// structures they expose may change between versions. Frontends should use the
// public API above. These exist so that a rewrite (e.g. Rust) can be validated
// against the exact same engine state.

// Get the probability distribution of the node under the crosshair.
// Returns the number of children written. Each child gets two entries in
// out_lbnds/out_hbnds: cumulative lower/upper probability bound, normalized
// to 65536 (=1<<16). The probability of child i is hbnds[i]-lbnds[i].
// Returns -1 if the engine is not realized.
DASHER_API int dasher_get_probabilities(dasher_ctx* ctx, int* out_lbnds, int* out_hbnds, int max_out);

// Convert screen pixel coordinates to Dasher internal coordinates.
// Returns 0 on success, -1 if not realized.
DASHER_API int dasher_screen_to_dasher(dasher_ctx* ctx, int sx, int sy, long long* out_dx, long long* out_dy);

// Convert Dasher internal coordinates to screen pixel coordinates.
// Returns 0 on success, -1 if not realized.
DASHER_API int dasher_dasher_to_screen(dasher_ctx* ctx, long long dx, long long dy, int* out_sx, int* out_sy);

// Get the number of children of the node currently under the crosshair.
// Returns -1 if not realized.
DASHER_API int dasher_get_root_child_count(dasher_ctx* ctx);

// Get probability bounds for a specific child of the crosshair node.
// Returns 0 on success, -1 if index out of range or not realized.
DASHER_API int dasher_get_root_child_bounds(dasher_ctx* ctx, int index, long long* out_lbnd, long long* out_hbnd);

// Get the number of symbols in the active alphabet.
// Returns -1 if not realized.
DASHER_API int dasher_get_alphabet_symbol_count(dasher_ctx* ctx);

// Get the display text for an alphabet symbol at the given index.
// Returns 0 on success, -1 if out of range. out_text is NUL-terminated.
DASHER_API int dasher_get_alphabet_symbol_text(dasher_ctx* ctx, int index, char* out_text, int max_len);

// Import custom training text into the language model.
// This enables deterministic testing with known training data.
// Returns 0 on success, -1 on failure.
DASHER_API int dasher_import_training_text(dasher_ctx* ctx, const char* text);

// Get the current Dasher offset (character position in the output).
// Returns -1 if not realized.
DASHER_API int dasher_get_offset(dasher_ctx* ctx);

#ifdef __cplusplus
}
#endif

#endif // DASHER_H
