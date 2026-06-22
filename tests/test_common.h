// test_common.h — shared test utilities for the DasherCore test suite.
//
// Built on doctest (vendored in Thirdparty/doctest/). Each test executable
// gets its main() from DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN below — do not
// define your own main() in test files.
//
// Legacy macros (TEST, ASSERT, ASSERT_EQ, ASSERT_NEQ, ASSERT_STR_EQ) are
// preserved as compatibility wrappers so existing tests keep working. New
// tests should prefer doctest idioms directly (TEST_CASE, REQUIRE, CHECK,
// SUBCASE, etc.).
//
// Utilities provided:
//   - get_test_data_dir(): project Data/ directory (read-only inputs)
//   - create_isolated_context(): dasher_ctx with a unique per-call user dir
//   - ScopedTempDir: RAII wrapper that removes the directory on destruction
//   - run_frames(): canonical frame-stepping helper (single source of truth
//     for the time-step convention so tests stop drifting between *16 and *20)

#pragma once

// Each test executable gets its doctest main from this header. Any
// translation unit that includes test_common.h and is linked into a test
// executable will trigger the implementation. Because each test target is
// a single .cpp file that includes test_common.h once, this is safe.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "dasher.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include <sys/stat.h>
#define dasher_mkdir(path) _mkdir(path)
#define dasher_getpid() _getpid()
static inline const char* dasher_temp_dir() {
    const char* t = getenv("TEMP");
    return t ? t : ".";
}
#else
#include <sys/stat.h>
#include <unistd.h>
#define dasher_mkdir(path) mkdir(path, 0755)
#define dasher_getpid() getpid()
static inline const char* dasher_temp_dir() {
    return "/tmp";
}
#endif

// ---------------------------------------------------------------------------
// Legacy compatibility macros
//
// These map the project's previous home-grown assertions onto doctest.
// ASSERT is mapped to REQUIRE (aborts the current TEST_CASE on failure) to
// preserve the historical "stop at first failed assertion" semantics, but
// crucially only within the current TEST_CASE — every other test in the
// executable still runs. This is the single biggest debuggability win from
// the doctest migration: one failure no longer hides the rest of the suite.
// ---------------------------------------------------------------------------

#define TEST(name) TEST_CASE(#name)
// Note the extra parens around (condition): doctest's expression decomposer
// cannot split `a && b` or `a || b` into two operands (the `operator&&`/
// `operator||` overloads on its decomposition proxy are deleted to prevent
// misuse). The extra parens make it a single opaque boolean expression.
#define ASSERT(condition) REQUIRE((condition))
#define ASSERT_EQ(a, b) REQUIRE_EQ(a, b)
#define ASSERT_NEQ(a, b) REQUIRE_NE(a, b)
#define ASSERT_STR_EQ(a, b) REQUIRE_EQ(std::string(a), std::string(b))

// ---------------------------------------------------------------------------
// Test data directory (project root: provides both Data/ and Strings/)
// ---------------------------------------------------------------------------

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "./Data"
#endif

inline const char* get_test_data_dir() {
    return TEST_DATA_DIR;
}

// ---------------------------------------------------------------------------
// RAII temporary directory
//
// Every persistence-touching test should construct one of these and pass
// path() into dasher_create(). When the test ends (success or failure),
// the directory and all its contents are removed. This closes the slow
// /tmp leak that the previous create_isolated_context() had.
// ---------------------------------------------------------------------------

struct ScopedTempDir {
    std::string path;

    ScopedTempDir() {
        static int counter = 0;
        char buf[256];
        snprintf(buf, sizeof(buf), "%s/dasher_test_%d_%d",
                 dasher_temp_dir(), dasher_getpid(), counter++);
        path = buf;
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
    }

    ~ScopedTempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    ScopedTempDir(const ScopedTempDir&) = delete;
    ScopedTempDir& operator=(const ScopedTempDir&) = delete;

    const char* c_str() const { return path.c_str(); }
    operator const char*() const { return path.c_str(); }
};

// ---------------------------------------------------------------------------
// Context construction
//
// create_isolated_context() remains for backward compatibility. It leaks
// its temp dir intentionally to preserve historical behavior — prefer
// create_scoped_context() in new tests, which returns a (ctx, dir) pair
// where the dir cleans up on scope exit. (The ctx itself is still owned
// by the caller; dasher_destroy() must be called.)
// ---------------------------------------------------------------------------

inline dasher_ctx* create_isolated_context() {
    char tmpdir[256];
    static int counter = 0;
    snprintf(tmpdir, sizeof(tmpdir), "%s/dasher_test_%d_%d",
             dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(tmpdir);
    return dasher_create(TEST_DATA_DIR, tmpdir, nullptr);
}

struct ScopedContext {
    dasher_ctx* ctx;
    ScopedTempDir dir;

    ScopedContext() {
        ctx = dasher_create(TEST_DATA_DIR, dir.c_str(), nullptr);
    }
    explicit ScopedContext(int width, int height) : ScopedContext() {
        dasher_set_screen_size(ctx, width, height);
    }
    ~ScopedContext() {
        if (ctx) dasher_destroy(ctx);
    }
    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
    operator dasher_ctx*() const { return ctx; }
};

// ---------------------------------------------------------------------------
// Canonical frame-stepping helper
//
// Previously every test file had its own copy of this with subtly different
// step sizes (16ms vs 20ms) — a real divergence bug. There is now one
// definition. The defaults encode the most common pattern observed in the
// existing suite (start at 1000ms, step 16ms ≈ 60 FPS).
//
// Note: this function does NOT call dasher_set_screen_size — the caller is
// responsible for that. Most tests want 800x600; use ScopedContext(w, h).
// ---------------------------------------------------------------------------

inline void run_frames(dasher_ctx* ctx, int count,
                       int64_t start_ms = 1000, int64_t step_ms = 16) {
    for (int i = 0; i < count; ++i) {
        int* cmds = nullptr;
        int cmd_count = 0;
        char** strs = nullptr;
        int str_count = 0;
        dasher_frame(ctx, start_ms + i * step_ms,
                     &cmds, &cmd_count, &strs, &str_count);
    }
}
