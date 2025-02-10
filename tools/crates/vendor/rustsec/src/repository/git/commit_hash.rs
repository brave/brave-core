use std::fmt::Display;

use tame_index::external::gix;

/// ID (i.e. SHA-1 hash) of a git commit
///
/// This is a wrapper around [gix::ObjectId] to prevent gix semver changes
/// also breaking semver for `rustsec` crate.
#[cfg_attr(docsrs, doc(cfg(feature = "git")))]
#[derive(Debug, PartialEq, Eq, Ord, PartialOrd, Clone, Copy)]
pub struct CommitHash {
    hash: gix::ObjectId,
}

impl CommitHash {
    // Conversions to/from `gix` are only pub(crate)
    // to avoid leaking `gix` types and semver into the external API
    pub(crate) fn to_gix(self) -> gix::ObjectId {
        self.hash
    }

    pub(crate) fn from_gix(hash: gix::ObjectId) -> Self {
        CommitHash { hash }
    }

    /// Interpret this object id as raw byte slice.
    #[inline]
    pub fn as_bytes(&self) -> &[u8] {
        self.hash.as_bytes()
    }

    /// Display the hash as a hexadecimal string.
    #[inline]
    pub fn to_hex(&self) -> String {
        self.hash.to_hex().to_string()
    }

    /// Returns `true` if this hash is equal to an empty blob.
    #[inline]
    pub fn is_empty_blob(&self) -> bool {
        self.hash.is_empty_blob()
    }

    /// Returns `true` if this hash is equal to an empty tree.
    #[inline]
    pub fn is_empty_tree(&self) -> bool {
        self.hash.is_empty_tree()
    }
}

impl Display for CommitHash {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.hash.fmt(f)
    }
}
