cmake_minimum_required(VERSION 3.16)

project(coco VERSION 1.0.0 LANGUAGES CXX)

option(COCO_BUILD_STATIC_LIBRARY "Build coco static library" ON)
option(COCO_BUILD_SHARED_LIBRARY "Build coco shared library" ON)
option(COCO_ENABLE_LTO           "Enable link time optimization if possible" ON)
option(COCO_WARNINGS_AS_ERRORS   "Treat warnings as errors" OFF)
option(COCO_BUILD_EXAMPLES       "Build coco examples" OFF)

file(GLOB_RECURSE COCO_HEADER_FILES "include/*.hpp")
file(GLOB_RECURSE COCO_SOURCE_FILES "src/*.cpp")

# Enable static library target.
if(COCO_BUILD_STATIC_LIBRARY)
    add_library(coco-static STATIC ${COCO_SOURCE_FILES} ${COCO_HEADER_FILES})

    # Set output name for static library.
    set_target_properties(coco-static PROPERTIES OUTPUT_NAME coco)

    # Set compile definitions for static library.
    target_compile_definitions(coco-static PUBLIC "COCO_API=")

    # Alias target for coco static library.
    add_library(coco::static ALIAS coco-static)
    add_library(coco::coco ALIAS coco-static)
    add_library(coco ALIAS coco-static)

    # Append this static library target to target list.
    list(APPEND COCO_ENABLED_TARGETS coco-static)
endif()

# Enable shared library target.
if(COCO_BUILD_SHARED_LIBRARY)
    add_library(coco-shared SHARED ${COCO_SOURCE_FILES} ${COCO_HEADER_FILES})

    # Set target properties.
    set_target_properties(
        coco-shared PROPERTIES
        OUTPUT_NAME coco
        POSITION_INDEPENDENT_CODE TRUE
        CXX_VISIBILITY_PRESET hidden
    )

    # Set compile definitions for shared library.
    target_compile_definitions(coco-shared PUBLIC "COCO_API=__attribute__((visibility(\"default\")))")

    # Alias target for coco shared library.
    if(NOT COCO_BUILD_STATIC_LIBRARY)
        add_library(coco::shared ALIAS coco-shared)
        add_library(coco::coco ALIAS coco-shared)
        add_library(coco ALIAS coco-shared)
    endif()

    # Append this static library target to target list.
    list(APPEND COCO_ENABLED_TARGETS coco-shared)
endif()

# Config targets.
foreach(COCO_TARGET ${COCO_ENABLED_TARGETS})
    # Set include directory.
    target_include_directories(${COCO_TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

    # Link system libraries.
    target_link_libraries(${COCO_TARGET} PUBLIC "uring")

    # Set C++ standard.
    target_compile_features(${COCO_TARGET} PUBLIC cxx_std_20)

    # Enable LTO.
    if(COCO_ENABLE_LTO)
        set_target_properties(${COCO_TARGET} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()

    # Compile options.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${COCO_TARGET} PRIVATE -Wall -Wextra -Wmost -pedantic -Wshadow -Wold-style-cast -Woverloaded-virtual -Wcast-align -Wunreachable-code)

        # Treat warnings as errors.
        if(COCO_WARNINGS_AS_ERRORS)
            target_compile_options(${COCO_TARGET} PRIVATE -Werror)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${COCO_TARGET} PRIVATE -Wall -Wextra -pedantic -Wshadow -Wold-style-cast -Woverloaded-virtual -Wcast-align -Wunreachable-code -Wno-class-memaccess)

        # Treat warnings as errors.
        if(COCO_WARNINGS_AS_ERRORS)
            target_compile_options(${COCO_TARGET} PRIVATE -Werror)
        endif()
    endif()
endforeach()

if(COCO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
