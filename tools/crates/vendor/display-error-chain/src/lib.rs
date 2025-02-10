//! A lightweight library for displaying errors and their sources.
//!
//! A sample output:
//!
//! ```rust
//! macro_rules! impl_error {
//!     // ...
//! #    ($ty:ty, $display:expr, $source:expr) => {
//! #        impl ::std::fmt::Display for $ty {
//! #            fn fmt(&self, f: &mut ::std::fmt::Formatter<'_>) -> ::std::fmt::Result {
//! #                write!(f, "{}", $display)
//! #            }
//! #        }
//! #
//! #        impl ::std::error::Error for $ty {
//! #            fn source(&self) -> Option<&(dyn ::std::error::Error + 'static)> {
//! #                $source
//! #            }
//! #        }
//! #    };
//! }
//!
//! // `TopLevel` is caused by a `MidLevel`.
//! #[derive(Debug)]
//! struct TopLevel;
//! impl_error!(TopLevel, "top level", Some(&MidLevel));
//!
//! // `MidLevel` is caused by a `LowLevel`.
//! #[derive(Debug)]
//! struct MidLevel;
//! impl_error!(MidLevel, "mid level", Some(&LowLevel));
//!
//! // `LowLevel` is the cause itself.
//! #[derive(Debug)]
//! struct LowLevel;
//! impl_error!(LowLevel, "low level", None);
//!
//! // Now let's see how it works:
//! let formatted = display_error_chain::DisplayErrorChain::new(&TopLevel).to_string();
//! assert_eq!(
//!     formatted,
//!     "\
//!top level
//!Caused by:
//!   -> mid level
//!   -> low level"
//! );
//!
//! // Or with `.chain()` helper:
//! use display_error_chain::ErrorChainExt as _;
//! let formatted = TopLevel.chain().to_string();
//! assert_eq!(
//!     formatted,
//!     "\
//!top level
//!Caused by:
//!   -> mid level
//!   -> low level"
//! );
//!
//! // Or even with `.into_chain()` helper to consume the error.
//! use display_error_chain::ErrorChainExt as _;
//! let formatted = TopLevel.into_chain().to_string();
//! assert_eq!(
//!     formatted,
//!     "\
//!top level
//!Caused by:
//!   -> mid level
//!   -> low level"
//! );
//! ```

use std::{error::Error, fmt};

mod result_ext;
pub use result_ext::ResultExt;

/// Provides an [fmt::Display] implementation for an error as a chain.
///
/// ```rust
/// use display_error_chain::{DisplayErrorChain, ErrorChainExt as _};
///
/// // Let's set up a custom error. Normally one would use `snafu` or
/// // something similar to avoid the boilerplate.
/// #[derive(Debug)]
/// enum CustomError {
///     NoCause,
///     IO { source: std::io::Error },
/// }
///
/// // Custom display implementation (which doesn't take error
/// // sources into consideration).
/// impl std::fmt::Display for CustomError {
///     fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
///         match self {
///             CustomError::NoCause => {
///                 write!(f, "No cause")
///             }
///             CustomError::IO { .. } => {
///                 write!(f, "Some I/O")
///             }
///         }
///     }
/// }
///
/// impl std::error::Error for CustomError {
///     fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
///         match self {
///             CustomError::NoCause => None,
///             CustomError::IO { source } => Some(source),
///         }
///     }
/// }
///
/// // And finally let's see how `DisplayErrorChain` helps!
/// let io = CustomError::IO {
///     source: std::io::Error::new(std::io::ErrorKind::AlreadyExists, "wow"),
/// };
/// let formatted = DisplayErrorChain::new(&io).to_string();
/// assert_eq!("Some I/O\nCaused by:\n  -> wow", formatted);
///
/// let no_cause = CustomError::NoCause;
/// // You can also use a `.chain()` shortcut from the `ErrorChainExt` trait.
/// let formatted = no_cause.chain().to_string();
/// assert_eq!("No cause", formatted);
///
/// // or `.into_chain()` to make the `DisplayErrorChain` to consume the error.
/// let formatted = no_cause.into_chain().to_string();
/// assert_eq!("No cause", formatted);
///
/// // `from` or `into` will also work with both owned and referenced errors:
/// let chain: DisplayErrorChain<_> = CustomError::NoCause.into();
/// assert_eq!("No cause", chain.to_string());
///
/// let chain: DisplayErrorChain<_> = (&CustomError::NoCause).into();
/// assert_eq!("No cause", chain.to_string());
/// ```
///
/// Other standard traits (like [`Debug`][std::fmt::Debug], [`Clone`] and some
/// others) are automatically derived for the convenience using the standard
/// derive macros. If you need another trait, feel free to submit a PR and/or
/// use the [`DisplayErrorChain::into_inner`] method to access the wrapped
/// error.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct DisplayErrorChain<E>(E);

impl<E: Error> From<E> for DisplayErrorChain<E> {
    fn from(value: E) -> Self {
        DisplayErrorChain::new(value)
    }
}

impl<E> DisplayErrorChain<E>
where
    E: Error,
{
    /// Initializes the formatter with the error provided.
    pub fn new(error: E) -> Self {
        DisplayErrorChain(error)
    }

    /// Deconstructs the [`DisplayErrorChain`] and returns the wrapped error.
    pub fn into_inner(self) -> E {
        self.0
    }
}

impl<E> fmt::Display for DisplayErrorChain<E>
where
    E: Error,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)?;

        let mut cause_printed = false;
        let mut source = self.0.source();
        while let Some(cause) = source {
            if !cause_printed {
                cause_printed = true;
                writeln!(f, "\nCaused by:")?;
            } else {
                writeln!(f)?
            }
            write!(f, "  -> {}", cause)?;
            source = cause.source();
        }
        Ok(())
    }
}

/// An extension trait for [`Error`] types to display their sources in a chain.
pub trait ErrorChainExt {
    /// Provides an [fmt::Display] implementation for an error as a chain.
    fn chain(&self) -> DisplayErrorChain<&Self>;

    /// Same as [`chain`][ErrorChainExt::chain], but consumes `self`.
    fn into_chain(self) -> DisplayErrorChain<Self>
    where
        Self: Sized;
}

impl<E> ErrorChainExt for E
where
    E: Error,
{
    fn chain(&self) -> DisplayErrorChain<&Self> {
        DisplayErrorChain::new(self)
    }

    fn into_chain(self) -> DisplayErrorChain<Self>
    where
        Self: Sized,
    {
        DisplayErrorChain::new(self)
    }
}
