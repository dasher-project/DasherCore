# DasherCore Architecture

This document describes the internal architecture of DasherCore — the
engine, not the frontends. It targets developers who need to understand
the code flow before making changes. For the public C API contract, see
`src/dasher.h`. For the cleanup plan, see `TIER2-3-PLAN.md`.


## Component overview

```
                         dasher.h (public C API)
                              │
                    ┌─────────┴──────────┐
                    │   CAPI.cpp          │   C → C++ adapter layer
                    │   (dasher_ctx)      │   Owns one Interface + one Screen
                    └─────────┬──────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
     ┌────────┴──────┐ ┌──────┴───────┐ ┌─────┴──────┐
     │ CDasherInter- │ │ CommandScreen │ │ PointerInput│  C-API-owned
     │ faceBase      │ │ (CDasherScreen)│ │ (CDasherInput)│ implementations
     │ (god class)   │ └──────────────┘ └────────────┘
     └──────┬────────┘
            │ owns / dispatches to
   ┌────────┼────────┬────────────┬─────────────┐
   │        │        │            │             │
┌──┴──┐ ┌───┴────┐ ┌──┴──────┐ ┌───┴────┐ ┌─────┴──────┐
│Model│ │View    │ │Input    │ │Module  │ │SettingsStore│
│     │ │(Dasher │ │Filter   │ │Manager │ │(XML-backed) │
│     │ │View)   │ │         │ │        │ │             │
└──┬──┘ └────────┘ └─────────┘ └───┬────┘ └─────────────┘
   │                                │
   │ creates nodes via              │ registers:
   │                                │  - Input filters (14)
┌──┴──────────────┐                │  - Language models (5)
│NodeCreation     │                │  - Input devices
│Manager (NCM)    │                │  - Colour palettes
│  ├AlphabetMgr   │◄───────────────┘
│  ├ConvMgr       │
│  ├ControlMgr    │
│  └Trainer       │
└─────────────────┘
```

### Key types

| Type | Role | Defined in |
|---|---|---|
| `dasher_ctx` | Opaque C handle. Owns one `Interface`, one `CommandScreen`, one `PointerInput`. | `CAPI.cpp:335` |
| `CDasherInterfaceBase` | The "god class" — 21 responsibilities (lifecycle, rendering, settings, edit buffer, training, etc.). Target for Tier 3 split. | `DasherInterfaceBase.h` |
| `CDasherModel` | The node tree + scheduling logic. Holds the root node and drives zooming. | `DasherModel.h` |
| `CDasherView` | Coordinate transforms + rendering. Subclassed by `CDasherViewSquare` (the only concrete view). | `DasherView.h` |
| `CDasherScreen` | Abstract output surface. C API provides `CommandScreen` which records draw commands instead of drawing pixels. | `DasherScreen.h` |
| `CDasherInput` | Abstract input device. C API provides `PointerInput` (mouse/touch). | `DasherInput.h` |
| `CInputFilter` | Input interpretation strategy (14 registered: Normal Control, Click Mode, One/Button/Two-Button/Two-Push Dynamic, etc.). | `InputFilter.h` |
| `CNodeCreationManager` | Creates and manages the node tree. Owns the AlphabetManager, ConversionManager, ControlManager, and Trainer. | `NodeCreationManager.h` |
| `CLanguageModel` | Probability model for predicting next symbol. 5 registered: PPM, Word, Mixture, CTW, Mandarin (PPMPY). | `LanguageModel.h` |
| `CSettingsStore` | Parameter storage. Backed by `XmlSettingsStore` for persistence. | `SettingsStore.h` |


## Sequence: Startup

Triggered by `dasher_set_screen_size()` — the first call with valid
dimensions causes Realize, which is the heavy initialization.

```
dasher_set_screen_size(ctx, w, h)
  │
  ├─ First call: create CommandScreen(w, h)
  ├─ intf->ChangeScreen(screen)
  ├─ intf->Realize(now)
  │    │
  │    ├─ CAlphIO: scan Data/alphabets/alphabet.*.xml
  │    ├─ CColorIO: scan Data/colours/color.*.xml
  │    ├─ ChangeView() → create CDasherViewSquare
  │    ├─ ChangeColors() → load active palette
  │    │
  │    ├─ CreateModules()
  │    │    └─ Register 14 input filters (CDefaultFilter, CPressFilter,
  │    │       CClickFilter, COneButtonDynamicFilter, ...)
  │    │
  │    ├─ ChangeAlphabet()
  │    │    ├─ CreateNCManager()
  │    │    │    ├─ Create CAlphabetManager (loads alphabet structure)
  │    │    │    ├─ Create CLanguageModel (PPM by default)
  │    │    │    ├─ Train on bundled training_*.txt files
  │    │    │    └─ Build initial root node
  │    │    └─ SetOffset(0, true) → build root node tree
  │    │
  │    ├─ CreateInput() → create PointerInput
  │    └─ CreateInputFilter() → activate "Normal Control" filter
  │
  ├─ ctx->realized = true
  └─ Force SP_INPUT_FILTER = "Normal Control"
```

