//! DAG-CBOR serialization and deserialization.
//!
//! # Usage
//!
//! Add this to your `Cargo.toml`:
//! ```toml
//! [dependencies]
//! serde_ipld_dagcbor = "0.1.0"
//! ```
//!
//! Storing and loading Rust types is easy and requires only
//! minimal modifications to the program code.
//!
//! ```rust
//! # #[cfg(not(feature = "std"))]
//! # fn main() {}
//! use serde_derive::{Deserialize, Serialize};
//! use std::error::Error;
//! use std::fs::File;
//! use std::io::BufReader;
//!
//! // Types annotated with `Serialize` can be stored as CBOR.
//! // To be able to load them again add `Deserialize`.
//! #[derive(Debug, Serialize, Deserialize)]
//! struct Mascot {
//!     name: String,
//!     species: String,
//!     year_of_birth: u32,
//! }
//!
//! # #[cfg(feature = "std")]
//! fn main() -> Result<(), Box<dyn Error>> {
//!     let ferris = Mascot {
//!         name: "Ferris".to_owned(),
//!         species: "crab".to_owned(),
//!         year_of_birth: 2015,
//!     };
//!
//!     let ferris_file = File::create("examples/ferris.cbor")?;
//!     // Write Ferris to the given file.
//!     // Instead of a file you can use any type that implements `io::Write`
//!     // like a HTTP body, database connection etc.
//!     serde_ipld_dagcbor::to_writer(ferris_file, &ferris)?;
//!
//!     let tux_file = File::open("examples/tux.cbor")?;
//!     let tux_reader = BufReader::new(tux_file);
//!     // Load Tux from a file.
//!     // Serde CBOR performs roundtrip serialization meaning that
//!     // the data will not change in any way.
//!     let tux: Mascot = serde_ipld_dagcbor::from_reader(tux_reader)?;
//!
//!     println!("{:?}", tux);
//!     // prints: Mascot { name: "Tux", species: "penguin", year_of_birth: 1996 }
//!
//!     Ok(())
//! }
//! ```
//!
//! There are a lot of options available to customize the format.
//! To operate on untyped DAG-CBOR values have a look at the [`ipld_core::ipld::Ipld`] type.
//!
//! # Type-based Serialization and Deserialization
//! Serde provides a mechanism for low boilerplate serialization & deserialization of values to and
//! from CBOR via the serialization API. To be able to serialize a piece of data, it must implement
//! the `serde::Serialize` trait. To be able to deserialize a piece of data, it must implement the
//! `serde::Deserialize` trait. Serde provides an annotation to automatically generate the
//! code for these traits: `#[derive(Serialize, Deserialize)]`.
//!
//! Read a general CBOR value with an unknown content.
//!
//! ```rust
//! use serde_ipld_dagcbor::from_slice;
//! use ipld_core::ipld::Ipld;
//!
//! let slice = b"\x82\x01\xa1aaab";
//! let value: Ipld = from_slice(slice).unwrap();
//! println!("{:?}", value); // List([Integer(1), Map({"a": String("b")})])
//! ```
//!
//! Serialize an object.
//!
//! ```rust
//! use std::collections::BTreeMap;
//! use serde_ipld_dagcbor::to_vec;
//!
//! let mut programming_languages = BTreeMap::new();
//! programming_languages.insert("rust", vec!["safe", "concurrent", "fast"]);
//! programming_languages.insert("python", vec!["powerful", "friendly", "open"]);
//! programming_languages.insert("js", vec!["lightweight", "interpreted", "object-oriented"]);
//! let encoded = to_vec(&programming_languages);
//! assert_eq!(encoded.unwrap().len(), 103);
//! ```
//!
//! # `no-std` support
//!
//! Serde CBOR supports building in a `no_std` context, use the following lines
//! in your `Cargo.toml` dependencies:
//! ``` toml
//! [dependencies]
//! serde = { version = "1.0", default-features = false }
//! serde_ipld_dagcbor = { version = "0.1.0", default-features = false }
//! ```
//!
//! Without the `std` feature the functions [from_reader], and [to_writer] are not exported.
//!
//! *Note*: to use derive macros in serde you will need to declare `serde`
//! dependency like so:
//! ``` toml
//! serde = { version = "1.0", default-features = false, features = ["derive"] }
//! ```

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

mod cbor4ii_nonpub;
// The `Codec` implementation is only available if the `no-cid-as-bytes` feature is disabled, due
// to the links being extracted with a Serde based approach.
#[cfg(all(feature = "std", not(feature = "no-cid-as-bytes"), feature = "codec"))]
pub mod codec;
pub mod de;
pub mod error;
pub mod ser;

#[doc(inline)]
pub use crate::error::{DecodeError, EncodeError};

// Convenience functions for serialization and deserialization.
#[doc(inline)]
pub use crate::de::from_slice;

#[cfg(feature = "std")]
#[doc(inline)]
pub use crate::de::from_reader;

#[doc(inline)]
pub use crate::ser::to_vec;

#[cfg(feature = "std")]
#[doc(inline)]
pub use crate::ser::to_writer;

/// The CBOR tag that is used for CIDs.
const CBOR_TAGS_CID: u64 = 42;
