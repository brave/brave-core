#[cfg(feature = "local")]
use crate::index::LocalRegistry;
use crate::{
    Error, IndexKrate, KrateName,
    index::{FileLock, RemoteGitIndex, RemoteSparseIndex},
};

/// A wrapper around either a [`RemoteGitIndex`] or [`RemoteSparseIndex`]
#[non_exhaustive]
pub enum ComboIndex {
    /// A standard git based registry index. No longer the default for crates.io
    /// as of 1.70.0
    Git(Box<RemoteGitIndex>),
    /// An HTTP sparse index
    Sparse(RemoteSparseIndex),
    /// A local registry
    #[cfg(feature = "local")]
    Local(LocalRegistry),
}

impl ComboIndex {
    /// Retrieves the index metadata for the specified crate name, optionally
    /// writing a cache entry for it if there was not already an up to date one
    ///
    /// Note no cache entry is written if this is a `Local` registry as they do
    /// not use .cache files
    #[inline]
    pub fn krate(
        &self,
        name: KrateName<'_>,
        write_cache_entry: bool,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        match self {
            Self::Git(index) => index.krate(name, write_cache_entry, lock),
            Self::Sparse(index) => index.krate(name, write_cache_entry, lock),
            #[cfg(feature = "local")]
            Self::Local(lr) => lr.cached_krate(name, lock),
        }
    }

    /// Retrieves the cached crate metadata if it exists
    #[inline]
    pub fn cached_krate(
        &self,
        name: KrateName<'_>,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        match self {
            Self::Git(index) => index.cached_krate(name, lock),
            Self::Sparse(index) => index.cached_krate(name, lock),
            #[cfg(feature = "local")]
            Self::Local(lr) => lr.cached_krate(name, lock),
        }
    }
}

impl From<RemoteGitIndex> for ComboIndex {
    #[inline]
    fn from(index: RemoteGitIndex) -> Self {
        Self::Git(Box::new(index))
    }
}

impl From<RemoteSparseIndex> for ComboIndex {
    #[inline]
    fn from(index: RemoteSparseIndex) -> Self {
        Self::Sparse(index)
    }
}

#[cfg(feature = "local")]
impl From<LocalRegistry> for ComboIndex {
    #[inline]
    fn from(local: LocalRegistry) -> Self {
        Self::Local(local)
    }
}
