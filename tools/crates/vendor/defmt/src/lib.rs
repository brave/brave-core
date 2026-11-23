//! A highly efficient logging framework that targets resource-constrained
//! devices, like microcontrollers.
//!
//! Check out the defmt book at <https://defmt.ferrous-systems.com> for more
//! information about how to use it.
//!
//! # Compatibility
//!
//! The `defmt` wire format might change between minor versions. Attempting to
//! read a defmt stream with an incompatible version will result in an error,
//! and any tool used to process that stream should first check for a symbol
//! named like `_defmt_version_ = X`, where X indicates the wire format version
//! in use.
//!
//! Updating your version of defmt might mean you also have to update your
//! version of `defmt-print` or `defmt-decoder`.

#![cfg_attr(not(feature = "unstable-test"), no_std)]
// NOTE if you change this URL you'll also need to update all other crates in this repo
#![doc(html_logo_url = "https://knurling.ferrous-systems.com/knurling_logo_light_text.svg")]
#![warn(missing_docs)]

#[cfg(feature = "alloc")]
extern crate alloc;

// This must be in the root lib.rs, otherwise it doesn't appear in the final binary.

/// The defmt ABI and wire format version.
///
/// This number has to be updated every time there is a backwards-incompatible change to
/// - the symbol naming scheme
/// - the symbol and section layout
/// - the data encoding / wire format
#[used]
#[cfg_attr(target_os = "macos", link_section = ".defmt,end.VERSION")]
#[cfg_attr(not(target_os = "macos"), link_section = ".defmt.end")]
#[export_name = "_defmt_version_ = 4"]
static DEFMT_VERSION: u8 = 0;

#[used]
#[cfg_attr(target_os = "macos", link_section = ".defmt,end.ENCODING")]
#[cfg_attr(not(target_os = "macos"), link_section = ".defmt.end")]
#[cfg_attr(feature = "encoding-raw", export_name = "_defmt_encoding_ = raw")]
#[cfg_attr(
    not(feature = "encoding-raw"),
    export_name = "_defmt_encoding_ = rzcobs"
)]
#[allow(missing_docs)]
#[doc(hidden)]
pub static DEFMT_ENCODING: u8 = 0;

mod encoding;
#[doc(hidden)]
pub mod export;
mod formatter;
mod impls;
#[cfg(all(test, feature = "unstable-test"))]
mod tests;
mod traits;

pub use crate::{
    encoding::Encoder,
    formatter::{Formatter, Str},
    impls::adapter::{Debug2Format, Display2Format},
    traits::{Format, Logger},
};

#[cfg(all(test, not(feature = "unstable-test")))]
compile_error!(
    "to run unit tests enable the `unstable-test` feature, e.g. `cargo t --features unstable-test`"
);

/// Just like the [`core::assert!`] macro but `defmt` is used to log the panic message
///
/// [`core::assert!`]: https://doc.rust-lang.org/core/macro.assert.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::assert_ as assert;

/// Just like the [`core::assert_eq!`] macro but `defmt` is used to log the panic message
///
/// [`core::assert_eq!`]: https://doc.rust-lang.org/core/macro.assert_eq.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::assert_eq_ as assert_eq;

/// Just like the [`core::assert_ne!`] macro but `defmt` is used to log the panic message
///
/// [`core::assert_ne!`]: https://doc.rust-lang.org/core/macro.assert_ne.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::assert_ne_ as assert_ne;

/// Just like the [`core::debug_assert!`] macro but `defmt` is used to log the panic message
///
/// [`core::debug_assert!`]: https://doc.rust-lang.org/core/macro.debug_assert.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::debug_assert_ as debug_assert;

/// Just like the [`core::debug_assert_eq!`] macro but `defmt` is used to log the panic message
///
/// [`core::debug_assert_eq!`]: https://doc.rust-lang.org/core/macro.debug_assert_eq.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::debug_assert_eq_ as debug_assert_eq;

/// Just like the [`core::debug_assert_ne!`] macro but `defmt` is used to log the panic message
///
/// [`core::debug_assert_ne!`]: https://doc.rust-lang.org/core/macro.debug_assert_ne.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::debug_assert_ne_ as debug_assert_ne;

/// Just like the [`core::unreachable!`] macro but `defmt` is used to log the panic message
///
/// [`core::unreachable!`]: https://doc.rust-lang.org/core/macro.unreachable.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::unreachable_ as unreachable;

/// Just like the [`core::todo!`] macro but `defmt` is used to log the panic message
///
/// [`core::todo!`]: https://doc.rust-lang.org/core/macro.todo.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::todo_ as todo;

/// Just like the [`core::unimplemented!`] macro but `defmt` is used to log the panic message
///
/// [`core::unimplemented!`]: https://doc.rust-lang.org/core/macro.unimplemented.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::todo_ as unimplemented;

