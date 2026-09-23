//! Terminal handling (TTY interactions, colors, etc)

#[cfg(feature = "application")]
pub mod component;
#[macro_use]
pub mod status;
pub mod streams;

pub use self::streams::Streams;
pub use termcolor::{Color, ColorChoice, StandardStream};

use once_cell::sync::OnceCell;

/// Terminal streams
static STREAMS: OnceCell<Streams> = OnceCell::new();

/// Initialize the terminal subsystem, registering the [`Streams`] static
pub(crate) fn init(color_choice: ColorChoice) {
    STREAMS
        .set(Streams::new(color_choice))
        .unwrap_or_else(|_| panic!("terminal streams already initialized!"));
}

/// Get the terminal [`Streams`].
pub fn streams() -> &'static Streams {
    STREAMS
        .get()
        .expect("terminal streams not yet initialized!")
}

/// Get the standard output stream
pub fn stdout() -> &'static StandardStream {
    &streams().stdout
}

/// Get the standard error stream
pub fn stderr() -> &'static StandardStream {
    &streams().stderr
}
