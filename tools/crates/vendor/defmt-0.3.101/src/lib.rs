//! A highly efficient logging framework that targets resource-constrained
//! devices, like microcontrollers.
//!
//! Check out the defmt book at <https://defmt.ferrous-systems.com> for more
//! information about how to use it.
//!
//! # Compatibility
//!
//! This is a defmt-0.3 compatbility crate. It depends upon `defmt-1.0` and
//! re-exports the items that were available in `defmt-0.3`. This allows you to
//! mix defmt-0.3 and defmt-1.0 within the same compilation.
//!
//! The `defmt` wire format might change between minor versions. Attempting to
//! read a defmt stream with an incompatible version will result in an error,
//! and any tool used to process that stream should first check for a symbol
//! named like `_defmt_version_ = X`, where X indicates the wire format version
//! in use.
//!
//! Updating your version of defmt might mean you also have to update your
//! version of `defmt-print` or `defmt-decoder`.

#![no_std]

/// Just like the [`core::assert!`] macro but `defmt` is used to log the panic message
///
/// [`core::assert!`]: https://doc.rust-lang.org/core/macro.assert.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::assert;

/// Just like the [`core::assert_eq!`] macro but `defmt` is used to log the panic message
///
/// [`core::assert_eq!`]: https://doc.rust-lang.org/core/macro.assert_eq.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::assert_eq;

/// Just like the [`core::assert_ne!`] macro but `defmt` is used to log the panic message
///
/// [`core::assert_ne!`]: https://doc.rust-lang.org/core/macro.assert_ne.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::assert_ne;

/// Just like the [`core::debug_assert!`] macro but `defmt` is used to log the panic message
///
/// [`core::debug_assert!`]: https://doc.rust-lang.org/core/macro.debug_assert.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::debug_assert;

/// Just like the [`core::debug_assert_eq!`] macro but `defmt` is used to log the panic message
///
/// [`core::debug_assert_eq!`]: https://doc.rust-lang.org/core/macro.debug_assert_eq.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::debug_assert_eq;

/// Just like the [`core::debug_assert_ne!`] macro but `defmt` is used to log the panic message
///
/// [`core::debug_assert_ne!`]: https://doc.rust-lang.org/core/macro.debug_assert_ne.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::debug_assert_ne;

/// Just like the [`core::unreachable!`] macro but `defmt` is used to log the panic message
///
/// [`core::unreachable!`]: https://doc.rust-lang.org/core/macro.unreachable.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::unreachable;

/// Just like the [`core::todo!`] macro but `defmt` is used to log the panic message
///
/// [`core::todo!`]: https://doc.rust-lang.org/core/macro.todo.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::todo;

/// Just like the [`core::unimplemented!`] macro but `defmt` is used to log the panic message
///
/// [`core::unimplemented!`]: https://doc.rust-lang.org/core/macro.unimplemented.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::todo as unimplemented;

/// Just like the [`core::panic!`] macro but `defmt` is used to log the panic message
///
/// [`core::panic!`]: https://doc.rust-lang.org/core/macro.panic.html
///
/// If used, the format string must follow the defmt syntax (documented in [the manual])
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::panic;

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
pub use defmt10::unwrap;

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
pub use defmt10::unwrap as expect;

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
pub use defmt10::panic_handler;

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
pub use defmt10::intern;

/// Always logs data irrespective of log level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::println;

/// Logs data at *debug* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::debug;
/// Logs data at *error* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::error;
/// Logs data at *info* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::info;
/// Logs data at *trace* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::trace;
/// Logs data at *warn* level.
///
/// Please refer to [the manual] for documentation on the syntax.
///
/// [the manual]: https://defmt.ferrous-systems.com/macros.html
pub use defmt10::warn;

/// Just like the [`std::dbg!`] macro but `defmt` is used to log the message at `TRACE` level.
///
/// [`std::dbg!`]: https://doc.rust-lang.org/std/macro.dbg.html
pub use defmt10::dbg;

/// Writes formatted data to a [`Formatter`].
///
/// [`Formatter`]: struct.Formatter.html
pub use defmt10::write;

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
pub use defmt10::global_logger;

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
pub use defmt10::timestamp;

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
pub use defmt10::bitflags;

#[doc(inline)]
pub use defmt10::{Debug2Format, Display2Format, Encoder, Formatter, Str};

#[doc(inline)]
pub use defmt10::{Format, Logger};

#[doc(hidden)]
pub use defmt10::export;

#[doc(inline)]
pub use defmt10::flush;
