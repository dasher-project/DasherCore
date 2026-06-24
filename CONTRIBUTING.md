# Contributing to DasherCore

Thank you for working on DasherCore! This document describes the coding standards,
tooling, and review process for all contributions.

## Quick Start

```bash
# Clone with submodules
git clone --recursive <repo-url>

# Configure and build
cmake --preset debug
cmake --build build --parallel

# Run all 153 tests
ctest --test-dir build --output-on-failure
```

## Development Presets

| Preset | Command | Purpose |
|--------|---------|---------|
| `debug` | `cmake --preset debug` | Standard debug build |
| `release` | `cmake --preset release` | Optimized build |
| `sanitize` | `cmake --preset sanitize` | ASan + UBSan (catch memory bugs) |
| `analyze` | `cmake --preset analyze` | clang-tidy static analysis |
| `strict` | `cmake --preset strict` | Warnings-as-errors build |

## CI Pipeline

Every PR must pass **four** CI jobs before merging:

1. **Build + Test** — Matrix across Linux/macOS/Windows × GCC/Clang/MSVC × Debug/Release
2. **Sanitize** — AddressSanitizer + UndefinedBehaviorSanitizer on all tests
3. **Analyze** — clang-tidy static analysis on all source files
4. **Format Check** — clang-format dry-run on all files

If any job fails, the PR cannot merge. Fix the issue, don't bypass the check.

---

## Coding Standards

### Rule 1: No Naked `new` or `delete`

Manual memory management is banned. Use smart pointers:

```cpp
// GOOD
auto widget = std::make_unique<Widget>();
auto shared = std::make_shared<Resource>();

// BAD
Widget* w = new Widget();  // Who calls delete?
delete w;                   // Easy to miss
```

Raw pointers are allowed **only** as non-owning observers (e.g., passing a
pointer to a function that will not store or free it).

### Rule 2: `const` Everything

Mark everything that doesn't need to mutate as `const`:

```cpp
// GOOD
const int max_children = node->ChildCount();
std::string GetText() const;              // const member function
void Process(const std::string& input);   // const reference parameter

// BAD
int max = node->ChildCount();             // Not const — looks mutable
```

### Rule 3: Use `auto` Wisely

Use `auto` when the type is obvious from the right side:

```cpp
// GOOD — type is clear from initializer
auto model = ctx->intf->GetModel();
for (auto& child : children) { ... }
auto it = map.find(key);

// BAD — forces reviewer to guess
auto result = DoSomething();  // What type is this?
```

### Rule 4: Clean C API Boundary

