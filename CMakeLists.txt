cmake_minimum_required(VERSION 3.12)

project("DasherCore")

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(HAVE_ROUND _CRT_SECURE_NO_WARNINGS)
set(CMAKE_SUPPRESS_REGENERATION true)

###############################
# Compiler Warning Flags
###############################

# GCC / Clang
add_compile_options(
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wshadow>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wnon-virtual-dtor>
)

# MSVC
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
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
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_CURRENT_LIST_DIR}/.clang-tidy")
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

# Still in the library but not actively used anymore. Also not supported on all platforms
list(REMOVE_ITEM HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/CannaConversionHelper.h")
list(REMOVE_ITEM HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/SocketInputBase.h")
list(REMOVE_ITEM HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/SocketInput.h")
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/CannaConversionHelper.cpp")
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/SocketInputBase.cpp")
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/DasherCore/SocketInput.cpp")

ADD_library(DasherCore STATIC
	${HEADER_FILES}
	${SOURCE_FILES}
)

set_target_properties(DasherCore PROPERTIES
	POSITION_INDEPENDENT_CODE ON
)

target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/)

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
    )

    target_include_directories(dasher PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/
    )

    target_link_libraries(dasher PRIVATE DasherCore)

    if(WIN32)
        target_compile_definitions(dasher PRIVATE DASHER_BUILDING)
        target_compile_options(dasher PRIVATE /EHa)
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

        # Helper function for adding a test executable
        function(dasher_add_test name source)
            add_executable(${name} ${CMAKE_CURRENT_LIST_DIR}/tests/${source})
            target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/ ${CMAKE_CURRENT_LIST_DIR}/tests/)
            target_link_libraries(${name} PRIVATE dasher)
            target_compile_definitions(${name} PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
            add_test(NAME ${name} COMMAND ${name})
        endfunction()

        # Variant for tests that need internal DasherCore symbols (e.g. ConvertUTF)
        function(dasher_add_test_internal name source)
            add_executable(${name} ${CMAKE_CURRENT_LIST_DIR}/tests/${source})
            target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/ ${CMAKE_CURRENT_LIST_DIR}/tests/)
            target_link_libraries(${name} PRIVATE DasherCore pugixml)
            target_compile_definitions(${name} PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
            add_test(NAME ${name} COMMAND ${name})
        endfunction()

        dasher_add_test(dasher_capi_tests test_capi.cpp)
        dasher_add_test(dasher_interaction_tests test_interaction.cpp)
        dasher_add_test(dasher_low_memory_tests test_low_memory.cpp)
        dasher_add_test(dasher_extended_tests test_capi_extended.cpp)
        dasher_add_test(dasher_language_model_tests test_language_models.cpp)
        dasher_add_test(dasher_parameter_tests test_parameters.cpp)
        dasher_add_test(dasher_multilingual_tests test_multilingual.cpp)
        dasher_add_test(dasher_draw_command_tests test_draw_commands.cpp)
        dasher_add_test(dasher_lifecycle_tests test_lifecycle.cpp)
        dasher_add_test(dasher_ppm_golden_tests test_ppm_golden.cpp)
        dasher_add_test(dasher_coordinates_tests test_coordinates.cpp)
        dasher_add_test(dasher_draw_snapshot_tests test_draw_snapshots.cpp)
        dasher_add_test(dasher_settings_xml_tests test_settings_xml.cpp)
        dasher_add_test(dasher_alphabet_xml_tests test_alphabet_xml.cpp)
        dasher_add_test(dasher_ppm_serialization_tests test_ppm_serialization.cpp)
        dasher_add_test(dasher_alphabet_map_tests test_alphabet_map.cpp)
        dasher_add_test(dasher_color_math_tests test_color_math.cpp)
        dasher_add_test_internal(dasher_utf_conversion_tests test_utf_conversion.cpp)
        dasher_add_test(dasher_deterministic_tests test_deterministic.cpp)
        dasher_add_test(dasher_training_tests test_training.cpp)
        dasher_add_test(dasher_node_tree_tests test_node_tree.cpp)
    endif()
endif()
