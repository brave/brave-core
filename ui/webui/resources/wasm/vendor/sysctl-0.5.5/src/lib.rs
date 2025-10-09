// lib.rs

//! A simplified interface to the `sysctl` system call.
//!
//! # Example: Get value
//! ```
//! # use sysctl::Sysctl;
//! #[cfg(any(target_os = "macos", target_os = "ios", target_os = "freebsd"))]
//! const CTLNAME: &str = "kern.ostype";
//! #[cfg(any(target_os = "linux", target_os = "android"))]
//! const CTLNAME: &str = "kernel.ostype";
//!
//! let ctl = sysctl::Ctl::new(CTLNAME).unwrap();
//! let desc = ctl.description().unwrap();
//! println!("Description: {}", desc);
//! let val = ctl.value().unwrap();
//! println!("Value: {}", val);
//! // On Linux all sysctls are String type. Use the following for
//! // cross-platform compatibility:
//! let str_val = ctl.value_string().unwrap();
//! println!("String value: {}", str_val);
//! ```
//!
//! # Example: Get value as struct
//! ```
//! // Not available on Linux
//! # use sysctl::Sysctl;
//! #[derive(Debug, Default)]
//! #[repr(C)]
//! struct ClockInfo {
//!     hz: libc::c_int, /* clock frequency */
//!     tick: libc::c_int, /* micro-seconds per hz tick */
//!     spare: libc::c_int,
//!     stathz: libc::c_int, /* statistics clock frequency */
//!     profhz: libc::c_int, /* profiling clock frequency */
//! }
//! # #[cfg(any(target_os = "macos", target_os = "ios", target_os = "freebsd"))]
//! let val: Box<ClockInfo> = sysctl::Ctl::new("kern.clockrate").unwrap().value_as().unwrap();
//! # #[cfg(any(target_os = "macos", target_os = "ios", target_os = "freebsd"))]
//! println!("{:?}", val);
//! ```

#[macro_use]
extern crate bitflags;
extern crate byteorder;
extern crate enum_as_inner;
extern crate libc;
extern crate thiserror;

#[cfg(any(target_os = "android", target_os = "linux"))]
extern crate walkdir;

#[cfg(any(target_os = "android", target_os = "linux"))]
#[path = "linux/mod.rs"]
mod sys;

#[cfg(any(target_os = "macos", target_os = "ios", target_os = "freebsd"))]
#[path = "unix/mod.rs"]
mod sys;

mod consts;
mod ctl_error;
mod ctl_flags;
mod ctl_info;
mod ctl_type;
mod ctl_value;
#[cfg(target_os = "freebsd")]
mod temperature;
mod traits;

pub use consts::*;
pub use ctl_error::*;
pub use ctl_flags::*;
pub use ctl_info::*;
pub use ctl_type::*;
pub use ctl_value::*;
pub use sys::ctl::*;
pub use sys::ctl_iter::*;
#[cfg(target_os = "freebsd")]
pub use temperature::Temperature;
pub use traits::Sysctl;
