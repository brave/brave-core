use defmt_macros::internp;

#[allow(unused_imports)]
use crate as defmt;
use crate::{export, Formatter, Str};

/// Trait for types that can be formatted via defmt.
///
/// This trait is used by the `{:?}` format specifier and can format a wide range of types.
/// User-defined types can `#[derive(Format)]` to get an auto-generated implementation of this
/// trait.
///
/// **Note**: The implementation of `#[derive(Format)]` assumes that no builtin types are shadowed
/// (for example by defining a `struct u8;`). This allows it to represent them more compactly.
///
/// # Example
///
/// Usually, an implementation of this trait can be `#[derive]`d automatically:
///
/// ```
/// use defmt::Format;
///
/// #[derive(Format)]
/// struct Header {
///     source: u8,
///     destination: u8,
///     sequence: u16,
/// }
/// ```
///
/// Manual implementations can make use of the [`write!`] macro:
///
/// ```
/// use defmt::{Format, Formatter, write};
///
/// struct Id(u32);
///
/// impl Format for Id {
///     fn format(&self, fmt: Formatter) {
///         // Format as hexadecimal.
///         write!(fmt, "Id({:x})", self.0);
///     }
/// }
/// ```
/// **Note** Some implementations of standard types like `Vec<T>` are hidden behind the `alloc` feature flag.
pub trait Format {
    /// Writes the defmt representation of `self` to `fmt`.
    fn format(&self, fmt: Formatter);

    #[doc(hidden)]
    fn _format_tag() -> Str {
        internp!("{=__internal_FormatSequence}")
    }

    #[doc(hidden)]
    fn _format_data(&self) {
        self.format(export::make_formatter());
        export::u16(&0); // terminator
    }
}

/// Global logger acquire-release mechanism
///
/// This trait's methods will be called by the defmt logging macros to transmit the
/// encoded log data over the wire. The call order is:
/// - One `acquire()` call to start the log frame.
/// - Multiple `write()` calls, with fragments of the log frame data each.
/// - One `release()` call.
///
/// The data passed to `write()` is *unencoded*. Implementations MUST encode it with `Encoder`
/// prior to sending it over the wire. The simplest way is for `acquire()` to call `Encoder::start_frame()`,
/// `write()` to call `Encoder::write()`, and `release()` to call `Encoder::end_frame()`.
///
/// The global logger can be acquired once for each "execution context". The definition
/// of execution context is up to the implementation. For example, it can be:
///
/// - the entire process.
/// - one thread in std environments.
/// - one interrupt priority level in embedded devices.
///
/// # Safety
///
/// - `acquire` logically acquires the global logger in the current execution context.
///   The acquiring is tracked internally, no Rust object is returned representing ownership.
/// - `acquire` is a safe function, therefore it must be thread-safe and interrupt-safe
///
/// And, not safety related, the methods should never be invoked from user code. The easiest way to
/// ensure this is to implement `Logger` on a *private* `struct` and mark that `struct` as the
/// `#[global_logger]`.
pub unsafe trait Logger {
    /// Acquire the global logger in the current execution context.
    ///
    /// This will be called by the defmt logging macros before writing each log frame.
    ///
    /// Panics if already acquired in the current execution context. Otherwise it must never fail.
    fn acquire();

    /// Block until host has read all pending data.
    ///
    /// The flush operation must not fail. This is a "best effort" operation, I/O errors should be discarded.
    ///
    /// # Safety
    /// Must only be called when the global logger is acquired in the current execution context.
    /// (i.e. between `acquire()` and `release()`).
    unsafe fn flush();

    /// Releases the global logger in the current execution context.
    ///
    /// This will be called by the defmt logging macros after writing each log frame.
    ///
    /// # Safety
    /// Must be called exactly once for each acquire(), in the same execution context.
    unsafe fn release();

    /// Writes `bytes` to the destination.
    ///
    /// This will be called by the defmt logging macros to transmit frame data. One log frame may cause multiple `write` calls.
    ///
    /// The write operation must not fail. This is a "best effort" operation, I/O errors should be discarded.
    ///
    /// The `bytes` are unencoded log frame data, they MUST be encoded with `Encoder` prior to
    /// sending over the wire.
    ///
    /// Note that a call to `write` does *not* correspond to a defmt logging macro invocation. A
    /// single `defmt::info!` call can result in an arbitrary number of `write` calls.
    ///
    /// # Safety
    /// Must only be called when the global logger is acquired in the current execution context.
    /// (i.e. between `acquire()` and `release()`).
    unsafe fn write(bytes: &[u8]);
}
