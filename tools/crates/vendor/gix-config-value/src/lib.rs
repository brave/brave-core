//! Parsing for data types used in `git-config` files to allow their use from environment variables and other sources.
//!
//! ## Examples
//!
//! ```
//! use bstr::ByteSlice;
//! use gix_config_value::{Boolean, Integer, Path};
//!
//! let auto_crlf: bool = Boolean::try_from("true")?.into();
//! assert!(auto_crlf);
//!
//! let packed_limit: usize = Integer::from_bytes("10m")?;
//! assert_eq!(packed_limit, 10 * 1024 * 1024);
//!
//! let ignore_revs = Path::from(":(optional)~/.git-blame-ignore-revs");
//! assert!(ignore_revs.is_optional);
//! assert_eq!(ignore_revs.value.as_bstr(), "~/.git-blame-ignore-revs");
//! # Ok::<(), gix_error::Error>(())
//! ```
//!
//! ## Feature Flags
#![cfg_attr(
    all(doc, feature = "document-features"),
    doc = ::document_features::document_features!()
)]
#![cfg_attr(all(doc, feature = "document-features"), feature(doc_cfg))]
#![deny(missing_docs, unsafe_code)]

mod boolean;
/// Color value parsing and the supported color names and attributes.
pub mod color;
/// Integer suffix parsing and conversion support.
pub mod integer;
/// Path interpolation support.
pub mod path;

mod types;
pub use types::{Boolean, Color, Integer, Path};
