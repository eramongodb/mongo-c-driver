use crate::log;
use crate::future::{Future, FutureValue, FutureValueType};

use std::ffi::{CString, c_char};

use async_ffi::FutureExt;

use mongodb;
use mongodb::bson::doc;

/// cbindgen:ignore
const MONGOC_LOG_DOMAIN: &str = "collection";

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_collection_destroy(collection: *mut Collection) {
    if !collection.is_null() {
        unsafe { drop(Box::from_raw(collection)) }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_collection_get_name(collection: *mut Collection) -> *const c_char {
    let name = &unsafe { &*collection }.name;

    name.as_ptr() as *const c_char
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_collection_drop(collection: *mut Collection) -> bool {
    let collection = &unsafe { &*collection };

    match collection
        .runtime
        .block_on(async move { collection.coll.drop().await })
    {
        Ok(_) => true,
        Err(e) => {
            log::error(MONGOC_LOG_DOMAIN, &e.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_collection_count_documents(collection: *mut Collection) -> i64 {
    let collection = &unsafe { &*collection };

    match collection
        .runtime
        .block_on(async move { collection.coll.count_documents(doc! {}).await })
    {
        Ok(count) => count as i64,
        Err(e) => {
            log::error(MONGOC_LOG_DOMAIN, &e.to_string());
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_collection_count_documents_async<'r>(collection: *mut Collection<'r>) -> *mut Future<'r> {
    let Collection { runtime, coll, .. } = &unsafe { &*collection };

    let future = Future {
        runtime: &runtime,
        value: FutureValue::UInt64(FutureValueType {
            future: async move { coll.count_documents(doc!{}).await }.into_ffi(),
            result: None,
        }),
    };

    Box::into_raw(Box::new(future))
}

pub struct Collection<'r> {
    runtime: &'r tokio::runtime::Runtime,
    coll: mongodb::Collection<mongodb::bson::Document>,
    name: CString, // Allow `mongoc_rust_collection_get_name()` to return `*const c_char`.
}

impl<'r> Collection<'r> {
    pub fn new(
        runtime: &'r tokio::runtime::Runtime,
        coll: mongodb::Collection<mongodb::bson::Document>,
    ) -> Self {
        let name = CString::new(coll.name()).unwrap();

        Self {
            runtime,
            coll,
            name,
        }
    }
}
