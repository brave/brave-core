#[cfg(all(
    feature = "render-line",
    not(any(feature = "render-line-crossterm", feature = "render-line-termion"))
))]
compile_error!("Please choose either one of these features: 'render-line-crossterm' or 'render-line-termion'");

mod draw;
mod engine;

pub use engine::{render, JoinHandle, Options, StreamKind};
