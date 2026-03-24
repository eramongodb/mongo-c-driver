use crate::bson::{bson_malloc, bson_strndup};
use crate::collection::Collection;
use crate::future::{Future, FutureValue, FutureValueType};
use crate::log;

use std::ffi::{CStr, CString, c_char};

use async_ffi::FutureExt;

use mongodb;

/// cbindgen:ignore
const MONGOC_LOG_DOMAIN: &str = "database";

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_database_destroy(database: *mut Database) {
    if !database.is_null() {
        unsafe { drop(Box::from_raw(database)) }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_database_get_name(database: *mut Database) -> *const c_char {
    let name = &unsafe { &*database }.name;

    name.as_ptr() as *const c_char
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_database_drop(database: *mut Database) -> bool {
    let database = &unsafe { &*database };

    match database
        .runtime
        .block_on(async move { database.db.drop().await })
    {
        Ok(_) => true,
        Err(e) => {
            log::error(MONGOC_LOG_DOMAIN, &e.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_database_drop_async<'r>(
    database: *mut Database<'r>,
) -> *mut Future<'r>
{
    let Database { runtime, db, .. } = &unsafe { &*database };

    let future = Future {
        runtime: &runtime,
        value: FutureValue::Void(FutureValueType {
            future: async move { db.drop().await }.into_ffi(),
            result: None,
        }),
    };

    Box::into_raw(Box::new(future))
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_database_create_collection(
    database: *mut Database,
    name: *const c_char,
) -> *mut Collection {
    let database = &unsafe { &*database };
    let name = unsafe {
        assert!(!name.is_null());
        CStr::from_ptr(name)
    }
    .to_str()
    .unwrap();

    match database
        .runtime
        .block_on(async move { database.db.create_collection(name).await })
    {
        Ok(_) => Box::into_raw(Box::new(database.collection(name))),
        Err(e) => {
            log::error(MONGOC_LOG_DOMAIN, &e.to_string());
            std::ptr::null_mut()
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_database_get_collection_names_with_opts(
    database: *mut Database,
) -> *mut *mut c_char {
    let database = &unsafe { &*database };

    let mut names = match database
        .runtime
        .block_on(async move { database.db.list_collection_names().await })
    {
        Ok(names) => names,
        Err(e) => {
            log::error(MONGOC_LOG_DOMAIN, &e.to_string());
            return std::ptr::null_mut();
        }
    };
    names.sort(); // For `test_database_create_collection()`.
    let names = names;

    unsafe {
        let ret =
            bson_malloc(std::mem::size_of::<*mut c_char>() * (names.len() + 1)) as *mut *mut c_char;

        for (i, name) in (&names).into_iter().enumerate() {
            let bytes = name.as_bytes();
            *ret.add(i) = bson_strndup(bytes.as_ptr() as *const c_char, bytes.len());
        }

        *ret.add(names.len()) = std::ptr::null_mut();

        ret
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_database_get_collection<'r>(
    database: *mut Database<'r>,
    name: *const c_char,
) -> *mut Collection<'r> {
    let database = &unsafe { &*database };
    let name = unsafe {
        assert!(!name.is_null());
        CStr::from_ptr(name)
    }
    .to_str()
    .unwrap();

    Box::into_raw(Box::new(database.collection(name)))
}

pub struct Database<'r> {
    runtime: &'r tokio::runtime::Runtime,
    db: mongodb::Database,
    name: CString, // Allow `mongoc_rust_database_get_name()` to return `*const c_char`.
}

impl<'r> Database<'r> {
    pub fn new(runtime: &'r tokio::runtime::Runtime, db: mongodb::Database) -> Self {
        let name = CString::new(db.name()).unwrap();

        Self { runtime, db, name }
    }

    fn collection(&'r self, name: &str) -> Collection<'r> {
        Collection::new(
            &self.runtime,
            self.db.collection::<mongodb::bson::Document>(name),
        )
    }
}
