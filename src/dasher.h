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

// ── Error conventions ──────────────────────────────────────────────────────
//
// The API predates a unified status type, so conventions vary by return
// type. Pinned by tests/test_capi_contracts.cpp; unifying them (where it
// changes observable returns) requires a DASHER_CAPI_VERSION bump.
//
//  Status ints (0 = success):
//    0 / -1          — dasher_enter_game_mode, dasher_set_offset,
//                      dasher_seed_buffer, dasher_get_parameter_info,
//                      dasher_get_palette_preview_colors, dasher_set_locale,
//                      dasher_screen_to_dasher / dasher_dasher_to_screen,
//                      dasher_get_alphabet_symbol_text/display/image,
//                      dasher_get_viewport, dasher_get_root_child_bounds
//    -1 also means "invalid state / not realized / not in game mode" for
//                      the state-query getters (dasher_get_offset,
//                      dasher_get_probabilities, game counters, coordinate
//                      converters, Strand 2 queries).
//    counts >= 0     — list/getters return 0 for "none or invalid", never -1
//                      (dasher_get_palette_count, dasher_get_alphabet_count,
//                      dasher_get_parameter_string_values, ...).
//
//  String returns (copy before the next API call):
//    ""              — the normal "no value / error" result (most getters).
//    NULL            — dasher_get_localized_string only (no translation
//                      found — a distinct state from an empty translation;
//                      frontends use it to pick fallbacks). Everything else
//                      returns "" (unified at CAPI version 2).
//
//  Failure of the engine itself:
//    dasher_has_engine_error() returns 1 after a C++ exception escaped a
//    per-frame entry point; per-frame calls then no-op until the context
//    is destroyed and recreated (see that function's comment).

#include <stdint.h>

#ifdef _WIN32
#define DASHER_API __declspec(dllexport)
#else
#define DASHER_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ── Constants ──────────────────────────────────────────────────────────────
//
// Named values for every integer the API passes or reports. They are plain
// #defines (no ABI impact, no exported symbols) so every language binding
// can declare the same names instead of hand-copying magic numbers. Values
// are frozen: adding new values is allowed, renumbering is an ABI break.

// Draw-command opcodes — each command in the dasher_frame() buffer is 6 ints
// [opcode, a, b, c, d, argb]; see dasher_frame() for the operand meaning.
#define DASHER_CMD_CLEAR 0        // argb = background colour
#define DASHER_CMD_CIRCLE 1       // a=x, b=y, c=radius, d=1 filled / 0 outline
#define DASHER_CMD_LINE 2         // a=x1, b=y1, c=x2, d=y2
#define DASHER_CMD_RECT_OUTLINE 3 // a=x1, b=y1, c=x2, d=y2
#define DASHER_CMD_RECT_FILL 4    // a=x1, b=y1, c=x2, d=y2
#define DASHER_CMD_TEXT 5         // a=x, b=y, c=fontSize, d=stringIndex
#define DASHER_CMD_LINE_WIDTH 6   // a=lineWidth (applies to subsequent DASHER_CMD_LINE)

// Output-callback event types (dasher_set_output_callback).
#define DASHER_EVENT_OUTPUT 0       // text inserted
#define DASHER_EVENT_DELETE 1       // text removed (backspace)
#define DASHER_EVENT_BUFFER_CLEAR 2 // whole buffer discarded; resync mirrors

// Message-callback types (dasher_set_message_callback).
#define DASHER_MESSAGE_INFO 0    // non-modal; user can continue writing
#define DASHER_MESSAGE_WARNING 1 // modal; text entry paused until dismissed

// Log levels (dasher_set_log_callback, level-ordering only, not bitmasks).
#define DASHER_LOG_DEBUG 0
#define DASHER_LOG_INFO 1
#define DASHER_LOG_WARN 2
#define DASHER_LOG_ERROR 3

// Appearance modes (dasher_get/set_appearance_mode) and palette
// classification (dasher_get_palette_appearance,
// dasher_set_system_appearance input).
#define DASHER_APPEARANCE_MODE_SYSTEM 0
#define DASHER_APPEARANCE_MODE_LIGHT 1
#define DASHER_APPEARANCE_MODE_DARK 2
#define DASHER_PALETTE_APPEARANCE_UNSPECIFIED 0
#define DASHER_PALETTE_APPEARANCE_LIGHT 1
#define DASHER_PALETTE_APPEARANCE_DARK 2