/// Just like the [`core::panic!`] macro but `defmt` is used to log the panic message
///
/// [`core::panic!`]: https://doc.rust-lang.org/core/macro.panic.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::panic_ as panic;

/// Unwraps an `Option` or `Result`, panicking if it is `None` or `Err`.
///
/// This macro is roughly equivalent to `{Option,Result}::{expect,unwrap}` but invocation looks
/// a bit different because this is a macro and not a method. The other difference is that
/// `unwrap!`-ing a `Result<T, E>` value requires that the error type `E` implements the `Format`
/// trait
///
/// The following snippet shows the differences between core's unwrap method and defmt's unwrap
/// macro:
///
/// ```
/// use defmt::unwrap;
///
/// # let option = Some(());
/// let x = option.unwrap();
/// let x = unwrap!(option);
///
/// # let result = Ok::<(), ()>(());
/// let x = result.unwrap();
/// let x = unwrap!(result);
///
/// let x = result.expect("text");
/// let x = unwrap!(result, "text");
///
/// # let arg = ();
/// let x = result.expect(&format!("text {:?}", arg));
/// let x = unwrap!(result, "text {:?}", arg); // arg must be implement `Format`
/// ```
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::unwrap;

/// This is an alias for defmt's [`unwrap`] macro which supports messages like std's except.
/// ```
/// use defmt::expect;
///
/// # let result = Ok::<(), ()>(());
/// # let arg = ();
/// let x = result.expect(&format!("text {:?}", arg));
/// let x = expect!(result, "text {:?}", arg); // arg must be implement `Format`
/// ```
///
/// For the complete documentation see that of defmt's *unwrap* macro.
// note: Linking to unwrap is broken as of 2024-10-09, it links back to expect
pub use defmt_macros::unwrap as expect;

/// Overrides the panicking behavior of `defmt::panic!`
///
/// By default, `defmt::panic!` calls `core::panic!` after logging the panic message using `defmt`.
/// This can result in the panic message being printed twice in some cases. To avoid that issue use
/// this macro. See [the manual] for details.
///
/// [the manual]: https://defmt.ferrous-systems.com/panic.html
///
/// # Inter-operation with built-in attributes
///
/// This attribute cannot be used together with the `export_name` or `no_mangle` attributes
pub use defmt_macros::panic_handler;

/// Creates an interned string ([`Str`]) from a string literal.
///
/// This must be called on a string literal, and will allocate the literal in the object file. At
/// runtime, only a small string index is required to refer to the string, represented as the
/// [`Str`] type.
///
/// # Example
///
/// ```
/// let interned = defmt::intern!("long string literal taking up little space");
/// ```
///
/// [`Str`]: struct.Str.html
pub use defmt_macros::intern;

/// Always logs data irrespective of log level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::println;

/// Logs data at *debug* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::debug;
/// Logs data at *error* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::error;
/// Logs data at *info* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::info;
/// Logs data at *trace* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::trace;
/// Logs data at *warn* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt_macros::warn;

/// Just like the [`std::dbg!`] macro but `defmt` is used to log the message at `TRACE` level.
///
/// [`std::dbg!`]: https://doc.rust-lang.org/std/macro.dbg.html
pub use defmt_macros::dbg;

/// Writes formatted data to a [`Formatter`].
///
/// [`Formatter`]: struct.Formatter.html
pub use defmt_macros::write;

/// Defines the global defmt logger.
///
/// `#[global_logger]` needs to be put on a unit struct type declaration. This struct has to
/// implement the [`Logger`] trait.
///
/// # Example
///
/// ```
/// use defmt::{Logger, global_logger};
///
/// #[global_logger]
/// struct MyLogger;
///
/// unsafe impl Logger for MyLogger {
///     fn acquire() {
/// # todo!()
///         // ...
///     }
///     unsafe fn flush() {
///         # todo!()
///         // ...
///     }
///     unsafe fn release() {
/// # todo!()
///         // ...
///     }
///     unsafe fn write(bytes: &[u8]) {
/// # todo!()
///         // ...
///     }
/// }
/// ```
///
/// [`Logger`]: trait.Logger.html
pub use defmt_macros::global_logger;

/// Defines the global timestamp provider for defmt.
///
/// This macro can be used to attach a timestamp or other data to every defmt message. Its syntax
/// works exactly like the logging macros, except that no local variables can be accessed and the
/// macro should be placed in a module instead of a function.
///
/// `timestamp!` must only be used once across the crate graph.
///
/// If no crate defines a timestamp, no timestamp will be included in the logged messages.
///
/// # Examples
///
/// ```
/// # use core::sync::atomic::{AtomicU32, Ordering};
///
/// static COUNT: AtomicU32 = AtomicU32::new(0);
/// defmt::timestamp!("{=u32:us}", COUNT.fetch_add(1, Ordering::Relaxed));
/// ```
pub use defmt_macros::timestamp;

