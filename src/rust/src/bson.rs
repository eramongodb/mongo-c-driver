use std::ffi::{c_char, c_void};

unsafe extern "C" {
    pub fn bson_malloc(size: usize) -> *mut c_void;
    pub fn bson_strndup(s: *const c_char, n: usize) -> *mut c_char;
}
