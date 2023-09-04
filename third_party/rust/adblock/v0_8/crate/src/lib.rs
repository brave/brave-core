//! `adblock-rust` is the engine powering Brave's native adblocker, available as a library for
//! anyone to use. It features:
//!
//! - Network blocking
//! - Cosmetic filtering
//! - Resource replacements
//! - Hosts syntax
//! - uBlock Origin syntax extensions
//! - iOS content-blocking syntax conversion
//! - Compiling to native code or WASM
//! - Rust bindings ([crates](https://crates.io/crates/adblock))
//! - JS bindings ([npm](https://npmjs.com/adblock-rs))
//! - Community-maintained Python bindings ([pypi](https://pypi.org/project/adblock/))
//! - High performance!
//!
//! Check the [`Engine`] documentation to get started with adblocking.

// Own modules, currently everything is exposed, will need to limit
pub mod blocker;
#[cfg(feature = "content-blocking")]
pub mod content_blocking;
pub mod cosmetic_filter_cache;
mod data_format;
mod engine;
pub mod filters;
pub mod lists;
mod optimizer;
pub mod regex_manager;
pub mod request;
pub mod resources;
pub mod url_parser;
#[doc(hidden)]
pub mod utils;

#[doc(inline)]
pub use engine::Engine;
#[doc(inline)]
pub use lists::FilterSet;

#[cfg(test)]
#[path = "../tests/test_utils.rs"]
mod test_utils;

#[cfg(test)]
mod sync_tests {
    #[allow(unused)]
    fn static_assert_sync<S: Sync>() {
        let _ = core::marker::PhantomData::<S>::default();
    }

    #[test]
    #[cfg(not(any(feature = "object-pooling", feature = "unsync-regex-caching")))]
    fn assert_engine_sync() {
        static_assert_sync::<crate::engine::Engine>();
    }
}
