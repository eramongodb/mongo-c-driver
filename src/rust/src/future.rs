use std::pin::pin;
use std::task::{Context, Poll, Waker};

use async_ffi::{FfiFuture, FutureExt};
use tokio;

pub struct FutureValueType<T> {
    pub future: FfiFuture<Result<T, mongodb::error::Error>>,
    pub result: Option<Result<T, mongodb::error::Error>>,
}

pub enum FutureValue {
    Void(FutureValueType<()>),
    Int32(FutureValueType<i32>),
    UInt64(FutureValueType<u64>),
    Document(FutureValueType<mongodb::bson::Document>),
    // ...
}

pub struct Future<'r> {
    pub runtime: &'r tokio::runtime::Runtime,
    pub value: FutureValue,
}

macro_rules! future_value_op {
    ($value:expr, $v:ident => $e:expr) => {
        match $value {
            FutureValue::Void($v) => $e,
            FutureValue::Int32($v) => $e,
            FutureValue::UInt64($v) => $e,
            FutureValue::Document($v) => $e,
            // ...
        }
    };
}

trait Pollable {
    fn poll(&mut self) -> bool;
    fn is_ready(&self) -> bool;
}

trait PollableWithTimeout {
    fn poll_with_timeout(&mut self, runtime: &tokio::runtime::Runtime, timeout_ms: u64) -> bool;
}

impl Pollable for FutureValue {
    fn poll(&mut self) -> bool {
        future_value_op!(self, v => v.poll())
    }

    fn is_ready(&self) -> bool {
        future_value_op!(self, v => v.is_ready())
    }
}

impl PollableWithTimeout for FutureValue {
    fn poll_with_timeout(&mut self, runtime: &tokio::runtime::Runtime, timeout_ms: u64) -> bool {
        future_value_op!(self, v => v.poll_with_timeout(runtime, timeout_ms))
    }
}

impl<T> Pollable for FutureValueType<T> {
    fn poll(&mut self) -> bool {
        match pin!(&mut self.future)
            .as_mut()
            .poll(&mut Context::from_waker(Waker::noop()))
        {
            Poll::Ready(n) => {
                self.result = Some(n);
                true
            }
            Poll::Pending => false,
        }
    }

    fn is_ready(&self) -> bool {
        self.result.is_some()
    }
}

impl<T> PollableWithTimeout for FutureValueType<T> {
    fn poll_with_timeout(&mut self, runtime: &tokio::runtime::Runtime, timeout_ms: u64) -> bool {
        match runtime.block_on(tokio::time::timeout(
            tokio::time::Duration::from_millis(timeout_ms),
            &mut self.future,
        )) {
            Ok(res) => {
                self.result = Some(res);
                true
            }
            Err(_) => false, // TODO(cdriver-6204): error handling.
        }
    }
}

/// Destroy the future.
#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_future_destroy(future: *mut Future) {
    if !future.is_null() {
        unsafe {
            drop(Box::from_raw(future));
        }
    }
}

/// Force progress on IO and timer drivers.
#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_future_make_progress(future: *mut Future) {
    let Future { runtime, .. } = unsafe { &*future };
    let _guard = runtime.enter();

    // Use an immediately-yielding task to allow IO and timer drivers to make progress on the current thread.
    //
    // Per Tokio documentation:
    //
    // > When [block_on()] is used on a current_thread runtime, only the `Runtime::block_on` method can drive the IO and
    // > timer drivers, but the `Handle::block_on` method cannot drive them. This means that, when using this method on
    // > a current_thread runtime, anything that relies on IO or timers will not work unless there is another thread
    // > currently calling `Runtime::block_on` on the same runtime.
    //
    // and:
    //
    // > A task yields by awaiting on `yield_now()`, and may resume when that future completes (with no output). The
    // > current task will be re-added as a pending task at the *back* of the pending queue. Any other pending tasks
    // > will be scheduled. NO other waking is required for the task to continue.
    //
    // and:
    //
    // > Tokio provides the following fairness guarantee:
    // > > If the total number of tasks does not grow without bound, and no task is blocking the thread, then it is
    // > > guaranteed that tasks are scheduled fairly.
    // > Other than the above fairness guarantee, there is no guarantee about the order in which tasks are scheduled.
    // > There is also no guarantee that the runtime is equally fair to all tasks. For example, if the runtime has two
    // > tasks A and B that are both ready, then the runtime may schedule A five times before it schedules B. This is
    // > the case even if A yields using yield_now. All that is guaranteed is that it will schedule B eventually.
    //
    // and:
    //
    // > Under the same assumptions as the previous fairness guarantee, Tokio guarantees that [the runtime] will wake
    // > tasks with an IO or timer event within some maximum number of time units.
    runtime.block_on(tokio::task::yield_now());
}

