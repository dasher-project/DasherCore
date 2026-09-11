# CAPI cleanup — tracked plan

Goal: make `src/dasher.h` / `src/CAPI.cpp` easier to use, manage, and read —
without breaking ABI or regressing behaviour.

Source review: session of 2026-09-11 (see git history / DasherCore-Review.txt).

---

## Phase 0 — regression safety net (do this FIRST, before any refactor)

- [x] 0.1 Record green baseline: `cmake --build build -j` then
      `ctest --test-dir build --output-on-failure`. Save the test list +
      pass count here:
      - date: 2026-09-11
      - tests passed: 43 / 43
- [x] 0.2 Symbol-count baseline: `nm -D build/bin/*/libdasher*.so | grep ' T ' | wc -l`
      (and `dumpbin /exports` on Windows). Every refactor step must keep the
      exported-symbol set IDENTICAL — record the count per step.
      - baseline saved: `tests/baseline_symbols.txt` (2026-09-11)
      - total exported T symbols: 1337 (incl. internal C++ — full freeze)
      - `dasher_*` C API functions: 109
- [x] 0.3 New test: exported-symbol freeze. A test (or CI script) that diffs
      the current exported symbol list against `tests/baseline_symbols.txt`.
      Fails if a symbol is removed/renamed. Adding symbols is allowed.
      - done: `tests/check_symbols.sh`, gating on the 109 `dasher_*` C API
        symbols only (internal C++ symbols deliberately NOT frozen — alarm
        fatigue), Mach-O underscore-normalized, Linux-only
      - automated: runs as ctest `dasher_symbol_freeze` in CI, not by memory
- [x] 0.4 New test: golden behaviour lock before the file split. Capture
      `test_draw_snapshots` + `test_ppm_golden` outputs (already golden-based);
      confirm they cover: frame commands, training, alphabet switch, palette
      switch. Add cases for anything uncovered — NOT for the refactor itself.
      - existing: frame0/10/mouse/output determinism + training/probability
        goldens (frame commands + training covered)
      - added: `snapshot_alphabet_switch_deterministic` +
        `snapshot_palette_switch_deterministic` (mid-session switch + input,
        hash-identical across runs) — the paths Phase 2's extraction touches
- [x] 0.5 Add missing coverage for quirks the refactor could silently break:
      - [x] shared `tlString` scratch: call two string getters back-to-back,
            assert first result isn't clobbered (extends
            `test_capi_buffer_lifetime.cpp`)
            - 3 new cases: same-getter address stability, cross-getter
              shared-buffer clobber (UB-safe ordering), stringBuf independence
      - [x] locale is process-global: two ctxs, different `dasher_set_locale`,
            document current (global) behaviour in a test so a future
            per-context fix is a deliberate change, not a surprise
            - `test_capi_contracts.cpp` "locale is process-global" (also
              covers global string overrides)
      - [x] error-sentinel conventions ("" vs NULL vs -1) — snapshot current
            return values for error inputs per function
            - `test_capi_contracts.cpp`: NULL-ctx tolerance batch,
              -1 sentinels, "" vs NULL string outliers, "Unknown" LM name,
              wrong-type parameter catches, capi-version pin
      - new suite registered: `dasher_capi_contracts_tests`
      - verified: 45/45 tests pass incl. `dasher_symbol_freeze` (109 C API
        symbols gated; the 1337 full-export freeze was rescoped during
        review — see below)

## Phase 0 review outcome (review-loop, 2026-09-11)

Two reviewer passes (7/10 → 8/10, gate met). Fixes applied: symbol freeze
automated + rescoped to the 109 `dasher_*` symbols (freezing 1228 internal
C++ exports was alarm fatigue); complete null-ctx sweep (~100 functions);
real-buffer preview pins; event-2 callback contract cases; fixed real
alphabet id ("English" is not a shipped id — AlphIO silently falls back, so
the switch test was a no-op); valid-id LM static pin; LC_ALL=C collation;
stale line refs.