// Key codes for dasher_key_event. The engine accepts the full VirtualKey
// range (buttons up to 16); these are the codes frontends commonly need.
#define DASHER_KEY_START_STOP 0 // typically Space
#define DASHER_KEY_BUTTON_1 1
#define DASHER_KEY_BUTTON_2 2
#define DASHER_KEY_BUTTON_3 3
#define DASHER_KEY_BUTTON_4 4
#define DASHER_KEY_PRIMARY 100   // typically mouse left
#define DASHER_KEY_SECONDARY 101 // typically mouse right
#define DASHER_KEY_TERTIARY 102  // typically third mouse button

// Parameter schema types (dasher_parameter_info.type).
#define DASHER_PARAM_TYPE_INVALID (-1)
#define DASHER_PARAM_TYPE_BOOL 0
#define DASHER_PARAM_TYPE_LONG 1
#define DASHER_PARAM_TYPE_STRING 2

// Suggested UI control types (dasher_parameter_info.ui_type).
#define DASHER_UI_NONE 0
#define DASHER_UI_SWITCH 1
#define DASHER_UI_SLIDER 2
#define DASHER_UI_STEP 3
#define DASHER_UI_ENUM 4
#define DASHER_UI_TEXTFIELD 5

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
// key values: DASHER_KEY_START_STOP, DASHER_KEY_BUTTON_1..4,
// DASHER_KEY_PRIMARY/SECONDARY/TERTIARY (the engine accepts the full
// VirtualKey button range 0-16).
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
//   DASHER_CMD_CLEAR        — argb = background colour
//   DASHER_CMD_CIRCLE       — a=x, b=y, c=radius, d=1 filled / 0 outline, argb
//   DASHER_CMD_LINE         — a=x1, b=y1, c=x2, d=y2, argb
//   DASHER_CMD_RECT_OUTLINE — a=x1, b=y1, c=x2, d=y2, argb
//   DASHER_CMD_RECT_FILL    — a=x1, b=y1, c=x2, d=y2, argb
//   DASHER_CMD_TEXT         — a=x, b=y, c=fontSize, d=stringIndex, argb
//   DASHER_CMD_LINE_WIDTH   — a=lineWidth (applies to subsequent DASHER_CMD_LINE commands)
//
// For DASHER_CMD_TEXT, d is an index into the strings array.
//
// LP_SHAPE_TYPE == CUBE renders through the same rect/text commands:
// each cube face is a DASHER_CMD_RECT_FILL (with a DASHER_CMD_RECT_OUTLINE
// when the node has one), the crosshair bar is a filled rectangle, and cube
// mode labels are text commands. The flat buffer carries no 3D extrusion,
// so cube mode looks like flat rectangles over this API.
//
// argb format: (alpha << 24) | (red << 16) | (green << 8) | blue
DASHER_API void dasher_frame(dasher_ctx* ctx, int64_t time_ms, int** out_commands, int* out_command_count,
                             char*** out_strings, int* out_string_count);

// Engine fault flag. Returns 1 if a C++ exception was caught at the boundary
// of dasher_frame / dasher_mouse_* / dasher_key_event, leaving the engine in
// an indeterminate state; 0 otherwise. When true, those per-frame entry points
// no-op and the frontend must stop calling them, surface an error, then
// dasher_destroy() + dasher_create() a fresh context. Not cleared by
// dasher_reset(). See RFC 0009 Amendment 2.
DASHER_API int dasher_has_engine_error(dasher_ctx* ctx);

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

// Get/set speed as a percentage (100 = raw LP_MAX_BITRATE 160). The set
// clamps to the engine's declared LP_MAX_BITRATE range (see
// dasher_get_parameter_info), not a fixed percent cap — Dasher v5 allowed raw
// 10–800 (6–500 %), and the historic 20–400 percent clamp silently truncated
// the top of that range.
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
// Parameter types (dasher_parameter_info.type): DASHER_PARAM_TYPE_BOOL /
// LONG / STRING / INVALID
// UI control types (dasher_parameter_info.ui_type): DASHER_UI_NONE /
// SWITCH / SLIDER / STEP / ENUM / TEXTFIELD