The C API (`dasher.h` / `CAPI.cpp`) is the boundary between DasherCore and
all frontends (Swift, C#, WASM, Rust). It must be flawless:

- **Never throw exceptions across the boundary.** Every C API function must
  catch all exceptions and return an error code.
- **No C++ types cross the boundary.** Use `const char*`, `int`, `int64_t`,
  and raw structs — never `std::string` or `std::vector`.

```cpp
// GOOD
DASHER_API int dasher_get_offset(dasher_ctx* ctx) {
    if (!ctx || !ctx->realized) return -1;
    try {
        return ctx->intf->GetModel()->GetOffset();
    } catch (...) {
        return -1;
    }
}

// BAD — exception crosses extern "C"
DASHER_API int dasher_get_offset(dasher_ctx* ctx) {
    return ctx->intf->GetModel()->GetOffset();  // Can throw!
}
```

### Rule 5: Zero Compiler Warnings

Code must compile cleanly with `-Wall -Wextra` (GCC/Clang) or `/W4` (MSVC).
In CI, the `strict` preset treats warnings as errors.

If you encounter a warning in third-party or legacy code, suppress it locally
with `#pragma GCC diagnostic` — never disable warnings globally.

### Rule 6: No Premature Optimization

Write readable code first. Optimize only after profiling proves a bottleneck.

- Prefer `std::vector` over `std::list` (cache-friendly contiguous memory)
- Prefer pass-by-`const`-reference over copy for large types
- Don't inline complex logic — let the compiler decide

### Rule 7: Naming Convention for New Code

DasherCore has 25 years of accumulated naming styles. We do NOT rename
existing code (that would be a massive churn PR with zero functional
benefit). Instead, all **new** code — new classes, new functions, new
files — should follow these conventions:

| Element | Convention | Example | Notes |
|---------|-----------|---------|-------|
| Classes | `PascalCase` | `class AlphabetManager` | Not `CAlphabetManager` (the C-prefix is legacy MFC style) |
| Methods | `PascalCase` | `void GetProbs()` | Not `getProbs()` or `get_probs()` |
| Member variables | `snake_case_` (trailing underscore) | `int symbol_count_;` | Not `m_iSymbolCount` (legacy Hungarian) |
| Free functions | `snake_case` | `int get_root_child_count()` | C API functions use `dasher_` prefix |
| Constants/enums | `PascalCase` or `UPPER_SNAKE` | `NORMALIZATION`, `EDIT_OUTPUT` | Match surrounding code |
| Files | `PascalCase.h` / `PascalCase.cpp` | `AlphabetManager.h` | Not `alphabet_manager.h` |
| Namespaces | `PascalCase` | `namespace Dasher` | |

**Existing conventions you'll encounter (do not change these):**
- `CFoo` — MFC-style class prefix (~70 classes). The dominant legacy style.
- `m_iFoo`, `m_bFoo` — Hungarian-notation member prefix. Common in legacy.
- `Get_node_under_crosshair` — snake_case methods (rare, in DasherModel).
- `dasher_foo_bar()` — C API functions in `dasher.h` (frozen, do not change).

**Rule of thumb:** When you touch a file, match the file's existing style
for that edit. When you create a new file, use the conventions above.
When in doubt, ask in the PR.

---

## Testing

All new features and bug fixes must include tests. The test suite is the
backbone for a future Rust rewrite — every test captures engine behavior that
a rewrite must match.

### Test Tiers

| Tier | Files | What they verify |
|------|-------|-----------------|
| Golden | `test_ppm_golden.cpp`, `test_coordinates.cpp`, `test_draw_snapshots.cpp` | Exact probability vectors, coordinate transforms, deterministic command hashes |
| Serialization | `test_settings_xml.cpp`, `test_alphabet_xml.cpp`, `test_ppm_serialization.cpp` | Round-trip persistence, parsing correctness |
| Algorithm | `test_alphabet_map.cpp`, `test_color_math.cpp`, `test_utf_conversion.cpp` | Symbol mapping, color math, UTF-8/16/32 |
| Contract | `test_deterministic.cpp`, `test_training.cpp`, `test_node_tree.cpp` | Same-input-same-output, training adaptation, tree structure |

### Adding a New Test

1. Create `tests/test_<your_feature>.cpp`
2. Include `test_common.h` and use the `TEST()` / `ASSERT()` macros
3. Register in `CMakeLists.txt` via `dasher_add_test()` (or `dasher_add_test_internal()` if you need internal headers)
4. Run: `ctest --test-dir build --output-on-failure`

### CAPI Test Hooks

If you need to expose internal engine state for testing, add a diagnostic
function to `dasher.h` / `CAPI.cpp` marked with the test/diagnostic comment
block. These are for tests only, not production frontends.

---

## Pre-commit Hooks (Optional)

Install pre-commit to run clang-format and clang-tidy before each commit:

```bash
pip install pre-commit
pre-commit install
```

---

## PR Review Checklist

Before requesting review, verify:

- [ ] All 4 CI jobs pass (Build+Test, Sanitize, Analyze, Format)
- [ ] No new compiler warnings
- [ ] No naked `new`/`delete`
- [ ] New functions are `const`-correct
- [ ] C API functions catch all exceptions
- [ ] New features have tests
- [ ] Code is clang-format clean (run `clang-format -i src/your_file.cpp`)
- [ ] Commits are signed off (DCO) — `git commit -s`
