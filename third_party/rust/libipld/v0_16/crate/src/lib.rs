//! The `Ipld` crate.
#![deny(missing_docs)]
#![deny(warnings)]

pub mod block;
pub mod codec_impl;
pub mod path;
pub mod prelude;
pub mod store;

#[cfg(feature = "dag-cbor")]
pub use libipld_cbor as cbor;
#[cfg(all(feature = "dag-cbor", feature = "derive"))]
pub use libipld_cbor_derive::DagCbor;
pub use libipld_core::*;
#[cfg(feature = "dag-json")]
pub use libipld_json as json;
pub use libipld_macro::*;
#[cfg(feature = "dag-pb")]
pub use libipld_pb as pb;

pub use block::Block;
pub use cid::Cid;
pub use codec_impl::IpldCodec;
pub use error::Result;
pub use ipld::Ipld;
pub use link::Link;
pub use multihash::Multihash;
pub use path::{DagPath, Path};
pub use store::DefaultParams;
