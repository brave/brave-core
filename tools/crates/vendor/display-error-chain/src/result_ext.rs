use std::{error::Error, fmt};

use crate::DisplayErrorChain;

/// A helper extension trait to "unwrap" results with an error chain.
pub trait ResultExt<T, E> {
    /// Like [`Result::unwrap`][Result::unwrap], but wraps the error in the
    /// [DisplayErrorChain] and prints out the full chain in case an `Err(_)`
    /// value is encountered.
    #[track_caller]
    fn unwrap_chain(self) -> T;

    /// Like [`Result::expect`][Result::expect], but wraps the error in the
    /// [DisplayErrorChain] and prints out the full chain in case an `Err(_)`
    /// value is encountered.
    #[track_caller]
    fn expect_chain(self, msg: &str) -> T;
}

impl<T, E: Error> ResultExt<T, E> for Result<T, E> {
    #[inline]
    #[track_caller]
    fn unwrap_chain(self) -> T {
        match self {
            Ok(value) => value,
            Err(e) => unwrap_failed(
                "called `Result::unwrap_chain()` on an `Err` value",
                &DisplayErrorChain::new(&e),
            ),
        }
    }
    #[inline]
    #[track_caller]
    fn expect_chain(self, msg: &str) -> T {
        match self {
            Ok(value) => value,
            Err(e) => unwrap_failed(msg, &DisplayErrorChain::new(&e)),
        }
    }
}

// This is a separate function to reduce the code size of the methods
#[inline(never)]
#[cold]
#[track_caller]
fn unwrap_failed(msg: &str, error: &dyn fmt::Display) -> ! {
    panic!("{}: {}", msg, error)
}

#[cfg(test)]
mod test {
    use super::ResultExt;

    macro_rules! impl_error {
        ($ty:ty, $display:expr, $source:expr) => {
            impl ::std::fmt::Display for $ty {
                fn fmt(&self, f: &mut ::std::fmt::Formatter<'_>) -> ::std::fmt::Result {
                    write!(f, "{}", $display)
                }
            }

            impl ::std::error::Error for $ty {
                fn source(&self) -> Option<&(dyn ::std::error::Error + 'static)> {
                    $source
                }
            }
        };
    }

    // `TopLevel` is caused by a `MidLevel`.
    #[derive(Debug)]
    struct TopLevel;
    impl_error!(TopLevel, "top level", Some(&MidLevel));

    // `MidLevel` is caused by a `LowLevel`.
    #[derive(Debug)]
    struct MidLevel;
    impl_error!(MidLevel, "mid level", Some(&LowLevel));

    // `LowLevel` is the cause itself.
    #[derive(Debug)]
    struct LowLevel;
    impl_error!(LowLevel, "low level", None);

    #[test]
    #[should_panic(expected = "\
    called `Result::unwrap_chain()` on an `Err` value: top level\n\
Caused by:
  -> mid level
  -> low level")]
    fn test_unwrap() {
        Err::<(), _>(TopLevel).unwrap_chain();
    }

    #[test]
    #[should_panic(expected = "\
    Some message: top level\n\
Caused by:
  -> mid level
  -> low level")]
    fn test_expect() {
        Err::<(), _>(TopLevel).expect_chain("Some message");
    }
}