After this, the engine is ready to render frames. The C API client
should call `dasher_frame()` on each display refresh.


## Sequence: Mouse input → model movement

The C API stores pointer position on the `PointerInput` object. The
actual model movement happens inside the next `dasher_frame()` call,
not at input time.

```
dasher_mouse_move(ctx, x, y)
  └─ ctx->input->SetPosition(x, y)     [stored, not processed yet]

dasher_mouse_down(ctx)
  ├─ intf->SetBoolParameter(BP_START_MOUSE, true)
  └─ intf->KeyDown(NOW, Primary_Input)
       └─ m_pInputFilter->KeyDown(...)   [filter decides: start/stop]

dasher_mouse_up(ctx)
  └─ intf->KeyUp(NOW, Primary_Input)
       └─ m_pInputFilter->KeyUp(...)     [filter decides: pause]
```

Inside the frame (see next section), the active input filter's
`Timer()` method reads the current pointer position from
`PointerInput` and calls `CDasherModel::ScheduleZoom()` or
`OneStepTowards()` to enqueue model movement.

Different input filters interpret the same input differently:
- **Normal Control (CDefaultFilter):** continuous — pointer position
  determines zoom direction and speed
- **Click Mode (CClickFilter):** discrete — clicks zoom into a target zone
- **One/Two-Button Dynamic:** button presses steer up/down
- **Two-Push Dynamic:** timing of pushes determines direction


## Sequence: Frame render

Called every display refresh (~60 FPS). This is the hot path.

```
dasher_frame(ctx, time_ms, &cmds, &cmd_count, &strs, &str_count)
  │
  ├─ screen->BeginFrame()           [clear command buffer]
  │
  ├─ intf->NewFrame(time_ms, true)
  │    │
  │    ├─ 1. INPUT FILTER TIMER
  │    │    m_pInputFilter->Timer(time, view, input, model, &policy)
  │    │      └─ reads pointer pos → model->ScheduleZoom(x, y, time)
  │    │
  │    ├─ 2. MODEL STEP
  │    │    model->NextScheduledStep()
  │    │      └─ executes queued zoom: shifts root, creates/deletes nodes
  │    │
  │    ├─ 3. RENDER
  │    │    Redraw(time, forceRedraw, policy)
  │    │      ├─ model->RenderToView(view, policy)
  │    │      │    └─ view traverses visible nodes, emits draw commands
  │    │      ├─ gameModule->DecorateView() [if game mode active]
  │    │      └─ inputFilter->DecorateView() [mouse line, guides, etc.]
  │    │
  │    ├─ 4. POLICY APPLY
  │    │    policy.apply()
  │    │      └─ expands/contracts child nodes based on visibility
  │    │
  │    └─ 5. FINISH
  │         screen->Display() [no-op for CommandScreen]
  │
  ├─ screen->BuildStringPtrs()
  │
  └─ Return command buffer + string pointers to caller
       (valid until next dasher_frame call)
```

### Draw command format

Each frame produces an array of `int` commands. Commands are
opcode-driven, 6 ints each (except text commands which are variable):

```
[opcode] [arg1] [arg2] [arg3] [arg4] [arg5]

Opcode 0: Clear screen     [argb color] [alpha] [0] [0] [0]
Opcode 1: Circle           [center_x] [center_y] [radius] [fill_argb] [outline_argb]
Opcode 2: Rectangle (outline-only) [x1] [y1] [x2] [y2] [color]
Opcode 3: Rectangle (filled) [x1] [y1] [x2] [y2] [fill_color]
Opcode 4: Polygon          [count] [points_offset] [fill] [outline] [width]
Opcode 5: Text             [string_index] [x] [y] [font_size] [color]
Opcode 6: Polyline         [width] [color] [0] [0] [0]
```

The frontend interprets these commands and draws to its native canvas.
This abstraction is what makes DasherCore cross-platform — it never
touches pixels directly.


## Sequence: Training

