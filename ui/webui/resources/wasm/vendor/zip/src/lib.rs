//! A library for reading and writing ZIP archives.
//! ZIP is a format designed for cross-platform file "archiving".
//! That is, storing a collection of files in a single datastream
//! to make them easier to share between computers.
//! Additionally, ZIP is able to compress and encrypt files in its
//! archives.
//!
//! The current implementation is based on [PKWARE's APPNOTE.TXT v6.3.9](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT)
//!
//! ---
//!
//! [`zip`](`crate`) has support for the most common ZIP archives found in common use.
//! However, in special cases,
//! there are some zip archives that are difficult to read or write.
//!
//! This is a list of supported features:
//!
//! |         | Reading | Writing |
//! | ------- | ------  | ------- |
//! | Deflate | ✅ [->](`crate::ZipArchive::by_name`)      | ✅ [->](`crate::write::FileOptions::compression_method`) |
//! | Deflate64 | ✅ | |
//! | Bzip2 | ✅ | ✅ |
//! | LZMA | ✅ | |
//! | AES encryption | ✅ | ✅ |
//! | ZipCrypto deprecated encryption | ✅ | ✅ |
//!
//!
#![warn(missing_docs)]

pub use crate::compression::{CompressionMethod, SUPPORTED_COMPRESSION_METHODS};
pub use crate::read::ZipArchive;
pub use crate::types::DateTime;
pub use crate::write::ZipWriter;

#[cfg(feature = "aes-crypto")]
mod aes;
#[cfg(feature = "aes-crypto")]
mod aes_ctr;
mod compression;
mod cp437;
mod crc32;
pub mod extra_fields;
pub mod read;
pub mod result;
mod spec;
mod types;
pub mod write;
mod zipcrypto;
pub use extra_fields::ExtraField;

#[doc = "Unstable APIs\n\
\
All APIs accessible by importing this module are unstable; They may be changed in patch \
releases. You MUST use an exact version specifier in `Cargo.toml`, to indicate the version of this \
API you're using:\n\
\
```toml\n
[dependencies]\n
zip = \"="]
#[doc=env!("CARGO_PKG_VERSION")]
#[doc = "\"\n\
```"]
pub mod unstable;
