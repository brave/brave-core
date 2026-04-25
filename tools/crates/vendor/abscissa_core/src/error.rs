//! Error types used by this crate

#[macro_use]
pub mod macros;

pub mod context;
pub mod framework;
pub mod message;

pub use self::{context::Context, message::Message};

/// Box containing a thread-safe + `'static` error suitable for use as a
/// as an `std::error::Error::source`
pub type BoxError = Box<dyn std::error::Error + Send + Sync + 'static>;