typedef struct dasher_parameter_info {
    int key;              // BP_*/LP_*/SP_* enum value
    const char* name;     // human-readable name (valid until next call)
    const char* desc;     // human-readable description (valid until next call)
    int type;             // parameter type (DASHER_PARAM_TYPE_*)
    int ui_type;          // suggested UI control type (DASHER_UI_*)
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

// ── Appearance / dark mode (RFC 0007) ─────────────────────────────────────
//
// DasherCore owns a light/dark appearance model at the C API layer so frontends
// don't each reinvent the System/Light/Dark toggle, the companion lookup, or
// the palette-preference storage. State persists to <user_dir>/
// appearance_settings.xml. The active palette (returned by
// dasher_get_current_palette) is *derived* from the mode + system input +
// preferences, so an auto-switch can never overwrite the user's explicit choice.
//
// Palettes may declare `appearance` ("light"/"dark") and a `companion` (their
// opposite-appearance partner) in their XML. Companion lookup is bidirectional,
// so legacy palettes without metadata are still paired with a dark companion
// that names them.

// Classify a palette. Returns: DASHER_PALETTE_APPEARANCE_UNSPECIFIED /
// LIGHT / DARK, or -1 if index out of range.
DASHER_API int dasher_get_palette_appearance(dasher_ctx* ctx, int index);

// Find the companion (opposite-appearance) palette for the given name.
// Bidirectional lookup. Returns the companion name (valid until the next API
// call), or NULL if the palette has no companion.
DASHER_API const char* dasher_find_companion_palette(dasher_ctx* ctx, const char* palette_name);

// Appearance mode (persisted). SYSTEM follows dasher_set_system_appearance;
// LIGHT/DARK are explicit overrides. Returns DASHER_APPEARANCE_MODE_*.
DASHER_API int dasher_get_appearance_mode(dasher_ctx* ctx);
DASHER_API void dasher_set_appearance_mode(dasher_ctx* ctx, int mode);

// Transient OS appearance input (not persisted). Frontends call this when the
// OS reports a change. Consulted only when mode == SYSTEM. Input values are
// DASHER_PALETTE_APPEARANCE_LIGHT / DARK.
DASHER_API int dasher_get_system_appearance(dasher_ctx* ctx);
DASHER_API void dasher_set_system_appearance(dasher_ctx* ctx, int appearance);

// User's preferred palette for each appearance (persisted). A picker should set
// the side matching the current effective appearance (see dasher_set_user_palette).
DASHER_API const char* dasher_get_light_palette(dasher_ctx* ctx);
DASHER_API const char* dasher_get_dark_palette(dasher_ctx* ctx);
DASHER_API void dasher_set_light_palette(dasher_ctx* ctx, const char* name);
DASHER_API void dasher_set_dark_palette(dasher_ctx* ctx, const char* name);

// Convenience for the picker: sets the preference for the current effective
// appearance, and defaults the other side to the chosen palette's companion if
// that side has not been customised. (dasher_set_palette routes through this.)
DASHER_API void dasher_set_user_palette(dasher_ctx* ctx, const char* name);

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

// Re-read dasher_settings.xml and apply any changed parameters through the
// normal SetParameter path — parameter-change handlers fire (alphabet
// rebuild, colour change, input filter switch), the frontend callback
// notifies, and the edit buffer is preserved. Safe to call any time;
// only differing values are applied. Use when the settings file changed
// externally (IME service sharing the user dir, migration, etc.).
DASHER_API void dasher_reload_settings(dasher_ctx* ctx);

// Reset every parameter to its built-in default value. Fires parameter-change
// notifications so a live engine reconfigures itself (alphabet/colour/LM
// reload, etc.). Does not delete the persisted settings files — frontends that
// want persisted defaults should delete dasher_settings.xml /
// appearance_settings.xml separately before calling.
DASHER_API void dasher_reset_settings(dasher_ctx* ctx);

// ── Output callbacks ───────────────────────────────────────────────────────
//
// Register a callback to receive output/delete events in real time.
// This enables Direct Mode (text injection into other apps) and other
// reactive behaviours without polling dasher_get_output_text().
//
// Event types (DASHER_EVENT_*):
//   DASHER_EVENT_OUTPUT       — text is the string being inserted
//   DASHER_EVENT_DELETE       — text is the string being removed (backspace)
//   DASHER_EVENT_BUFFER_CLEAR — text is empty; the whole buffer was discarded
//                               (dasher_reset, dasher_reset_output_text, or an
//                               alphabet change). Deltas alone cannot express
//                               this, so subscribers maintaining a shadow
//                               buffer must treat this as "clear your copy".
//
// The callback fires on the thread that calls dasher_frame(). Event type 2
// may also fire from the thread calling the reset function itself.

typedef void (*dasher_output_callback)(int event_type, const char* text, void* user_data);

DASHER_API void dasher_set_output_callback(dasher_ctx* ctx, dasher_output_callback callback, void* user_data);

// ── Text measurement callback ──────────────────────────────────────────────
//
// The engine lays out node labels (the anti-overlap "shunting" that pushes a
// child label past its parent's right edge) using the canvas's reported text
// width. Frontends that render the command buffer draw with a real font, so
// only they know true glyph advances — an estimate compounds down the label
// chain and deep-zoom text degenerates into overlapping jumbles (issue #56).
//
// Register this callback to supply real measurements made with the SAME font
// the canvas draws text commands (DASHER_CMD_TEXT) with (see SP_DASHER_FONT and the
// per-command font size). `text` is UTF-8. Fill *out_width and *out_height in
// pixels and return 0. Return non-zero (or pass a null callback) to fall back
// to the engine's built-in estimate.
//
// - Fires on the thread that calls dasher_frame(); keep it fast (the engine
//   caches results per label and font size).
// - Only single-line labels are measured through the callback; wrapped labels
//   (e.g. the paused/lock message) always use the estimate.
// - After the canvas font family/face changes, call
//   dasher_text_metrics_changed() so cached measurements are re-queried.

typedef int (*dasher_text_size_callback)(const char* text, int font_size, int* out_width, int* out_height,
                                         void* user_data);

DASHER_API void dasher_set_text_size_callback(dasher_ctx* ctx, dasher_text_size_callback callback, void* user_data);

// Invalidate all cached text measurements. Call whenever the font the canvas
// actually draws with changes (e.g. SP_DASHER_FONT was set, or the frontend's
// font selection UI was used), so subsequent frames re-measure via the
// registered dasher_text_size_callback.
DASHER_API void dasher_text_metrics_changed(dasher_ctx* ctx);

// ── Message callback ───────────────────────────────────────────────────────
//
// Register a callback to receive engine messages (warnings, errors, info).
// These are the same messages that appear as yellow/white text on the canvas.
// Frontends can use this to display messages in native UI (alerts, toasts, etc).
//
// Message types (DASHER_MESSAGE_*):
//   DASHER_MESSAGE_INFO    — non-modal, user can continue writing
//   DASHER_MESSAGE_WARNING — modal, text entry is paused until dismissed
//
// The callback fires on the thread that calls dasher_frame() or any API method
// that triggers a message (e.g. dasher_enter_game_mode).

typedef void (*dasher_message_callback)(int message_type, const char* text, void* user_data);

DASHER_API void dasher_set_message_callback(dasher_ctx* ctx, dasher_message_callback callback, void* user_data);

// ── Log callback ───────────────────────────────────────────────────────────
//
// Register a callback to receive internal diagnostic log messages from
// the engine. This is the single channel for frontend→engine diagnostic
// logging — replaces the former CFileLogger/CBasicLog/UserLog systems.
//
// When no callback is registered, log messages are discarded (zero overhead).
// When registered, only messages at or above min_level are delivered.
//
// Log levels (DASHER_LOG_*):
//   DASHER_LOG_DEBUG   — verbose tracing (per-frame details, LM state)
//   DASHER_LOG_INFO    — normal operation (alphabet loaded, training complete)
//   DASHER_LOG_WARN    — recoverable problems (missing file, bad parameter)
//   DASHER_LOG_ERROR   — unrecoverable problems (assertion-level)
//
// The callback fires on the thread that calls dasher_frame() or any API
// method that produces a log message.

typedef void (*dasher_log_callback)(int level, const char* message, void* user_data);

DASHER_API void dasher_set_log_callback(dasher_ctx* ctx, dasher_log_callback callback, void* user_data, int min_level);

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

// Get the display label (what appears in Dasher boxes) for an alphabet symbol.
// Returns 0 on success, -1 if out of range. out_text is NUL-terminated.
DASHER_API int dasher_get_alphabet_symbol_display(dasher_ctx* ctx, int index, char* out_text, int max_len);

// Get the optional image path for an alphabet symbol (RFC 0014).
// The path is relative to the alphabet's data directory, or empty if no image.
// Returns 0 on success, -1 if out of range. out_path is NUL-terminated.
DASHER_API int dasher_get_alphabet_symbol_image(dasher_ctx* ctx, int index, char* out_path, int max_len);

// Import custom training text into the language model.
// This enables deterministic testing with known training data.
// Returns 0 on success, -1 on failure.
DASHER_API int dasher_import_training_text(dasher_ctx* ctx, const char* text);

// Absolute path of the current alphabet's user training file — the single
// file adaptive learning appends to and frontends should read/export/reset.
// Empty string when no model is realized or the alphabet has no training
// file. The file may not exist yet (nothing learned). Returned pointer is
// valid until the next API call on this context (dasher-project/DasherCore#84,
// dasher-project/Dasher-Windows#53).
DASHER_API const char* dasher_get_training_path(dasher_ctx* ctx);

// Test hook (todo.md 0.6): deterministically inject a C++ exception at a C
// API entry point. Exercises the Rule-4 boundary end to end — the exception
// is caught by the boundary guard, reported through the log callback at
// DASHER_LOG_ERROR, and the engine fault flag latches
// (dasher_has_engine_error() == 1; per-frame calls then no-op until the
// context is recreated). NOT for production frontends.
// site: one of the DASHER_FAIL_INJECT_* values; NONE disarms.
DASHER_API void dasher_test_inject_failure(dasher_ctx* ctx, int site);
#define DASHER_FAIL_INJECT_NONE 0
#define DASHER_FAIL_INJECT_FRAME 1
#define DASHER_FAIL_INJECT_MOUSE_MOVE 2
#define DASHER_FAIL_INJECT_MOUSE_DOWN 3
#define DASHER_FAIL_INJECT_MOUSE_UP 4
#define DASHER_FAIL_INJECT_KEY_EVENT 5
#define DASHER_FAIL_INJECT_REALIZE 6

// C API version, incremented when a behavioural change frontends might
// condition on lands. Frontends should gate compatibility workarounds on
// this rather than probing for symbols.
//
//   1 — startup training load scans the per-context USER data directory in
//       addition to the bundled data dir, so split-dir frontends (Android,
//       GTK, Apple) finally load learning accumulated in previous sessions
//       (DasherCore#84). A frontend that re-imports the training file after
//       create as a stopgap MUST skip that re-import at version >= 1 or the
//       text would be counted twice.
//   2 — string sentinels unified: dasher_get_language_model_name returns ""
//       (was "Unknown") for an unknown id, and dasher_find_companion_palette
//       returns "" (was NULL) when no companion exists. Frontends that
//       branched on those exact values must treat "" as the failure case.
DASHER_API int dasher_capi_version(void);
#define DASHER_CAPI_VERSION 2

// Get the current Dasher offset (character position in the output).
// Returns -1 if the engine is not realized.
DASHER_API int dasher_get_offset(dasher_ctx* ctx);

// ── Context awareness (RFC 0015) ──────────────────────────────────────────

// Convert a platform caret position into the UTF-8 byte unit used by
// dasher_set_offset / dasher_seed_buffer. utf16_offset counts UTF-16 code
// units (Windows UIA / EDIT controls). A surrogate pair counts as two
// units; an offset landing inside a pair resolves to the pair's start.
// Malformed UTF-8 degrades byte-per-byte. Returns the byte offset; negative
// and out-of-range values clamp to 0 / strlen. Returns -1 if text is NULL.
DASHER_API int dasher_byte_offset_from_utf16(const char* utf8_text, int utf16_offset);

// Codepoint-unit variant (macOS AX and GTK atspi report character counts;
// emoji count as one here, two in UTF-16). Same clamping rules.
DASHER_API int dasher_byte_offset_from_codepoints(const char* utf8_text, int codepoint_offset);

// Re-anchor the model at a buffer position (v5's SetOffset). The language
// model context becomes the edit buffer's text before the offset, so
// predictions continue from there. Use after external edits moved the caret
// within the session buffer. OFFSET IS A UTF-8 BYTE POSITION (the buffer's
// unit throughout this CAPI) — convert platform carets with
// dasher_byte_offset_from_utf16 / _from_codepoints; a value that lands
// mid-codepoint snaps down to the codepoint start rather than corrupting
// output.
// Returns 0 on success, -1 on failure.
DASHER_API int dasher_set_offset(dasher_ctx* ctx, int offset);

// Replace the edit buffer with text read from the target field (e.g. via
// UI Automation) and anchor the model at caret_offset — predictions then
// continue from text the user did not type through Dasher (RFC 0015 tier 3).
// caret_offset is a UTF-8 byte position — convert platform carets with
// dasher_byte_offset_from_utf16 / _from_codepoints (see dasher_set_offset
// for the full unit contract). Emits output event 2 (buffer
// cleared) FIRST so subscribers resync their mirrors without injecting
// (backspacing a whole field into the target would destroy the user's
// text). Typing-rate stats reset. Realizes if needed.
// Returns 0 on success, -1 on failure.
DASHER_API int dasher_seed_buffer(dasher_ctx* ctx, const char* text, int caret_offset);

// ── Typing rate (RFC 0012) ─────────────────────────────────────────────────
//
// Live typing-rate metrics from a rolling 5-second window of recent output.
// CPS = characters produced per second; WPM = CPS × 12 (the standard
// 5-character-word convention). Both return 0.0 when the user hasn't typed
// recently (the window expires after 5 seconds of inactivity).

DASHER_API double dasher_get_cps(dasher_ctx* ctx);
DASHER_API double dasher_get_wpm(dasher_ctx* ctx);

// Clear the typing-rate measurement window so CPS/WPM restart from zero.
// Frontends use this for a "reset averages" button — the user can see their
// rate from a clean starting point without restarting the engine.
// Also clears the auto-speed-controller's learned typing rate so
// BP_AUTO_SPEEDCONTROL starts adapting fresh (it re-reads LP_MAX_BITRATE
// on the next frame, so call dasher_set_speed_percent() first if you want
// to reset to a specific baseline speed).
DASHER_API void dasher_reset_cps(dasher_ctx* ctx);

// ── Custom rendering, Strand 2 (RFC 0013) ──────────────────────────────────
//
// An alternative to the int[] command buffer (Strand 1): the frontend queries
// the visible node tree each frame and renders it itself with its own graphics
// API. Use this for 3D cubes, VR/spatial layouts, custom visualisations, or
// accessibility views that the flat painter's-algorithm buffer can't express.
//
// Capture is OFF by default — call dasher_set_visible_nodes_enabled(ctx, 1)
// once at setup; then after each dasher_frame() the recorded nodes are
// available via dasher_get_visible_nodes(). Strand 1 frontends pay zero
// overhead if they never enable it.
//
// The captured node set matches exactly what the command buffer draws for the
// same frame (Strand 1/Strand 2 parity), so a frontend can mix strands.

// Per-node info. Both structs begin with struct_size, set by the caller to
// sizeof(...) before the call, so the engine can detect the ABI version and
// the structs can grow without breaking existing frontends.
typedef struct dasher_node_info {
    int struct_size;     // caller sets to sizeof(dasher_node_info)
    long long dasher_y1; // node's Dasher-Y range
    long long dasher_y2;
    int symbol;               // alphabet symbol index (-1 for group/control nodes)
    int has_children;         // 1 if this node has children
    int depth;                // tree depth from the rendered root (0 = root)
    int is_game_node;         // 1 if on the game-mode path
    int screen_x1, screen_y1; // node's clipped screen bounds
    int screen_x2, screen_y2;
    int fill_argb;    // node fill colour (from active palette)
    int outline_argb; // node outline colour
    int label_index;  // index into out_strings (-1 if no label)
} dasher_node_info;

// Viewport state for the frame.
typedef struct dasher_viewport {
    int struct_size;         // caller sets to sizeof(dasher_viewport)
    long long crosshair_x;   // crosshair Dasher X (fixed at the origin)
    long long crosshair_y;   // crosshair Dasher Y
    long long visible_min_y; // visible Dasher-Y range
    long long visible_max_y;
    int screen_width; // canvas size the engine was told via dasher_set_screen_size
    int screen_height;
} dasher_viewport;

// Enable/disable per-frame node capture. Default off. Returns 0 on success,
// -1 if ctx is null / not realised.
DASHER_API int dasher_set_visible_nodes_enabled(dasher_ctx* ctx, int enabled);

// Query the visible node tree recorded during the most recent dasher_frame().
// Nodes are written to out_nodes (up to max_nodes), depth-first (parent before
// children). Returns the number of nodes written (may exceed max_nodes — call
// with a larger buffer if so; -1 on error). out_strings/out_string_count hold
// label text; dasher_node_info.label_index indexes into out_strings.
//
// out_nodes, out_strings point into engine-owned buffers valid only until the
// next dasher_frame() or dasher_get_visible_nodes() call — copy if you need
// the data beyond the next frame. Returns -1 if capture is disabled or the
// engine is not realised.
DASHER_API int dasher_get_visible_nodes(dasher_ctx* ctx, dasher_node_info* out_nodes, int max_nodes,
                                        char*** out_strings, int* out_string_count);

// Query viewport state. Returns 0 on success, -1 on error.
DASHER_API int dasher_get_viewport(dasher_ctx* ctx, dasher_viewport* out);

#ifdef __cplusplus
}
#endif

#endif // DASHER_H
