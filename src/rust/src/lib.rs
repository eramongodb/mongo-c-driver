pub mod client;
pub mod collection;
pub mod database;
pub mod future;
pub mod log;

/// cbindgen:ignore
pub mod bson;

/// cbindgen:ignore
pub mod mongoc;

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_sanity_check(n: i32) -> i32 {
    n
}
