//! Provides functionality for interacting with both local and remote registry
//! indices

pub mod cache;
#[cfg(all(feature = "__git", feature = "sparse"))]
mod combo;
#[allow(missing_docs)]
pub mod git;
#[cfg(feature = "__git")]
pub(crate) mod git_remote;
#[cfg(feature = "local")]
pub mod local;
pub mod location;
#[allow(missing_docs)]
pub mod sparse;
#[cfg(feature = "sparse")]
mod sparse_remote;

pub use cache::IndexCache;
#[cfg(all(feature = "__git", feature = "sparse"))]
pub use combo::ComboIndex;
pub use git::GitIndex;
#[cfg(feature = "__git")]
pub use git_remote::RemoteGitIndex;
#[cfg(feature = "local")]
pub use local::LocalRegistry;
pub use location::{IndexLocation, IndexPath, IndexUrl};
pub use sparse::SparseIndex;
#[cfg(feature = "sparse")]
pub use sparse_remote::{AsyncRemoteSparseIndex, RemoteSparseIndex};

pub use crate::utils::flock::FileLock;

/// Global configuration of an index, reflecting the [contents of config.json](https://doc.rust-lang.org/cargo/reference/registries.html#index-format).
#[derive(Eq, PartialEq, PartialOrd, Ord, Clone, Debug, serde::Deserialize, serde::Serialize)]
pub struct IndexConfig {
    /// Pattern for creating download URLs. See [`Self::download_url`].
    pub dl: String,
    #[serde(default)]
    /// Base URL for publishing, etc.
    pub api: Option<String>,
    /// Indicates whether this is a private registry that requires all
    /// operations to be authenticated including API requests, crate downloads
    /// and sparse index updates.
    #[serde(default, rename = "auth-required")]
    pub auth_required: bool,
}

impl IndexConfig {
    /// Gets the download url for the specified crate version
    ///
    /// See <https://doc.rust-lang.org/cargo/reference/registries.html#index-format>
    /// for more info
    pub fn download_url(&self, name: crate::KrateName<'_>, version: &str) -> String {
        // Special case crates.io which will easily be the most common case in
        // almost all scenarios, we just use the _actual_ url directly, which
        // avoids a 301 redirect, though obviously this will be bad if crates.io
        // ever changes the redirect, this has been stable since 1.0 (at least)
        // so it's unlikely to ever change, and if it does, it would be easy to
        // update, though obviously would be broken on previously published versions
        if self.dl == "https://crates.io/api/v1/crates" {
            return format!("https://static.crates.io/crates/{name}/{name}-{version}.crate");
        }

        let mut dl = self.dl.clone();

        if dl.contains('{') {
            while let Some(start) = dl.find("{crate}") {
                dl.replace_range(start..start + 7, name.0);
            }

            while let Some(start) = dl.find("{version}") {
                dl.replace_range(start..start + 9, version);
            }

            if dl.contains("{prefix}") || dl.contains("{lowerprefix}") {
                let mut prefix = String::with_capacity(6);
                name.prefix(&mut prefix, '/');

                while let Some(start) = dl.find("{prefix}") {
                    dl.replace_range(start..start + 8, &prefix);
                }

                if dl.contains("{lowerprefix}") {
                    prefix.make_ascii_lowercase();

                    while let Some(start) = dl.find("{lowerprefix}") {
                        dl.replace_range(start..start + 13, &prefix);
                    }
                }
            }
        } else {
            // If none of the markers are present, then the value /{crate}/{version}/download is appended to the end
            if !dl.ends_with('/') {
                dl.push('/');
            }

            dl.push_str(name.0);
            dl.push('/');
            dl.push_str(version);
            dl.push('/');
            dl.push_str("download");
        }

        dl
    }
}

use crate::Error;

/// Provides simpler access to the cache for an index, regardless of the registry kind
#[non_exhaustive]
pub enum ComboIndexCache {
    /// A git index
    Git(GitIndex),
    /// A sparse HTTP index
    Sparse(SparseIndex),
    /// A local registry
    #[cfg(feature = "local")]
    Local(LocalRegistry),
}

impl ComboIndexCache {
    /// Retrieves the index metadata for the specified crate name
    #[inline]
    pub fn cached_krate(
        &self,
        name: crate::KrateName<'_>,
        lock: &FileLock,
    ) -> Result<Option<crate::IndexKrate>, Error> {
        match self {
            Self::Git(index) => index.cached_krate(name, lock),
            Self::Sparse(index) => index.cached_krate(name, lock),
            #[cfg(feature = "local")]
            Self::Local(lr) => lr.cached_krate(name, lock),
        }
    }

