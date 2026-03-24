use crate::database::Database;
use crate::log;

use std::ffi::CStr;
use std::ffi::c_char;

use tokio;

use mongodb;

/// cbindgen:ignore
const MONGOC_LOG_DOMAIN: &str = "client";

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_client_new(uri_string: *const c_char) -> *mut Client {
    let uri_string = unsafe { CStr::from_ptr(uri_string) }.to_str().unwrap();

    let runtime = tokio::runtime::Builder::new_current_thread()
        .enable_all() // Enable I/O drivers and timers.
        .build()
        .unwrap();
    let _guard = runtime.enter();

    match runtime.block_on(mongodb::Client::with_uri_str(uri_string)) {
        Ok(client) => Box::into_raw(Box::new(Client::new(runtime, client))),
        Err(e) => {
            log::error(MONGOC_LOG_DOMAIN, &e.to_string());
            std::ptr::null_mut()
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_client_destroy(client: *mut Client) {
    if !client.is_null() {
        unsafe { drop(Box::from_raw(client)) }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_client_get_database<'r>(
    client: *mut Client,
    name: *const c_char,
) -> *mut Database<'r> {
    let client = unsafe { &*client };
    let name = unsafe {
        assert!(!name.is_null());
        CStr::from_ptr(name)
    }
    .to_str()
    .unwrap();

    Box::into_raw(Box::new(client.database(name)))
}

pub struct Client {
    runtime: tokio::runtime::Runtime,
    client: mongodb::Client,
}

impl Client {
    fn new(runtime: tokio::runtime::Runtime, client: mongodb::Client) -> Self {
        Self { runtime, client }
    }

    fn database<'r>(&'r self, name: &str) -> Database<'r> {
        Database::new(&self.runtime, self.client.database(name))
    }
}
