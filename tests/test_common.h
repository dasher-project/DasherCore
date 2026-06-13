#pragma once

#include "dasher.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

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
    snprintf(tmpdir, sizeof(tmpdir), "/tmp/dasher_test_%d_%d", getpid(), counter++);
    mkdir(tmpdir, 0755);
    return dasher_create(TEST_DATA_DIR, tmpdir, nullptr);
}

struct StdoutUnbuffered {
    StdoutUnbuffered() { setvbuf(stdout, NULL, _IONBF, 0); }
};
static StdoutUnbuffered g_unbuffer;