    /// Gets the path to the cache entry for the specified crate
    pub fn cache_path(&self, name: crate::KrateName<'_>) -> crate::PathBuf {
        match self {
            Self::Git(index) => index.cache.cache_path(name),
            Self::Sparse(index) => index.cache().cache_path(name),
            #[cfg(feature = "local")]
            Self::Local(lr) => lr.krate_path(name),
        }
    }

    /// Constructs a [`Self`] for the specified index.
    ///
    /// See [`Self::crates_io`] if you want to create a crates.io index based
    /// upon other information in the user's environment
    pub fn new(il: IndexLocation<'_>) -> Result<Self, Error> {
        #[cfg(feature = "local")]
        {
            if let IndexUrl::Local(path) = il.url {
                return Ok(Self::Local(LocalRegistry::open(path.into(), true)?));
            }
        }

        let index = if il.url.is_sparse() {
            let sparse = SparseIndex::new(il)?;
            Self::Sparse(sparse)
        } else {
            let git = GitIndex::new(il)?;
            Self::Git(git)
        };

        Ok(index)
    }
}

impl From<SparseIndex> for ComboIndexCache {
    #[inline]
    fn from(si: SparseIndex) -> Self {
        Self::Sparse(si)
    }
}

impl From<GitIndex> for ComboIndexCache {
    #[inline]
    fn from(gi: GitIndex) -> Self {
        Self::Git(gi)
    }
}

#[cfg(test)]
mod test {
    use super::IndexConfig;
    use crate::kn;

    /// Validates we get the non-redirect url for crates.io downloads
    #[test]
    fn download_url_crates_io() {
        let crates_io = IndexConfig {
            dl: "https://crates.io/api/v1/crates".into(),
            api: Some("https://crates.io".into()),
            auth_required: false,
        };

        assert_eq!(
            crates_io.download_url(kn!("a"), "1.0.0"),
            "https://static.crates.io/crates/a/a-1.0.0.crate"
        );
        assert_eq!(
            crates_io.download_url(kn!("aB"), "0.1.0"),
            "https://static.crates.io/crates/aB/aB-0.1.0.crate"
        );
        assert_eq!(
            crates_io.download_url(kn!("aBc"), "0.1.0"),
            "https://static.crates.io/crates/aBc/aBc-0.1.0.crate"
        );
        assert_eq!(
            crates_io.download_url(kn!("aBc-123"), "0.1.0"),
            "https://static.crates.io/crates/aBc-123/aBc-123-0.1.0.crate"
        );
    }

    /// Validates we get a simple non-crates.io download
    #[test]
    fn download_url_non_crates_io() {
        let ic = IndexConfig {
            dl: "https://dl.cloudsmith.io/public/embark/deny/cargo/{crate}-{version}.crate".into(),
            api: Some("https://cargo.cloudsmith.io/embark/deny".into()),
            auth_required: false,
        };

        assert_eq!(
            ic.download_url(kn!("a"), "1.0.0"),
            "https://dl.cloudsmith.io/public/embark/deny/cargo/a-1.0.0.crate"
        );
        assert_eq!(
            ic.download_url(kn!("aB"), "0.1.0"),
            "https://dl.cloudsmith.io/public/embark/deny/cargo/aB-0.1.0.crate"
        );
        assert_eq!(
            ic.download_url(kn!("aBc"), "0.1.0"),
            "https://dl.cloudsmith.io/public/embark/deny/cargo/aBc-0.1.0.crate"
        );
        assert_eq!(
            ic.download_url(kn!("aBc-123"), "0.1.0"),
            "https://dl.cloudsmith.io/public/embark/deny/cargo/aBc-123-0.1.0.crate"
        );
    }

    /// Validates we get a more complicated non-crates.io download, exercising all
    /// of the possible replacement components
    #[test]
    fn download_url_complex() {
        let ic = IndexConfig {
            dl: "https://complex.io/ohhi/embark/rust/cargo/{lowerprefix}/{crate}/{crate}/{prefix}-{version}".into(),
            api: None,
            auth_required: false,
        };

        assert_eq!(
            ic.download_url(kn!("a"), "1.0.0"),
            "https://complex.io/ohhi/embark/rust/cargo/1/a/a/1-1.0.0"
        );
        assert_eq!(
            ic.download_url(kn!("aB"), "0.1.0"),
            "https://complex.io/ohhi/embark/rust/cargo/2/aB/aB/2-0.1.0"
        );
        assert_eq!(
            ic.download_url(kn!("ABc"), "0.1.0"),
            "https://complex.io/ohhi/embark/rust/cargo/3/a/ABc/ABc/3/A-0.1.0"
        );
        assert_eq!(
            ic.download_url(kn!("aBc-123"), "0.1.0"),
            "https://complex.io/ohhi/embark/rust/cargo/ab/c-/aBc-123/aBc-123/aB/c--0.1.0"
        );
    }
}
