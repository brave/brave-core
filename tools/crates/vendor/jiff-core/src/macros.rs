#![allow(unused_macros, unused_imports)]

/*
/// Unwrap an `Option<T>` in a `const` context.
///
/// If it fails, panics with the given message.
macro_rules! unwrap {
    ($val:expr, $msg:expr$(,)?) => {
        match $val {
            Some(val) => val,
            None => panic!($msg),
        }
    };
}
*/

/// Unwrap a `Result<T, E>` in a `const` context.
///
/// If it fails, panics with the given message.
macro_rules! unwrapr {
    ($val:expr, $msg:expr$(,)?) => {
        match $val {
            Ok(val) => val,
            Err(_) => panic!($msg),
        }
    };
}

/// Get `T` out of `Result<T, E>` or return `Err(e)` in a `const` context.
///
/// No conversions are performed. This only works when used in a function
/// that itself returns `Result<..., E>`.
macro_rules! ctry {
    ($val:expr) => {
        match $val {
            Ok(val) => val,
            Err(err) => return Err(err),
        }
    };
}

/// Convert the given error to a `RangeError` and return it.
///
/// `val` must be one of `RangeError`, `BoundsError` or `SpecialBoundsError`.
/// In particular, `val.into_range_error()` is called to convert to a
/// `RangeError`.
///
/// This only works when used in a function
/// that itself returns `Result<..., RangeError>`.
macro_rules! rbail {
    ($val:expr) => {
        return Err($val.into_range_error())
    };
}

/// Get `T` out of `Result<T, E>` or return `Err(e)` in a `const` context.
///
/// `E` must be one of `RangeError`, `BoundsError` or `SpecialBoundsError`. In
/// particular, `E::into_range_error` is called to convert to a `RangeError`.
///
/// This only works when used in a function
/// that itself returns `Result<..., RangeError>`.
macro_rules! rtry {
    ($val:expr) => {
        match $val {
            Ok(val) => val,
            Err(err) => crate::macros::rbail!(err),
        }
    };
}

pub(crate) use {ctry, rbail, rtry, unwrapr};
