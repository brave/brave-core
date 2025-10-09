//! [![github]](https://github.com/dtolnay/monostate)&ensp;[![crates-io]](https://crates.io/crates/monostate)&ensp;[![docs-rs]](https://docs.rs/monostate)
//!
//! [github]: https://img.shields.io/badge/github-8da0cb?style=for-the-badge&labelColor=555555&logo=github
//! [crates-io]: https://img.shields.io/badge/crates.io-fc8d62?style=for-the-badge&labelColor=555555&logo=rust
//! [docs-rs]: https://img.shields.io/badge/docs.rs-66c2a5?style=for-the-badge&labelColor=555555&logo=docs.rs
//!
//! <br>
//!
//! This library implements a type macro for a zero-sized type that is Serde
//! deserializable only from one specific value.
//!
//! # Examples
//!
//! ```
//! use monostate::MustBe;
//! use serde::Deserialize;
//!
//! #[derive(Deserialize)]
//! struct Example {
//!     kind: MustBe!("success"),
//!     code: MustBe!(200),
//! }
//! ```
//!
//! The above struct would deserialize from `{"kind":"success", "code":200}` in
//! JSON, but would fail the deserialization if "kind" or "code" were any other
//! value.
//!
//! This can sometimes be helpful in processing untagged enums in which the
//! variant identification is more convoluted than what is handled by Serde's
//! externally tagged and internally tagged representations, for example because
//! the variant tag has an inconsistent type or key.
//!
//! ```
//! use monostate::MustBe;
//! use serde::Deserialize;
//!
//! #[derive(Deserialize)]
//! #[serde(untagged)]
//! pub enum ApiResponse {
//!     Success {
//!         success: MustBe!(true),
//!     },
//!     Error {
//!         kind: MustBe!("error"),
//!         message: String,
//!     },
//! }
//! ```

#![no_std]
#![doc(html_root_url = "https://docs.rs/monostate/0.1.18")]
#![allow(non_camel_case_types, non_upper_case_globals)]
#![allow(
    clippy::borrow_as_ptr,
    clippy::builtin_type_shadow,
    clippy::cast_lossless,
    clippy::cast_sign_loss,
    clippy::derivable_impls,
    clippy::elidable_lifetime_names,
    clippy::expl_impl_clone_on_copy,
    clippy::missing_safety_doc,
    clippy::module_name_repetitions,
    clippy::needless_lifetimes,
    clippy::ptr_as_ptr,
    clippy::uninhabited_references,
    clippy::uninlined_format_args
)]

extern crate serde_core as serde;

#[doc(hidden)]
pub mod alphabet;
mod debug;
mod default;
mod deserialize;
mod eq;
mod format;
mod hash;
mod ord;
mod partial_eq;
mod partial_ord;
mod serialize;
mod string;
mod value;

pub use crate::string::ConstStr;
pub use crate::value::MustBe;
pub use monostate_impl::MustBe;

#[derive(Copy, Clone)]
pub struct MustBeChar<const V: char>;

#[derive(Copy, Clone)]
#[doc(hidden)]
pub struct MustBePosInt<const V: u128>;

#[derive(Copy, Clone)]
#[doc(hidden)]
pub struct MustBeNegInt<const V: i128>;

#[derive(Copy, Clone)]
pub struct MustBeU8<const V: u8>;

#[derive(Copy, Clone)]
pub struct MustBeU16<const V: u16>;

#[derive(Copy, Clone)]
pub struct MustBeU32<const V: u32>;

#[derive(Copy, Clone)]
pub struct MustBeU64<const V: u64>;

#[derive(Copy, Clone)]
pub struct MustBeU128<const V: u128>;

#[derive(Copy, Clone)]
pub struct MustBeI8<const V: i8>;

#[derive(Copy, Clone)]
pub struct MustBeI16<const V: i16>;

#[derive(Copy, Clone)]
pub struct MustBeI32<const V: i32>;

#[derive(Copy, Clone)]
pub struct MustBeI64<const V: i64>;

#[derive(Copy, Clone)]
pub struct MustBeI128<const V: i128>;

#[derive(Copy, Clone)]
pub struct MustBeBool<const V: bool>;

#[allow(type_alias_bounds)]
pub type MustBeStr<V: ConstStr> = crate::string::MustBeStr<V>;

impl<V: ConstStr> Copy for MustBeStr<V> {}

impl<V: ConstStr> Clone for MustBeStr<V> {
    fn clone(&self) -> Self {
        *self
    }
}

#[doc(hidden)]
pub use self::string::value::*;