Two training paths exist:

### Import training text (explicit, via C API)

```
dasher_import_training_text(ctx, "the cat sat on the mat")
  │
  ├─ Write text to {userDir}/.dasher_training_tmp.txt
  └─ intf->ImportTrainingText(tmpfile_path)
       └─ m_pNCManager->ImportTrainingText(path)
            └─ CTrainer::ParseFile(path, learn=true)
                 └─ For each character/symbol in the text:
                      ├─ LM->LearnSymbol(symbol)
                      └─ Node tree updated with new counts
```

### Adaptive learning (implicit, during normal use)

When `BP_LM_ADAPTIVE` is true (default), every symbol the user outputs
triggers `CSymbolNode::TrainSymbol()`:

```
User navigates into a symbol node
  └─ CDasherNode::Output()
       └─ CSymbolNode::Output()
            ├─ Append text to edit buffer
            └─ TrainSymbol()
                 ├─ If context changed: flush old context to training file
                 ├─ LM->LearnSymbol(current_symbol)
                 └─ Update context state
```

The "flush old context to training file" step is what caused the
CWD leak fixed in Tier 1 #5. It now writes to `{userDir}/` via
`FileUtils::WriteUserDataFile()`.


## Parameter system

Parameters are defined in `settings_manifest.json` (the canonical
source). A Python codegen (`Scripts/generate_parameters.py`) generates
`Parameters.h` (enum) and `Parameters.cpp` (default values + metadata).

Three parameter kinds:
- **BP_** (Bool Parameters): `BP_LM_ADAPTIVE`, `BP_CONTROL_MODE`, etc.
- **LP_** (Long Parameters): `LP_MAX_BITRATE`, `LP_ORIENTATION`, etc.
- **SP_** (String Parameters): `SP_ALPHABET_ID`, `SP_COLOUR_ID`, etc.

Parameters are accessed via `GetBoolParameter()`, `GetLongParameter()`,
`GetStringParameter()` on `CSettingsStore`. Changes fire the
`OnParameterChanged` event, which `CDasherInterfaceBase` listens to
and dispatches (e.g., alphabet change → `ChangeAlphabet()`, colour
change → `ChangeColors()`).

Localization strings live in `Strings/strings_*.json` (one per locale),
keyed by the parameter's `enumKeyName + ".label"` / `".description"`.


## Language model hierarchy

```
CLanguageModel (abstract)
  ├─ CPPMLanguageModel        ← base PPM (correct normalization)
  │    └─ CPPMPYLanguageModel ← Mandarin pinyin (adds routing)
  │    └─ CRoutingPPMLanguageModel ← routing-aware PPM
  ├─ CWordLanguageModel       ← word-level (uses a PPM spelling model underneath)
  ├─ CDictLanguageModel       ← dictionary-based
  ├─ CMixtureLanguageModel    ← blends two LMs (typically Word + PPM)
  └─ CCTWLanguageModel        ← context tree weighting

Registered in LMRegistry (singleton), selected via LP_LANGUAGE_MODEL_ID.
```

The `GetProbs()` contract: returns a cumulative probability array where
`probs[0] == 0` (sentinel) and `probs[N-1]` should equal the norm
(65536). PPM and CTW normalize correctly. Word and Mixture have known
leaks (see `test_property_invariants.cpp` and Tier 2 #2.5).


## Threading model

DasherCore is **single-threaded**. The C API is not thread-safe —
`dasher_ctx` must be accessed from one thread at a time. Frontends
that need multi-threading (e.g., Android IME) must marshal all
DasherCore calls to a single thread.

The `NewFrame()` reentrancy guard (`static bool bReentered`) is the
only threading protection, and it's a safety net, not a design feature.


## Test architecture

31 test executables, all using doctest (vendored in `Thirdparty/doctest/`).
Shared helpers in `tests/test_common.h`:

- `create_isolated_context()` / `ScopedContext` — per-test temp dir + context
- `run_frames()` — canonical frame-stepping helper (60 FPS default)
- `ScopedTempDir` — RAII temp directory cleanup

Three tiers of tests:
1. **Unit tests** (Phase A): alphabet map, coordinates, color math, draw commands, UTF-8
2. **Characterization tests** (Phase B): LM correctness, view geometry, input filters, XML error paths, buffer lifetime
3. **Deep coverage** (Phase C): end-to-end spelling, control-mode navigation, benchmarks, property invariants

Build: `cmake --preset debug && cmake --build build --parallel`
Test: `ctest --test-dir build --timeout 300`
