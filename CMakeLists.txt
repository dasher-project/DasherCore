cmake_minimum_required(VERSION 3.12)

project("DasherCore" VERSION 0.2.23)

###############################
# Release-version drift guard
###############################
# The DLL's FILEVERSION comes from project() VERSION above, and MSI upgrade
# correctness depends on it advancing with EVERY release (#89). Nothing else
# ties that number to the release tag, so a tag pushed without bumping this
# line would ship a DLL with the PREVIOUS version — silently reintroducing
# the stale-DLL upgrade bug this resource exists to fix. When building FROM
# a release tag, the tag must match project() VERSION exactly or the
# configure step fails:
#   - CI tag builds: GITHUB_REF=refs/tags/vX.Y.Z (works in shallow
#     checkouts where the tag object isn't fetched)
#   - local tag checkouts: `git describe --tags --exact-match`
# Non-tag builds (branches, PRs, dev trees) are unconstrained.
set(_dasher_tag_version "")
# CI tag builds: GITHUB_REF=refs/tags/vX.Y.Z. ONLY in DasherCore's own CI —
# when Dasher-Windows' installer workflow builds the submodule, GITHUB_REF is
# the WINDOWS tag (e.g. v0.1.29), which must not be compared against the
# engine version. GITHUB_REPOSITORY distinguishes the two.
if("$ENV{GITHUB_REPOSITORY}" STREQUAL "dasher-project/DasherCore"
        AND DEFINED ENV{GITHUB_REF}
        AND "$ENV{GITHUB_REF}" MATCHES "^refs/tags/v([0-9]+\\.[0-9]+\\.[0-9]+)$")
    set(_dasher_tag_version "${CMAKE_MATCH_1}")
else()
    execute_process(
        COMMAND git describe --tags --exact-match --match "v[0-9]*"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _dasher_git_tag
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _dasher_git_result
        ERROR_QUIET
        # NOTE: do NOT add OUTPUT_QUIET here — on execute_process it
        # suppresses the variable CAPTURE, not just printing; with it the
        # tag reads back empty and the guard never fires (found the hard
        # way: the mismatch test silently passed).
    )
    if(_dasher_git_result EQUAL 0 AND _dasher_git_tag MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)$")
        set(_dasher_tag_version "${CMAKE_MATCH_1}")
    endif()
endif()

if(_dasher_tag_version AND NOT _dasher_tag_version STREQUAL PROJECT_VERSION)
    message(FATAL_ERROR
        "Release tag v${_dasher_tag_version} does not match project VERSION ${PROJECT_VERSION}. "
        "Bump project(... VERSION ...) in CMakeLists.txt before tagging — the DLL FILEVERSION "
        "must advance with every release or MSI upgrades can keep stale DLLs (see #89).")
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(HAVE_ROUND _CRT_SECURE_NO_WARNINGS)
set(CMAKE_SUPPRESS_REGENERATION true)

# Put all executables and shared libraries in the same directory so that
# the dasher.dll is found by test executables at runtime on Windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###############################
# Compiler Warning Flags
###############################

# GCC / Clang (C++ only — C files don't need C++-specific flags like -Wnon-virtual-dtor)
add_compile_options(
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>>:-Wall>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>>:-Wextra>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>>:-Wshadow>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>>:-Wnon-virtual-dtor>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>>:-Wno-unused-parameter>
)
# GCC / Clang (C files — basic warnings only, no fallthrough noise in legacy UTF code)
add_compile_options(
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>>>:-Wall>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>>>:-Wno-implicit-fallthrough>
)

# MSVC
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    # C4100 (unre-ferenced formal parameter) — disabled globally. The legacy
    # codebase has hundreds of inherited virtual method overrides where
    # parameters are intentionally unused in specific subclasses. This is a
    # style issue, not a bug. The bug-finding warnings (C4701 uninitialized,
    # C4244 narrowing) remain enabled.
    $<$<CXX_COMPILER_ID:MSVC>:/wd4100>
)

###############################
# Optional: Treat warnings as errors
###############################

