//! Tracing subsystem

#[cfg(feature = "application")]
pub mod component;
mod config;

#[cfg(feature = "application")]
pub use self::component::Tracing;
pub use self::config::Config;
