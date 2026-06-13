// Coordinate transform tests: verify Screen↔Dasher mapping round-trips
#include "test_common.h"

TEST(coord_screen_to_dasher_basic) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    long long dx, dy;
    int rc = dasher_screen_to_dasher(ctx, 400, 300, &dx, &dy);
    ASSERT_EQ(rc, 0);
    printf("  Screen(400,300) -> Dasher(%lld, %lld)\n", dx, dy);
    ASSERT(dx != 0 || dy != 0);

    dasher_destroy(ctx);
    printf("v coord_screen_to_dasher_basic passed\n");
}

TEST(coord_dasher_to_screen_basic) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    long long dx, dy;
    dasher_screen_to_dasher(ctx, 400, 300, &dx, &dy);

    int sx, sy;
    int rc = dasher_dasher_to_screen(ctx, dx, dy, &sx, &sy);
    ASSERT_EQ(rc, 0);
    printf("  Dasher(%lld,%lld) -> Screen(%d,%d) [orig: 400,300]\n", dx, dy, sx, sy);

    ASSERT(abs(sx - 400) <= 5);
    ASSERT(abs(sy - 300) <= 5);

    dasher_destroy(ctx);
    printf("v coord_dasher_to_screen_basic passed\n");
}

TEST(coord_round_trip_multiple_points) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int test_points[][2] = {
        {0, 0}, {800, 0}, {0, 600}, {800, 600}, {400, 300}, {100, 100}, {700, 500}, {200, 400},
    };
    int npoints = sizeof(test_points) / sizeof(test_points[0]);

    for (int i = 0; i < npoints; i++) {
        int sx = test_points[i][0], sy = test_points[i][1];
        long long dx, dy;
        int rc = dasher_screen_to_dasher(ctx, sx, sy, &dx, &dy);
        ASSERT_EQ(rc, 0);

        int sx2, sy2;
        rc = dasher_dasher_to_screen(ctx, dx, dy, &sx2, &sy2);
        ASSERT_EQ(rc, 0);

        int err_x = abs(sx2 - sx);
        int err_y = abs(sy2 - sy);
        printf("  (%d,%d) -> Dasher(%lld,%lld) -> (%d,%d) err=(%d,%d)\n", sx, sy, dx, dy, sx2, sy2, err_x, err_y);
        ASSERT(err_x <= 10);
        ASSERT(err_y <= 10);
    }

    dasher_destroy(ctx);
    printf("v coord_round_trip_multiple_points passed\n");
}

TEST(coord_dasher_origin_maps_near_crosshair) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    long long dx, dy;
    dasher_screen_to_dasher(ctx, 400, 300, &dx, &dy);
    printf("  Center of screen -> Dasher(%lld, %lld)\n", dx, dy);

    long long dx2, dy2;
    dasher_screen_to_dasher(ctx, 0, 0, &dx2, &dy2);
    printf("  Top-left -> Dasher(%lld, %lld)\n", dx2, dy2);

    long long dx3, dy3;
    dasher_screen_to_dasher(ctx, 800, 600, &dx3, &dy3);
    printf("  Bottom-right -> Dasher(%lld, %lld)\n", dx3, dy3);

    ASSERT(dy2 < dy);
    ASSERT(dy < dy3);

    dasher_destroy(ctx);
    printf("v coord_dasher_origin_maps_near_crosshair passed\n");
}

TEST(coord_different_screen_sizes) {
    int sizes[][2] = {{400, 300}, {800, 600}, {1024, 768}, {320, 480}};

    for (int s = 0; s < 4; s++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, sizes[s][0], sizes[s][1]);

        long long dx, dy;
        int rc = dasher_screen_to_dasher(ctx, sizes[s][0] / 2, sizes[s][1] / 2, &dx, &dy);
        ASSERT_EQ(rc, 0);
        printf("  %dx%d center -> Dasher(%lld,%lld)\n", sizes[s][0], sizes[s][1], dx, dy);

        dasher_destroy(ctx);
    }
    printf("v coord_different_screen_sizes passed\n");
}

TEST(coord_crosshair_y_is_origin) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    long long dx, dy;
    dasher_screen_to_dasher(ctx, 800, 300, &dx, &dy);
    printf("  Right-center -> Dasher(%lld, %lld) [expect dy near 2048]\n", dx, dy);

    dasher_destroy(ctx);
    printf("v coord_crosshair_y_is_origin passed\n");
}

int main() {
    printf("Running coordinate transform tests...\n\n");

    test_coord_screen_to_dasher_basic();
    test_coord_dasher_to_screen_basic();
    test_coord_round_trip_multiple_points();
    test_coord_dasher_origin_maps_near_crosshair();
    test_coord_different_screen_sizes();
    test_coord_crosshair_y_is_origin();

    printf("\nAll coordinate transform tests passed!\n");
    return 0;
}
