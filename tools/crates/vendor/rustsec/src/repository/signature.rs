//! Git commit signatures

use crate::error::Error;

/// Digital signatures (in OpenPGP format) on commits to the repository
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Signature(Vec<u8>);

impl Signature {
    /// Parse a signature from a Git commit
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, Error> {
        // TODO: actually verify the signature is well-structured
        Ok(Signature(bytes.into()))
    }
}

impl AsRef<[u8]> for Signature {
    fn as_ref(&self) -> &[u8] {
        self.0.as_ref()
    }
}
