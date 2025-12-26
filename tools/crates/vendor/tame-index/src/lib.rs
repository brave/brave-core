#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![doc = include_str!("../README.md")]

pub mod error;
pub mod index;
pub mod krate;
mod krate_name;
pub mod utils;

pub use camino::{Utf8Path as Path, Utf8PathBuf as PathBuf};

pub use error::{CacheError, Error, HttpError, InvalidUrl, InvalidUrlError};
pub use index::{
    GitIndex, IndexCache, IndexLocation, IndexPath, IndexUrl, SparseIndex, git::CRATES_IO_INDEX,
    sparse::CRATES_IO_HTTP_INDEX,
};
pub use krate::{IndexDependency, IndexKrate, IndexVersion};
pub use krate_name::KrateName;
pub use semver::Version;

/// Reexports of some crates for easier downstream usage without requiring adding
/// your own dependencies
pub mod external {
    #[cfg(feature = "__git")]
    pub use gix;
    pub use http;
    #[cfg(any(feature = "sparse", feature = "local-builder"))]
    pub use reqwest;
    #[cfg(any(feature = "sparse", feature = "local-builder"))]
    pub use tokio;
}
