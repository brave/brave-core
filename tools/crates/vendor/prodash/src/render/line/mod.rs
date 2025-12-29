#[cfg(all(feature = "render-line", not(any(feature = "render-line-crossterm"))))]
compile_error!("Please use the 'render-line-crossterm' feature");

mod draw;
mod engine;

pub use engine::{render, JoinHandle, Options, StreamKind};