/// Generates a bitflags structure that can be formatted with defmt.
///
/// This macro is a wrapper around the [`bitflags!`] crate, and provides an (almost) identical
/// interface. Refer to [its documentation] for an explanation of the syntax.
///
/// [its documentation]: https://docs.rs/bitflags/1/bitflags/
///
/// # Limitations
///
/// This macro only supports bitflags structs represented as one of Rust's built-in unsigned integer
/// types (`u8`, `u16`, `u32`, `u64`, or `u128`). Custom types are not supported. This restriction
/// is necessary to support defmt's efficient encoding.
///
/// # Examples
///
/// The example from the bitflags crate works as-is:
///
/// ```
/// defmt::bitflags! {
///     struct Flags: u32 {
///         const A = 0b00000001;
///         const B = 0b00000010;
///         const C = 0b00000100;
///         const ABC = Self::A.bits | Self::B.bits | Self::C.bits;
///     }
/// }
///
/// defmt::info!("Flags::ABC: {}", Flags::ABC);
/// defmt::info!("Flags::empty(): {}", Flags::empty());
/// ```
pub use defmt_macros::bitflags;

#[doc(hidden)] // documented as the `Format` trait instead
pub use defmt_macros::Format;

// There is no default timestamp format. Instead, the decoder looks for a matching ELF symbol. If
// absent, timestamps are turned off.
#[export_name = "__defmt_default_timestamp"]
fn default_timestamp(_f: Formatter<'_>) {}

#[export_name = "__defmt_default_panic"]
fn default_panic() -> ! {
    core::panic!()
}

/// Block until host has read all pending data.
///
/// The flush operation will not fail, but might not succeed in flushing _all_ pending data. It is
/// implemented as a "best effort" operation.
///
/// This calls the method `flush` of the used "global [`Logger`]". The logger is likely provided by
/// [`defmt-rtt`](https://crates.io/crates/defmt-rtt) or [`defmt-itm`](https://crates.io/crates/defmt-itm).
pub fn flush() {
    match () {
        #[cfg(feature = "unstable-test")]
        () => {
            // no-op when run on host
        }

        #[cfg(not(feature = "unstable-test"))]
        () => {
            extern "Rust" {
                fn _defmt_acquire();
                fn _defmt_flush();
                fn _defmt_release();
            }
            // SAFETY:
            // * we call these function in the correct order: first acquire the lock, then flush and
            //   finally release the lock
            // * these function should be provided by the macro `#[global_logger]` and therefore
            //   trustworthy to call through FFI-bounds
            unsafe {
                _defmt_acquire();
                _defmt_flush();
                _defmt_release()
            }
        }
    }
}

#[cfg(not(feature = "unstable-test"))]
#[doc(hidden)]
pub struct IdRanges {
    pub trace: core::ops::Range<u16>,
    pub debug: core::ops::Range<u16>,
    pub info: core::ops::Range<u16>,
    pub warn: core::ops::Range<u16>,
    pub error: core::ops::Range<u16>,
}

#[cfg(not(feature = "unstable-test"))]
impl IdRanges {
    pub fn get() -> Self {
        extern "C" {
            static __DEFMT_MARKER_TRACE_START: u8;
            static __DEFMT_MARKER_TRACE_END: u8;
            static __DEFMT_MARKER_DEBUG_START: u8;
            static __DEFMT_MARKER_DEBUG_END: u8;
            static __DEFMT_MARKER_INFO_START: u8;
            static __DEFMT_MARKER_INFO_END: u8;
            static __DEFMT_MARKER_WARN_START: u8;
            static __DEFMT_MARKER_WARN_END: u8;
            static __DEFMT_MARKER_ERROR_START: u8;
            static __DEFMT_MARKER_ERROR_END: u8;
        }

        let trace_start = unsafe { &__DEFMT_MARKER_TRACE_START as *const u8 as u16 };
        let trace_end = unsafe { &__DEFMT_MARKER_TRACE_END as *const u8 as u16 };
        let debug_start = unsafe { &__DEFMT_MARKER_DEBUG_START as *const u8 as u16 };
        let debug_end = unsafe { &__DEFMT_MARKER_DEBUG_END as *const u8 as u16 };
        let info_start = unsafe { &__DEFMT_MARKER_INFO_START as *const u8 as u16 };
        let info_end = unsafe { &__DEFMT_MARKER_INFO_END as *const u8 as u16 };
        let warn_start = unsafe { &__DEFMT_MARKER_WARN_START as *const u8 as u16 };
        let warn_end = unsafe { &__DEFMT_MARKER_WARN_END as *const u8 as u16 };
        let error_start = unsafe { &__DEFMT_MARKER_ERROR_START as *const u8 as u16 };
        let error_end = unsafe { &__DEFMT_MARKER_ERROR_END as *const u8 as u16 };
        Self {
            trace: trace_start..trace_end,
            debug: debug_start..debug_end,
            info: info_start..info_end,
            warn: warn_start..warn_end,
            error: error_start..error_end,
        }
    }
}
