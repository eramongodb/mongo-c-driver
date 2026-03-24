use crate::mongoc::Level;
use crate::mongoc::mongoc_log;

use std::ffi::{CString, c_char};

pub fn error(domain: &str, message: &str) {
    unsafe {
        mongoc_log(
            Level::Error,
            CString::new(domain).unwrap().as_ptr(),
            c"%s".as_ptr() as *const c_char,
            CString::new(message).unwrap().as_ptr(),
        )
    }
}
