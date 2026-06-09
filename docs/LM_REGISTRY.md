# Language Model Registry (LMRegistry)

## Overview

The LMRegistry is a plugin-style factory for language models in DasherCore. It replaces the hardcoded `switch` statement in `CAlphabetManager::CreateLanguageModel()` with a dynamic registry where LMs self-register.

## Architecture

### Core Classes

| File | Purpose |
|------|---------|
| `src/DasherCore/LanguageModelling/LMRegistry.h` | `LMDescriptor` struct + `LMRegistry` singleton |
| `src/DasherCore/LanguageModelling/LMRegistry.cpp` | Registry implementation + built-in LM registrations |
| `src/DasherCore/AlphabetManager.cpp` | Factory call: `LMRegistry::instance().create(...)` |

### LMDescriptor

Each language model registers a descriptor:

```cpp
struct LMDescriptor {
    int id;                    // Unique ID (used by LP_LANGUAGE_MODEL_ID parameter)
    std::string name;          // Display name (e.g. "PPM", "KenLM")
    std::string description;   // Human-readable description
    bool needsAlphabetMap;     // True if factory needs CAlphabetMap*
    bool needsAlphInfo;        // True if factory needs CAlphInfo*
    FactoryFn factory;         // Lambda: (settings, alphInfo, alphMap, numSyms) -> CLanguageModel*
};
```

### Built-in LMs

| ID | Name | Class | Notes |
|----|------|-------|-------|
| 0 | PPM | `CPPMLanguageModel` | Default. Prediction by Partial Match. |
| 2 | Word | `CWordLanguageModel` | Word-level model. |
| 3 | Mixture | `CMixtureLanguageModel` | PPM + Dictionary blend. |
| 4 | CTW | `CCTWLanguageModel` | Context Tree Weighting. |

Note: ID 1 is unused (historical). IDs 5+ are available for external LMs.

## Adding a New Language Model

### Option A: Built-in (compiled into DasherCore)

1. Create your LM class in `src/DasherCore/LanguageModelling/`:
   ```cpp
   // MyNeuralLM.h
   #pragma once
   #include "LanguageModel.h"
   namespace Dasher {
   class CMyNeuralLM : public CLanguageModel {
   public:
       CMyNeuralLM(int numSyms, const std::string& modelPath);
       Context CreateEmptyContext() override;
       Context CloneContext(Context) override;
       void ReleaseContext(Context) override;
       void EnterSymbol(Context, int) override;
       void LearnSymbol(Context, int) override;
       void GetProbs(Context, std::vector<unsigned int>&, int, int) const override;
   };
   }
   ```

2. Add a registration in `LMRegistry.cpp` (in the `StaticInit` struct):
   ```cpp
   reg.registerLM({5, "Neural", "Neural network language model", false, false,
     [](CSettingsStore*, const CAlphInfo*, const CAlphabetMap*, int n) -> CLanguageModel* {
       return new CMyNeuralLM(n, "/path/to/model.onnx");
     }});
   ```

3. Add the `.h` and `.cpp` to the build (CMake GLOB_RECURSE picks them up automatically).

### Option B: External (linked at build time by frontend)

1. Create your LM class (as above) in your own source tree.
2. Before calling `dasher_create()`, use the C++ API to register:
   ```cpp
   #include "DasherCore/LanguageModelling/LMRegistry.h"
   #include "MyKenLMLanguageModel.h"
   
   Dasher::LMRegistry::instance().registerLM({
     5, "KenLM", "KenLM n-gram model", false, false,
     [](Dasher::CSettingsStore*, const Dasher::CAlphInfo*, const Dasher::CAlphabetMap*, int n) 
     -> Dasher::CLanguageModel* {
       return new CKenLMLanguageModel(n, "kenlm.binary");
     }
   });
   ```
3. Link your LM code + dependencies (e.g. libkenlm) alongside DasherCore.

### Option C: External via C API (for non-C++ frontends)

The C API provides introspection functions to query available LMs:
```c
int count = dasher_get_language_model_count();
for (int i = 0; i < count; i++) {
    int id = dasher_get_language_model_id_at(i);
    const char* name = dasher_get_language_model_name(id);
    const char* desc = dasher_get_language_model_description(id);
}
dasher_set_language_model_id(ctx, 5);
```

Registration of new LMs via C API is not yet supported — you must use the C++ API (Option B) or compile into DasherCore (Option A).

## C API Reference

| Function | Description |
|----------|-------------|
| `dasher_get_language_model_count()` | Number of registered LMs |
| `dasher_get_language_model_id_at(index)` | Get ID by index (0..count-1) |
| `dasher_get_language_model_name(id)` | Display name for an ID |
| `dasher_get_language_model_description(id)` | Description for an ID |
| `dasher_get_language_model_id(ctx)` | Currently active LM ID |
| `dasher_set_language_model_id(ctx, id)` | Switch to a different LM |

## Frontend Integration

Frontends should call `dasher_get_language_model_count()` and iterate to populate their settings UI, rather than hardcoding the list. This ensures new LMs appear automatically.

### Example (Swift)
```swift
let count = dasher_get_language_model_count()
var models: [(id: Int, name: String)] = []
for i in 0..<count {
    let id = dasher_get_language_model_id_at(Int32(i))
    let name = String(cString: dasher_get_language_model_name(id))
    models.append((Int(id), name))
}
```

## File-based LMs

Some LMs (like KenLM) load from a binary model file. The recommended pattern is:

1. The factory function checks for a model file (e.g. `kenlm_<AlphabetID>.binary`)
2. If no file found, fall back to PPM (ID 0)
3. The frontend provides a file picker for importing model files

This is what Dasher-Mobile does — see `third_party/DasherCore/src/DasherCore/AlphabetManager.cpp` in the Dasher-Mobile repo.

## Special Cases

### Mandarin (Pinyin) and Routing Alphabets

These alphabet types override `CreateLanguageModel()` entirely in their own `AlphabetManager` subclasses (`CMandarinAlphMgr`, `CRoutingAlphMgr`). They are not affected by the registry — they always create their specific LM type.

### Fallback Behavior

If an unknown or missing LM ID is requested, the registry falls back to PPM (ID 0). If PPM is not registered (shouldn't happen), `nullptr` is returned and `CAlphabetManager` creates PPM directly.
