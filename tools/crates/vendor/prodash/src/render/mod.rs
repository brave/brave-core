#[cfg(feature = "render-tui")]
///
pub mod tui;
#[cfg(feature = "render-tui")]
pub use self::tui::render as tui;

#[cfg(feature = "render-line")]
///
pub mod line;
#[cfg(feature = "render-line")]
pub use self::line::render as line;
