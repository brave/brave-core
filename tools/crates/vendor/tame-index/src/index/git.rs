use super::{FileLock, IndexCache};
use crate::{Error, IndexKrate, KrateName, PathBuf};

/// The URL of the crates.io index for use with git
pub const CRATES_IO_INDEX: &str = "https://github.com/rust-lang/crates.io-index";

/// Allows access to a cargo git registry index
///
/// Uses Cargo's cache.
pub struct GitIndex {
    pub(super) cache: IndexCache,
    #[allow(dead_code)]
    pub(super) url: String,
    /// The sha-1 head commit id encoded as hex
    pub head: Option<[u8; 40]>,
}

impl GitIndex {
    /// Creates a new git index for the specified location
    #[inline]
    pub fn new(il: crate::index::IndexLocation<'_>) -> Result<Self, Error> {
        if il.url.is_sparse() {
            return Err(crate::InvalidUrl {
                url: il.url.as_str().to_owned(),
                source: crate::InvalidUrlError::SparseForGit,
            }
            .into());
        }

        let (path, url) = il.into_parts()?;
        Ok(Self {
            cache: IndexCache::at_path(path),
            url,
            head: None,
        })
    }

    /// Sets the sha-1 id for the head commit.
    ///
    /// If set, this will be used to disregard cache entries that do not match
    #[inline]
    pub fn set_head_commit(&mut self, commit_id: Option<[u8; 20]>) {
        if let Some(id) = &commit_id {
            let mut hex_head = [0u8; 40];
            crate::utils::encode_hex(id, &mut hex_head);
            self.head = Some(hex_head);
        } else {
            self.head = None;
        }
    }

    /// Gets the hex-encoded sha-1 id for the head commit
    #[inline]
    pub fn head_commit(&self) -> Option<&str> {
        self.head.as_ref().map(|hc| {
            // SAFETY: the buffer is always ASCII hex
            #[allow(unsafe_code)]
            unsafe {
                std::str::from_utf8_unchecked(hc)
            }
        })
    }

    /// Reads a crate from the local cache of the index.
    ///
    /// There are no guarantees around freshness, and no network I/O will be
    /// performed.
    #[inline]
    pub fn cached_krate(
        &self,
        name: KrateName<'_>,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        self.cache.cached_krate(name, self.head_commit(), lock)
    }

    /// Writes the specified crate to the cache.
    ///
    /// Note that no I/O will be performed if `blob_id` or [`Self::set_head_commit`]
    /// has not been set to `Some`
    #[inline]
    pub fn write_to_cache(
        &self,
        krate: &IndexKrate,
        blob_id: Option<&str>,
        lock: &FileLock,
    ) -> Result<Option<PathBuf>, Error> {
        let Some(id) = blob_id.or_else(|| self.head_commit()) else {
            return Ok(None);
        };
        self.cache.write_to_cache(krate, id, lock).map(Some)
    }
}
