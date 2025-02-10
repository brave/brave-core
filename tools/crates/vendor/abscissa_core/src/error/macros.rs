//! Error-handling macros for the `abscissa` framework
//!
//! This crate defines two error handling macros designed to produce formatted
//! error messages from error kind enums that implement the `Fail` trait:
//!
//! * `err!(kind, description)` creates a new `Error<Kind>` with the given
//!   description. If additional parameters are given, `description` is treated as
//!   a format string, e.g. `err!(kind, "something went wrong: {}", &wrongness)`.
//! * `fail!(kind, description)` creates a new `Error<kind>` and returns it.

/// Create a new error (of a given kind) with a formatted message
#[macro_export]
macro_rules! format_err {
    ($kind:expr, $msg:expr) => {
        $kind.context($crate::error::Message::new($msg))
    };
    ($kind:expr, $fmt:expr, $($arg:tt)+) => {
        format_err!($kind, &format!($fmt, $($arg)+))
    };
}

/// Create and return an error with a formatted message
#[macro_export]
macro_rules! fail {
    ($kind:expr, $msg:expr) => {
        return Err(format_err!($kind, $msg).into())
    };
    ($kind:expr, $fmt:expr, $($arg:tt)+) => {
        fail!($kind, &format!($fmt, $($arg)+))
    };
}

/// Ensure a condition holds, returning an error if it doesn't (ala assert)
#[macro_export]
macro_rules! ensure {
    ($cond:expr, $kind:expr, $msg:expr) => {
        if !($cond) {
            return Err(format_err!($kind, $msg).into());
        }
    };
    ($cond:expr, $kind:expr, $fmt:expr, $($arg:tt)+) => {
        ensure!($cond, $kind, format!($fmt, $($arg)+))
    };
}

/// Terminate the application with a fatal error, running Abscissa's shutdown hooks.
///
/// This macro is useful in cases where you don't have a particular error type
/// you'd like to use when exiting but would like to have a formatted error
/// message. If you do have a suitable error type, use `fatal_error!()` instead.
///
/// Takes the same arguments as `format!()`.
#[macro_export]
macro_rules! fatal {
    ($app:expr, $msg:expr) => {
        $crate::application::exit::fatal_error($app, $crate::error::Message::new($msg))
    };
    ($app:expr, $fmt:expr, $($arg:tt)+) => {
        fatal!($app, format!($fmt, $($arg:tt)+))
    };
}
