//! Provides helpers for making ptrace system calls

#[cfg(linux_android)]
mod linux;

#[cfg(linux_android)]
pub use self::linux::*;

#[cfg(any(bsd_without_apple, target_os = "macos"))]
mod bsd;

#[cfg(any(bsd_without_apple, target_os = "macos"))]
pub use self::bsd::*;