- [x] 0.6 Engine-error lifecycle pin (was deferred — done 2026-09-11,
      before Phase 2 extractions, since the boundary guard now owns these
      semantics): `dasher_test_inject_failure` test hook (test/diagnostic
      section, DASHER_FAIL_INJECT_* sites: frame, mouse×3, key, Realize)
      drives a real C++ exception through the boundary guard. Contracts
      suite pins: outputs zeroed on failed frame, DASHER_LOG_ERROR via log
      callback with entry-point context, engineError latches, disarmed
      calls still no-op, dasher_reset does NOT clear, recreate clears,
      failed-Realize → retry recovers (review-P1-#77 scenario).
      - baseline updated deliberately: 109 → 110 exported C API symbols.

## Phase 1 — constants + error-convention docs (header only, zero ABI risk)

- [x] 1.1 Add `#define`/`enum` constants to `dasher.h` for every magic int:
      opcodes 0–6, output event types 0–2, message types 0/1, log levels 0–3,
      appearance modes 0/1/2, key codes (0, 1–4, 100–102), parameter types
      0/1/2, UI control types 0–5. Plain int constants — ABI-free.
      - done 2026-09-11: ~35 `DASHER_*` defines in a Constants block near the
        top of dasher.h (drawn from the engine's real VirtualKey enum, not
        just the header prose). No new exported symbols (freeze stayed 109).
- [x] 1.2 Update header prose to reference the constants instead of raw numbers.
      - all opcode/event/message/log/appearance/key/param-type/UI prose now
        uses the constant names; docs/C_API.md tables + code samples too
- [x] 1.3 Add one "error conventions" table to `dasher.h` (or docs/C_API.md):
      which functions return 0/-1, "" vs NULL, and what -1 means per getter.
      - added "Error conventions" section to dasher.h (after thread-safety
        note), cross-referenced by tests/test_capi_contracts.cpp
- [x] 1.4 Tests: compile-check constants exist and match current values
      (`static_assert`-style doctest case in `test_capi.cpp`).
      - 39 static_asserts (renumber = compile error) + runtime case
        `header_constants_match_runtime_behaviour` (frame's first opcode ==
        DASHER_CMD_CLEAR; param/ui type ranges; appearance defaults)
      - caught during writing: DASHER_MESSAGE_INFO/WARNING values initially
        transposed vs the implementation (bInterrupt?1:0) — verified correct
- [x] 1.5 Update docs/C_API.md + any frontends' notes if they hardcode numbers.
      - opcode table gained a Constant column; key table, event table,
        appearance snippets use constants
- verified: full rebuild, 45/45 ctest green, symbol freeze 109 unchanged.
  NOTE: clang-tidy not runnable locally (no compile_commands.json in any
  build dir) — relies on CI. Process note: never pipe builds through
  `head` (SIGPIPE truncated a build and left stale binaries undetected).

## Phase 2 — split CAPI.cpp into internal TUs (no exported-surface change)

Target layout (internal only; `dasher.h` unchanged, still one public header):

- [x] 2.1 `src/CAPI_internal.h` — shared `dasher_ctx` definition, guard
      helper, `log_boundary_error`, small utils (moved, not copied).
      Group ctx fields into sub-structs: `Callbacks`, `Appearance`,
      scratch buffers — opaque type, zero ABI impact.
      - done 2026-09-11 (commit e113e678 follow-up): ctx + utils moved
        VERBATIM (comment archaeology preserved); Interface now defined
        out-of-line as `dasher_ctx::Interface` in CAPI.cpp
      - done (follow-up commit): sub-struct grouping — `callbacks`
        (7 callback+userdata pairs + log level), `appearance` (RFC 0007
        model state), `scratch` (5 string-buffer families with documented
        lifetimes). Member-access rewrite was mechanical (\b-bounded sed);
        compiler-verified, 45/45, freeze 110
- [x] 2.2 Exception guard helper (single template/macro) replaces the ~25
      duplicated try/catch blocks. Behaviour identical: catch, log via
      callback if registered, latch engineError where the old code did.
      - done: `capi::guarded` (void, latch flag) + `capi::guarded_result`
        (explicit error result) in CAPI_internal.h; 15 uniform sites
        converted (~30 catch blocks deleted)
      - kept explicit (bespoke/silent semantics, commented at top of
        CAPI.cpp): create, destroy, ensure_realized_for_context,
        set_visible_nodes_enabled, get_visible_nodes, get_viewport,
        import_training_text, get_training_path
      - one diagnostic delta: reset_settings failure messages now carry a
        "dasher_reset_settings: " prefix via the shared guard (was raw
        e.what()); log text only, no pinned behaviour affected
- [x] 2.3 Extract the command-buffer screen — CommandScreen + CachingLabel +
      PointerInput (~260 lines, todo said ~450: estimate was high).
      - done 2026-09-11 as `src/CAPI_screen.h` (header-only): both classes
        are fully inline, so a .cpp would have been an empty anchor. Moved
        verbatim; utf8_codepoint_count moved with them (static→inline).
        CAPI.cpp: 2316 → 2058 lines. 45/45, freeze 110.
- [ ] 2.4 Extract `src/CAPI_appearance.cpp` — appearance model, companion
      lookup, sidecar load/save (~200 lines).
- [ ] 2.5 Extract `src/CAPI_locale.cpp` — locale, overrides, string tables.
      Delete the dead `State` enum + `(void)state` in `parseStringsJson`
      while touching it. (Replacing the hand-rolled JSON parser is Phase 5.)
- [ ] 2.6 Extract `src/CAPI_params.cpp` — parameter introspection, enum
      values, string values, LM registry accessors. Delete dead
      `s_enumEntries`.
- [ ] 2.7 Extract `src/CAPI_edit.cpp` — UTF-8 helpers (`getRange`,
      `findAfter`/`findBefore`, `clamp_caret_to_codepoint`,
      `ValidatedSequenceLength`, `byte_offset_from_count`), seed/set-offset.
- [ ] 2.8 `src/CAPI.cpp` keeps: create/destroy, frame, input events,
      game mode, callbacks registration, Strand 2, typing rate.
- [ ] 2.9 Fix the mid-file `#include` block (CAPI.cpp:154) — all includes
      move to file tops during the split.
- [ ] 2.10 Each extraction = one commit. After EACH commit: rebuild, full
      ctest, symbol-list diff from 0.2 must be empty.

## Phase 3 — dedupe + dead code (behaviour-preserving)

- [ ] 3.1 Custom-action marshalling lambda exists twice (CAPI.cpp Interface
      `GetPendingCustomActions` ~line 686 and `dasher_register_action`
      ~line 2505). Extract one shared helper.
- [ ] 3.2 Mouse-up release duplication (`dasher_set_alphabet_id`,
      `dasher_set_palette`) — one `release_mouse_if_down(ctx)` helper.
- [ ] 3.3 Tests: `test_control_actions.cpp` must still pass unchanged after
      3.1 (that's the regression guard for the lambda merge).

## Phase 4 — consistency fixes (small, test-visible — decide each one)

Each of these changes observable behaviour slightly. Do them one PR each,
with tests, NOT bundled with the refactor.

- [ ] 4.1 `dasher_get_language_model_name` returns "Unknown" on bad id but
      `..._description` returns "" — pick one ("" recommended) + test.
- [ ] 4.2 Decide NULL-vs-"" policy for string getters; fix outliers
      (`dasher_find_companion_palette` returns NULL, most others "").
      Needs a `DASHER_CAPI_VERSION` bump if frontends might branch on it.
- [ ] 4.3 Cache `GetPermittedValues` result on ctx so the indexed palette/
      alphabet name getters stop rebuilding the vector per call (622-alphabet
      loop currently rebuilds 622×). Perf-only, but add a timing/behaviour
      test that iteration returns stable, correct names.

## Phase 5 — bigger cleanups (only after 1–4 land)

- [ ] 5.1 Replace hand-rolled `parseStringsJson` with pugixml-backed XML
      strings files OR a real JSON parser. Needs a decision + migration for
      existing `Strings/strings_*.json` files. Property-test round-trip
      against the old parser first.
- [ ] 5.2 Locale/override state is process-global while setters take a ctx —
      move onto the ctx (breaking change for multi-context users; gate with
      `DASHER_CAPI_VERSION` bump).
- [ ] 5.3 Deprecate the indexed list getters (`dasher_get_palette_name(i)`
      etc.) in favour of `dasher_get_parameter_string_values`, or make them
      thin wrappers over it (after 4.3).

## Done when

- [ ] No single source file over ~800 lines in `src/`.
- [ ] Exported symbol list identical to Phase 0 baseline (additions allowed).
- [ ] Full ctest green on Linux, macOS, Windows CI.
- [ ] `dasher.h` has constants for every magic int; docs table for errors.
- [ ] Frontend repos (Dasher-Apple / GTK / Android / Windows / dasher-electron)
      still build against the header unchanged.

---

### Per-step checklist (copy into each PR/commit)

```
[ ] built clean
[ ] ctest --test-dir build --output-on-failure  (all pass)
[ ] exported symbols unchanged vs baseline_symbols.txt
[ ] no new warnings in CI for touched files
[ ] clang-tidy clean for touched files (see .clang-tidy)
```
