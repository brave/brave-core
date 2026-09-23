// SPDX-License-Identifier: CC0-1.0

/// Formats error.
///
/// If `std` feature is OFF appends error source (delimited by `: `). We do this because
/// `e.source()` is only available in std builds, without this macro the error source is lost for
/// no-std builds.
macro_rules! write_err {
    ($writer:expr, $string:literal $(, $args:expr)*; $source:expr) => {
        {
            #[cfg(feature = "std")]
            {
                let _ = &$source;   // Prevents clippy warnings.
                write!($writer, $string $(, $args)*)
            }
            #[cfg(not(feature = "std"))]
            {
                write!($writer, concat!($string, ": {}") $(, $args)*, $source)
            }
        }
    }
}
pub(crate) use write_err;
