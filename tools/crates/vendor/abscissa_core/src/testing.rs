//! Acceptance testing for Abscissa applications.
//!
//! The recommended way to import types for testing is:
//!
//! ```
//! use abscissa_core::testing::prelude::*;
//! ```
//!
//! The main entrypoint for running tests is [`CmdRunner`].

mod config;
pub mod prelude;
pub mod process;
mod regex;
mod runner;

pub use self::{regex::Regex, runner::CmdRunner};