option(DASHER_WERROR "Treat compiler warnings as errors" OFF)
if(DASHER_WERROR)
    add_compile_options(
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
endif()

###############################
# Optional: Sanitizers (ASan + UBSan)
###############################

option(DASHER_SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
if(DASHER_SANITIZE)
    add_compile_options(
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fsanitize=address,undefined>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fno-omit-frame-pointer>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-g>
    )
    add_link_options(
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fsanitize=address,undefined>
    )
endif()

###############################
# Optional: clang-tidy
###############################

option(DASHER_ENABLE_CLANG_TIDY "Run clang-tidy during build" OFF)
if(DASHER_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set(DASHER_CLANG_TIDY_CMD "${CLANG_TIDY_EXE};--config-file=${CMAKE_CURRENT_LIST_DIR}/.clang-tidy")
    else()
        message(WARNING "DASHER_ENABLE_CLANG_TIDY is ON but clang-tidy not found")
    endif()
endif()

option(DASHER_HAVE_OWN_FILEUTILS "Set to true if you provide your own FileUtils implementation" OFF)
if(${DASHER_HAVE_OWN_FILEUTILS})
	add_compile_definitions(HAVE_OWN_FILEUTILS)
endif()

###############################
# Building pugixml Library
###############################

set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Thirdparty/pugixml" EXCLUDE_FROM_ALL)

###############################
# Code Generation: Parameters.cpp from settings_manifest.json
###############################

set(PARAMETERS_MANIFEST "${CMAKE_CURRENT_LIST_DIR}/settings_manifest.json")
set(PARAMETERS_GENERATOR "${CMAKE_CURRENT_LIST_DIR}/Scripts/generate_parameters.py")
set(PARAMETERS_OUTPUT "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/Parameters.cpp")

find_package(Python3 COMPONENTS Interpreter)

if(Python3_Interpreter_FOUND)
    add_custom_command(
        OUTPUT ${PARAMETERS_OUTPUT}
        COMMAND ${Python3_EXECUTABLE} ${PARAMETERS_GENERATOR}
        DEPENDS ${PARAMETERS_MANIFEST} ${PARAMETERS_GENERATOR}
        COMMENT "Generating Parameters.cpp from settings_manifest.json"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
else()
    message(WARNING "Python3 not found. Parameters.cpp will not be auto-regenerated from settings_manifest.json. Edit the JSON and run: python3 Scripts/generate_parameters.py")
endif()

###############################
# Building Core Dasher Library
###############################

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/src/**/*.h
)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/src/**/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/**/*.c
)

# Exclude standalone UTF test harnesses that don't compile as library code
list(REMOVE_ITEM SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/Common/Unicode/harness.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/Common/Unicode/original_harness.c"
)

ADD_library(DasherCore STATIC
	${HEADER_FILES}
	${SOURCE_FILES}
)

set_target_properties(DasherCore PROPERTIES
	POSITION_INDEPENDENT_CODE ON
)

target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/)

# Apply clang-tidy only to our own targets (not Thirdparty/pugixml)
if(DASHER_ENABLE_CLANG_TIDY AND DASHER_CLANG_TIDY_CMD)
    set_target_properties(DasherCore PROPERTIES CXX_CLANG_TIDY "${DASHER_CLANG_TIDY_CMD}")
endif()

add_dependencies(DasherCore pugixml)
target_link_libraries(DasherCore pugixml)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src/ FILES ${HEADER_FILES} ${SOURCE_FILES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DasherCore)

###############################
# C API shared library
###############################

option(BUILD_CAPI "Build the C API shared library (dasher.dll / libdasher.so)" ON)

if(BUILD_CAPI)
    add_library(dasher SHARED
        ${CMAKE_CURRENT_LIST_DIR}/src/CAPI.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/CAPI_appearance.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/CAPI_locale.cpp
    )

    target_include_directories(dasher PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/
    )

    target_link_libraries(dasher PRIVATE DasherCore)

    if(DASHER_ENABLE_CLANG_TIDY AND DASHER_CLANG_TIDY_CMD)
        set_target_properties(dasher PROPERTIES CXX_CLANG_TIDY "${DASHER_CLANG_TIDY_CMD}")
    endif()

    if(WIN32)
        target_compile_definitions(dasher PRIVATE DASHER_BUILDING)
        target_compile_options(dasher PRIVATE /EHa)

        # VERSIONINFO resource (Dasher-Windows #59): an unversioned DLL let
        # MSI upgrade heuristics keep stale copies under a new EXE; a
        # FILEVERSION makes newer-always-wins deterministic. Keep project()
        # VERSION in lockstep with the release tag.
        string(REPLACE "." ";" _dasher_ver_list ${PROJECT_VERSION})
        list(GET _dasher_ver_list 0 DASHER_VERSION_MAJOR)
        list(GET _dasher_ver_list 1 DASHER_VERSION_MINOR)
        list(GET _dasher_ver_list 2 DASHER_VERSION_PATCH)
        set(DASHER_VERSION_STRING "${PROJECT_VERSION}.0")
        configure_file(
            ${CMAKE_CURRENT_LIST_DIR}/src/dasher_version.rc.in
            ${CMAKE_CURRENT_BINARY_DIR}/dasher_version.rc
            @ONLY)
        target_sources(dasher PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/dasher_version.rc)
    endif()

    set_target_properties(dasher PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )

    # C API Tests
    option(BUILD_TESTS "Build the C API unit tests" ON)
    if(BUILD_TESTS)
        # Default test data directory (project root, so both Data/ and Strings/ are accessible)
        if(NOT DEFINED TEST_DATA_DIR)
            set(TEST_DATA_DIR "${CMAKE_CURRENT_LIST_DIR}")
        endif()

        # doctest is the test framework. Vendored as a single header in Thirdparty/.
        set(DOCTEST_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/Thirdparty/doctest")

        # Default per-test timeout. Tests routinely take 15-30s because each
        # dasher_create() loads every alphabet XML file from Data/. 300s
        # gives headroom on slow filesystems (WSL+9P, Windows CI) and for
        # Debug builds with asserts active, while still catching real hangs.
        # (CI can override via `ctest --timeout` on the command line.)
        set(DASHER_TEST_TIMEOUT 300 CACHE STRING "Per-test timeout in seconds")

        # Helper function for adding a test executable
        function(dasher_add_test name source)
            add_executable(${name} ${CMAKE_CURRENT_LIST_DIR}/tests/${source})
            target_include_directories(${name} PRIVATE
                ${CMAKE_CURRENT_LIST_DIR}/src/
                ${CMAKE_CURRENT_LIST_DIR}/tests/
                ${DOCTEST_INCLUDE_DIR})
            target_link_libraries(${name} PRIVATE dasher)
            target_compile_definitions(${name} PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
            add_test(NAME ${name} COMMAND ${name})
            set_tests_properties(${name} PROPERTIES TIMEOUT ${DASHER_TEST_TIMEOUT})
        endfunction()

        # Variant for tests that need internal DasherCore symbols (e.g. ConvertUTF)
        function(dasher_add_test_internal name source)
            add_executable(${name} ${CMAKE_CURRENT_LIST_DIR}/tests/${source})
            target_include_directories(${name} PRIVATE
                ${CMAKE_CURRENT_LIST_DIR}/src/
                ${CMAKE_CURRENT_LIST_DIR}/tests/
                ${DOCTEST_INCLUDE_DIR})
            target_link_libraries(${name} PRIVATE DasherCore pugixml)
            target_compile_definitions(${name} PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
            add_test(NAME ${name} COMMAND ${name})
            set_tests_properties(${name} PROPERTIES TIMEOUT ${DASHER_TEST_TIMEOUT})
        endfunction()

        dasher_add_test(dasher_capi_tests test_capi.cpp)
        dasher_add_test_internal(dasher_framerate_tests test_framerate.cpp)
        dasher_add_test(dasher_palette_tests test_palettes.cpp)
        # Internal: drives CColorIO/ColorPalette directly. CAPI-only tests
        # live in test_palettes.cpp (shared lib); mixing both link styles in
        # one binary duplicates every static (Settings::parameter_defaults
        # et al) and corrupts the heap at static-destruction time.
        dasher_add_test_internal(dasher_palette_alias_tests test_palette_aliases.cpp)
        target_include_directories(dasher_palette_alias_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Thirdparty/pugixml/src)
        dasher_add_test(dasher_interaction_tests test_interaction.cpp)
        dasher_add_test(dasher_direct_mode_shadow_tests test_direct_mode_shadow.cpp)
        dasher_add_test(dasher_low_memory_tests test_low_memory.cpp)
        dasher_add_test(dasher_extended_tests test_capi_extended.cpp)
        # The extended suite creates many engines (each parses the full
        # training corpus); under sanitizers that runs several times slower
        # and crossed the default 300s budget (CI Sanitize job timeout).
        # Generous headroom rather than trimming tests: coverage is the point.
        set_tests_properties(dasher_extended_tests PROPERTIES TIMEOUT 900)
dasher_add_test(dasher_text_metrics_tests test_text_metrics.cpp)
        dasher_add_test(dasher_language_model_tests test_language_models.cpp)
        dasher_add_test(dasher_parameter_tests test_parameters.cpp)
        dasher_add_test(dasher_multilingual_tests test_multilingual.cpp)
        dasher_add_test(dasher_draw_command_tests test_draw_commands.cpp)
        dasher_add_test(dasher_lifecycle_tests test_lifecycle.cpp)
        dasher_add_test(dasher_ppm_golden_tests test_ppm_golden.cpp)
        dasher_add_test(dasher_coordinates_tests test_coordinates.cpp)
        dasher_add_test(dasher_draw_snapshot_tests test_draw_snapshots.cpp)
        dasher_add_test(dasher_settings_xml_tests test_settings_xml.cpp)
        dasher_add_test(dasher_reset_settings_tests test_reset_settings.cpp)
        dasher_add_test(dasher_alphabet_xml_tests test_alphabet_xml.cpp)
        dasher_add_test(dasher_ppm_serialization_tests test_ppm_serialization.cpp)
        dasher_add_test(dasher_alphabet_map_tests test_alphabet_map.cpp)
        dasher_add_test(dasher_color_math_tests test_color_math.cpp)
        dasher_add_test_internal(dasher_utf_conversion_tests test_utf_conversion.cpp)
        dasher_add_test(dasher_deterministic_tests test_deterministic.cpp)
        dasher_add_test(dasher_training_tests test_training.cpp)
        dasher_add_test(dasher_node_tree_tests test_node_tree.cpp)
        dasher_add_test(dasher_locales_sync_tests test_locales_sync.cpp)

        # Phase B (characterization) — closes coverage gaps noted in the review.
        #   - buffer_lifetime: the C API "valid until next call" contract
        #   - lm_correctness: training, LP_UNIFORM/LP_LM_*/BP_LM_ADAPTIVE effects
        #   - view_geometry:   LP_ORIENTATION, BP_NONLINEAR_Y, LP_GEOMETRY
        #   - input_filters:   all 14 registered filters (12 previously untested)
        #   - xml_error_paths: malformed alphabet/color/settings XML
        dasher_add_test(dasher_capi_buffer_lifetime_tests test_capi_buffer_lifetime.cpp)
        # todo.md Phase 0.5 — error/return conventions, NULL-ctx tolerance,
        # process-global locale state. Characterization: pins current
        # behaviour so the Phase 4 unification is deliberate.
        dasher_add_test(dasher_capi_contracts_tests test_capi_contracts.cpp)

        # todo.md Phase 0.3 — exported C API symbol freeze (dasher_* only).
        # Linux-only: the script uses nm/bash (see header comment). Runs as
        # a ctest so the gate is enforced in CI, not by memory.
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_test(NAME dasher_symbol_freeze
                     COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/tests/check_symbols.sh $<TARGET_FILE:dasher>)
            set_tests_properties(dasher_symbol_freeze PROPERTIES TIMEOUT 30)
        endif()
        dasher_add_test(dasher_lm_correctness_tests test_lm_correctness.cpp)
        dasher_add_test(dasher_view_geometry_tests test_view_geometry.cpp)
        dasher_add_test(dasher_circle_geometry_tests test_circle_geometry.cpp)
        # Internal: links DasherCore directly to drive CircleTo/IsSpaceAroundNode.
        dasher_add_test_internal(dasher_circle_view_internal_tests test_circle_view_internal.cpp)
        dasher_add_test(dasher_cube_geometry_tests test_cube_geometry.cpp)
        dasher_add_test(dasher_strand2_nodes_tests test_strand2_nodes.cpp)
        dasher_add_test(dasher_image_labels_tests test_image_labels.cpp)
        dasher_add_test(dasher_input_filter_tests test_input_filters.cpp)
        dasher_add_test(dasher_xml_error_path_tests test_xml_error_paths.cpp)

        # Phase C (deeper coverage) — end-to-end + perf + property tests.
        #   - control_actions_navigation: fires callback via actual node nav
        #   - spell_word: drives mouse via probabilities to enter children
        #   - benchmarks: dasher_frame p99 latency baseline (catches regressions)
        #   - property_invariants: normalization holds across random training
        dasher_add_test(dasher_control_actions_navigation_tests test_control_actions_navigation.cpp)
        dasher_add_test(dasher_spell_word_tests test_spell_word.cpp)
        dasher_add_test(dasher_benchmark_tests test_benchmarks.cpp)
        dasher_add_test(dasher_property_invariant_tests test_property_invariants.cpp)

        # Control action system tests — needs internal DasherCore classes (ActionRegistry)
        # AND C API functions, so we compile CAPI.cpp directly and link DasherCore
        add_executable(dasher_control_action_tests
            ${CMAKE_CURRENT_LIST_DIR}/tests/test_control_actions.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/CAPI.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/CAPI_appearance.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/CAPI_locale.cpp)
        target_include_directories(dasher_control_action_tests PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/src/
            ${CMAKE_CURRENT_LIST_DIR}/tests/
            ${DOCTEST_INCLUDE_DIR})
        target_link_libraries(dasher_control_action_tests PRIVATE DasherCore pugixml)
        target_compile_definitions(dasher_control_action_tests PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
        add_test(NAME dasher_control_action_tests COMMAND dasher_control_action_tests)
        set_tests_properties(dasher_control_action_tests PROPERTIES TIMEOUT ${DASHER_TEST_TIMEOUT})
    endif()
endif()
