#pragma once

#include "dasher.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define dasher_mkdir(path) _mkdir(path)
#define dasher_getpid() _getpid()
static inline const char* dasher_temp_dir() {
    const char* t = getenv("TEMP");
    return t ? t : ".";
}
#else
#include <unistd.h>
#define dasher_mkdir(path) mkdir(path, 0755)
#define dasher_getpid() getpid()
static inline const char* dasher_temp_dir() {
    return "/tmp";
}
#endif

#define TEST(name) void test_##name()
#define ASSERT(condition)                                                                                              \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            printf("FAILED: %s\n", #condition);                                                                        \
            fflush(stdout);                                                                                            \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NEQ(a, b) ASSERT((a) != (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "./Data"
#endif

inline const char* get_test_data_dir() {
    return TEST_DATA_DIR;
}

inline dasher_ctx* create_isolated_context() {
    static int counter = 0;
    char tmpdir[256];
    snprintf(tmpdir, sizeof(tmpdir), "%s/dasher_test_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(tmpdir);
    return dasher_create(TEST_DATA_DIR, tmpdir, nullptr);
}

struct StdoutUnbuffered {
    StdoutUnbuffered() { setvbuf(stdout, NULL, _IONBF, 0); }
};
static StdoutUnbuffered g_unbuffer;
