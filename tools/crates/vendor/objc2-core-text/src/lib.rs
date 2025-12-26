//! # Bindings to the `CoreText` framework
//!
//! See [Apple's docs][apple-doc] and [the general docs on framework crates][framework-crates] for more information.
//!
//! [apple-doc]: https://developer.apple.com/documentation/coretext/
//! [framework-crates]: https://docs.rs/objc2/latest/objc2/topics/about_generated/index.html
#![no_std]
#![cfg_attr(feature = "unstable-darwin-objc", feature(darwin_objc))]
#![cfg_attr(docsrs, feature(doc_cfg))]
// Update in Cargo.toml as well.
#![doc(html_root_url = "https://docs.rs/objc2-core-text/0.3.2")]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

mod generated;
mod thread_safety;

#[allow(unused_imports, unreachable_pub)]
pub use self::generated::*;

#[allow(dead_code)]
pub(crate) type Fixed = i32;
#[allow(dead_code)]
pub(crate) type FourCharCode = u32;
#[allow(dead_code)]
pub(crate) type ConstStr255Param = *const core::ffi::c_char;
#[allow(dead_code)]
pub(crate) type UniChar = u16;
