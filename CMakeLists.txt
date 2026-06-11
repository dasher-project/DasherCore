cmake_minimum_required(VERSION 3.12)

project("DasherCore")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(HAVE_ROUND _CRT_SECURE_NO_WARNINGS)
set(CMAKE_SUPPRESS_REGENERATION true)

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
        add_executable(dasher_test
            ${CMAKE_CURRENT_LIST_DIR}/tests/test_capi.cpp
        )

        target_include_directories(dasher_test PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/src/
        )

        target_link_libraries(dasher_test PRIVATE dasher)

        # Allow setting test data directory
        if(TEST_DATA_DIR)
            target_compile_definitions(dasher_test PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
        endif()

        add_test(NAME dasher_capi_tests COMMAND dasher_test)
    endif()
endif()
