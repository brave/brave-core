#![doc = include_str!("../README.md")]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/iqlusioninc/abscissa/main/img/abscissa-sq.svg"
)]
#![forbid(unsafe_code)]
#![warn(
    missing_docs,
    rust_2018_idioms,
    unused_lifetimes,
    unused_qualifications
)]

/// Abscissa version
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

#[cfg(feature = "trace")]
#[allow(unused_imports)]
#[macro_use]
pub extern crate tracing;

// Modules with macro exports

#[macro_use]
pub mod error;
#[cfg(feature = "terminal")]
#[macro_use]
pub mod terminal;

// Other modules

#[cfg(feature = "application")]
pub mod application;
#[cfg(feature = "options")]
pub mod command;
#[cfg(feature = "application")]
pub mod component;
#[cfg(feature = "config")]
pub mod config;
pub mod path;
#[cfg(feature = "application")]
pub mod prelude;
mod runnable;
#[cfg(feature = "application")]
mod shutdown;
#[cfg(feature = "testing")]
pub mod testing;
pub mod thread;
#[cfg(feature = "trace")]
pub mod trace;

// Re-exports

pub use crate::{
    error::framework::{FrameworkError, FrameworkErrorKind},
    runnable::{Runnable, RunnableMut},
};
pub use std::collections::{btree_map as map, btree_set as set, BTreeMap as Map};

#[cfg(feature = "application")]
pub use crate::{
    application::{boot, Application},
    component::Component,
    shutdown::Shutdown,
};

#[cfg(feature = "config")]
pub use crate::config::{Config, Configurable};

#[cfg(feature = "options")]
pub use crate::{command::Command, path::StandardPaths};

// Re-exported modules/types from third-party crates

#[cfg(feature = "options")]
pub use clap;
pub use fs_err as fs;
#[cfg(feature = "secrets")]
pub use secrecy as secret;
#[cfg(feature = "secrets")]
pub use secrecy::{SecretBox, SecretSlice, SecretString};
#[cfg(feature = "application")]
pub use semver::Version;
