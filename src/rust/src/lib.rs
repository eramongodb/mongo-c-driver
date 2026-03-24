#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_sanity_check(n: i32) -> i32 {
    n
}
