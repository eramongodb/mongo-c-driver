use std::ffi::c_char;

#[repr(C)]
pub enum Level {
    Error,
    _Critical,
    _Warning,
    _Message,
    _Info,
    _Debug,
    _Trace,
}

unsafe extern "C" {
    pub fn mongoc_log(log_level: Level, log_domain: *const c_char, format: *const c_char, ...);
}
