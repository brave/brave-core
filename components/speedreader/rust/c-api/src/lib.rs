use ::speedreader::*;
use libc::{c_char, c_int, size_t};
use std::cell::RefCell;
use std::{ptr, slice, str};

#[inline]
fn to_ptr<T>(val: T) -> *const T {
    Box::into_raw(Box::new(val))
}

#[inline]
fn to_ptr_mut<T>(val: T) -> *mut T {
    Box::into_raw(Box::new(val))
}

// NOTE: abort the thread if we receive NULL where unexpected
macro_rules! assert_not_null {
    ($var:ident) => {
        assert!(!$var.is_null(), "{} is NULL", stringify!($var));
    };
}

// NOTE: all these utilities are macros so we can propagate the variable
// name to the null pointer assertion.
macro_rules! to_ref {
    ($ptr:ident) => {{
        assert_not_null!($ptr);
        unsafe { &*$ptr }
    }};
}

macro_rules! void_to_box {
    ($ptr:ident) => {{
        assert_not_null!($ptr);
        unsafe { Box::from_raw($ptr as *mut _) }
    }};
}

macro_rules! leak_void_to_box {
    ($ptr:ident) => {{
        assert_not_null!($ptr);
        Box::leak(unsafe { Box::from_raw($ptr as *mut _) })
    }};
}

macro_rules! box_to_opaque {
    ($ptr:ident, $opaque:ident) => {{
        to_ptr_mut($ptr) as *mut $opaque
    }};
}

macro_rules! to_box {
    ($ptr:ident) => {{
        assert_not_null!($ptr);
        unsafe { Box::from_raw($ptr) }
    }};
}

macro_rules! to_bytes {
    ($data:ident, $len:ident) => {{
        assert_not_null!($data);
        unsafe { slice::from_raw_parts($data as *const u8, $len) }
    }};
}

macro_rules! to_str {
    ($data:ident, $len:ident) => {
        str::from_utf8(to_bytes!($data, $len)).into()
    };
}

macro_rules! unwrap_or_ret {
    ($expr:expr, $ret_val:expr) => {
        match $expr {
            Ok(v) => v,
            Err(err) => {
                crate::errors::LAST_ERROR.with(|cell| *cell.borrow_mut() = Some(err.into()));
                return $ret_val;
            }
        }
    };
}

macro_rules! unwrap_or_ret_err_code {
    ($expr:expr) => {
        unwrap_or_ret!($expr, -1)
    };
}

macro_rules! unwrap_or_ret_null {
    ($expr:expr) => {
        unwrap_or_ret!($expr, ptr::null_mut())
    };
}

mod errors;
mod speedreader;
mod charbuf;

pub use self::charbuf::CharBuf;
