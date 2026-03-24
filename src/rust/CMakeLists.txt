cmake_minimum_required (VERSION 3.15...4.0)

if (NOT ENABLE_RUST)
    return ()
endif ()

if (ENABLE_RUST_SYSTEM)
    message (FATAL_ERROR "ENABLE_RUST_SYSTEM is not yet implemented")
endif ()

find_program (CARGO_EXECUTABLE cargo REQUIRED)
find_program (CARGO_CBINDGEN_EXECUTABLE cbindgen REQUIRED)

set (MONGOC_RUST_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/target")
set (MONGOC_RUST_LIBRARY "${MONGOC_RUST_TARGET_DIR}/debug/libmongoc_rust.so")
set (MONGOC_RUST_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/mongoc/mongoc-rust-private.h")

file (
    GLOB_RECURSE MONGOC_RUST_SOURCES
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/*.rs"
)

add_custom_command (
    OUTPUT "${MONGOC_RUST_LIBRARY}"
    COMMAND "${CARGO_EXECUTABLE}" build --target-dir "${MONGOC_RUST_TARGET_DIR}" --lib
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS
        "Cargo.toml"
        "${MONGOC_RUST_SOURCES}"
)

add_custom_command (
    OUTPUT "${MONGOC_RUST_HEADER}"
    COMMAND "${CARGO_CBINDGEN_EXECUTABLE}" --config cbindgen.toml --output "${MONGOC_RUST_HEADER}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS
        "cbindgen.toml"
        "${MONGOC_RUST_LIBRARY}"
)

add_custom_target (mongoc-rust-target DEPENDS "${MONGOC_RUST_LIBRARY}" "${MONGOC_RUST_HEADER}")

add_library (mongoc-rust INTERFACE)
add_dependencies(mongoc-rust mongoc-rust-target)
target_link_libraries (mongoc-rust INTERFACE bson::shared "${MONGOC_RUST_LIBRARY}")
target_include_directories(mongoc-rust INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/include")

message (STATUS "Building mongoc-rust... done.")