/// Poll the future.
#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_future_poll(future: *mut Future) -> bool {
    let Future { runtime, value } = unsafe { &mut *future };
    let _guard = runtime.enter();

    if value.is_ready() {
        return true;
    }

    // Allow directly polling the future without any special wakeup conditions (using a noop waker).
    //
    // Per Tokio documentation:
    //
    // > Futures alone are *inert*; they must be *actively* `poll`ed for the underlying computation to make progress,
    // > meaning that each time the current task is woken up, it should actively re-`poll` pending futures that it still
    // > has an interest in.
    //
    // The active (re-)polling of pending futures (via `.block_on()`) must be handled by either
    // `mongoc_rust_future_make_progress()` or `mongoc_rust_future_poll_with_timeout()`. Repeatedly calling
    // `mongoc_rust_future_poll()` alone DOES NOT make progress.
    value.poll()
}

/// Poll the future with a timeout.
#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_future_poll_with_timeout(
    future: *mut Future,
    timeout_ms: i64,
) -> bool {
    let Future { runtime, value } = unsafe { &mut *future };
    let _guard = runtime.enter();

    if value.is_ready() {
        return true;
    }

    value.poll_with_timeout(runtime, timeout_ms as u64) // TODO(cdriver-6204): error handling.
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_void() {
        let runtime = tokio::runtime::Builder::new_current_thread()
            .build()
            .unwrap();

        let mut future = Future {
            runtime: &runtime,
            value: FutureValue::Void(FutureValueType {
                future: async move { Ok(()) }.into_ffi(),
                result: None,
            }),
        };

        assert!(future.value.poll());

        if let FutureValue::Void(v) = &future.value {
            assert!(v.result.is_some());
            assert_eq!(v.result.as_ref().unwrap().clone().unwrap(), ());
        } else {
            assert!(false, "incorrect type");
        };
    }

    #[test]
    fn test_int32() {
        let runtime = tokio::runtime::Builder::new_current_thread()
            .build()
            .unwrap();

        let mut future = Future {
            runtime: &runtime,
            value: FutureValue::Int32(FutureValueType {
                future: async move { Ok(123) }.into_ffi(),
                result: None,
            }),
        };

        assert!(future.value.poll());

        if let FutureValue::Int32(v) = &future.value {
            assert!(v.result.is_some());
            assert_eq!(v.result.as_ref().unwrap().clone().unwrap(), 123);
        } else {
            assert!(false, "incorrect type");
        };

        let mut ret: i32 = 0;
        assert!(!mongoc_rust_future_get_void(&mut future));
        assert!(mongoc_rust_future_get_int32(&mut future, &mut ret));
        assert_eq!(ret, 123);
    }

    #[test]
    fn test_uint64() {
        let runtime = tokio::runtime::Builder::new_current_thread()
            .build()
            .unwrap();

        let mut future = Future {
            runtime: &runtime,
            value: FutureValue::UInt64(FutureValueType {
                future: async move { Ok(123) }.into_ffi(),
                result: None,
            }),
        };

        assert!(future.value.poll());

        if let FutureValue::UInt64(v) = &future.value {
            assert!(v.result.is_some());
            assert_eq!(v.result.as_ref().unwrap().clone().unwrap(), 123);
        } else {
            assert!(false, "incorrect type");
        };

        let mut ret: u64 = 0;
        assert!(!mongoc_rust_future_get_void(&mut future));
        assert!(mongoc_rust_future_get_uint64(&mut future, &mut ret));
        assert_eq!(ret, 123);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_future_get_void(future: *mut Future) -> bool {
    let Future { value, .. } = unsafe { &mut *future };

    if let FutureValue::Void(FutureValueType { result: Some(Ok(_)), .. }) = value {
        true
    } else {
        false
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_future_get_int32(
    future: *mut Future,
    result_out_ptr: *mut i32,
) -> bool {
    let Future { value, .. } = unsafe { &mut *future };
    let result_out = unsafe { &mut *result_out_ptr };

    if let FutureValue::Int32(FutureValueType { result: Some(Ok(v)), .. }) = value {
        *result_out = *v;
        true
    } else {
        false
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mongoc_rust_future_get_uint64(
    future: *mut Future,
    result_out_ptr: *mut u64,
) -> bool {
    let Future { value, .. } = unsafe { &mut *future };
    let result_out = unsafe { &mut *result_out_ptr };

    if let FutureValue::UInt64(FutureValueType { result: Some(Ok(v)), .. }) = value {
        *result_out = *v;
        true
    } else {
        false
    }
}
