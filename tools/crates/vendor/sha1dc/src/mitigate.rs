//! Hashing that *mitigates* a detected collision attack instead of reporting
//! the colliding digest.
//!
//! This is the special mode. Prefer [`crate::Hasher`]. A mitigated digest
//! matches no other SHA-1 implementation, so you cannot use it as a shared
//! identifier.
//!
//! Use this mode when you must produce a usable digest during an attack and
//! no external system must agree with the value. Examples are a private
//! content-addressed store, a cache key, and a pipeline with no error path.

use core::fmt;

use crate::{Builder, Digest, Inner, hasher_common};

/// A collision attack was detected, and mitigated.
///
/// [`Hasher::finalize`] returns this. You can use the digest as an
/// identity, because no message is known to give the same value. But it
/// matches no other SHA-1 implementation, and an attacker made the input.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Mitigated {
    pub(crate) digest: Digest,
}

impl Mitigated {
    /// The mitigated digest. You can use this value.
    pub const fn digest(&self) -> Digest {
        self.digest
    }
}

impl fmt::Display for Mitigated {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("SHA-1 collision attack detected; digest was mitigated")
    }
}

impl core::error::Error for Mitigated {}

/// SHA-1 hasher that mitigates a detected collision attack.
///
/// A message that is not part of an attack gives plain SHA-1. A message that
/// is part of one gets a *mitigated* digest, so the two colliding messages
/// give different values and the collision has no effect. Finalization still
/// reports the attack, because the digest is then non-standard and an attacker
/// made the input.
///
/// # Examples
///
/// ```
/// use sha1dc::mitigate;
///
/// let mut hasher = mitigate::Hasher::new();
/// hasher.update(b"hello world");
///
/// match hasher.finalize() {
///     Ok(digest) => println!("{digest}"),
///     // You can use this digest, but it is non-standard and an attacker made
///     // the input.
///     Err(mitigated) => println!("attack detected, hashed as {}", mitigated.digest()),
/// }
/// ```
#[derive(Clone)]
pub struct Hasher(pub(crate) Inner);

impl Default for Hasher {
    fn default() -> Self {
        Self::new()
    }
}

impl Hasher {
    /// Creates a hasher.
    pub fn new() -> Self {
        Builder::new().build_mitigating()
    }
}

hasher_common!(Hasher, Mitigated, |digest| Mitigated { digest });

/// Hashes `data`, mitigating a detected collision attack.
///
/// This is the one-shot form of [`Hasher`]. For the usual non-mitigating form,
/// see [`crate::digest`].
///
/// # Examples
///
/// ```
/// let digest = sha1dc::mitigate::digest(b"hello world").expect("no collision");
/// assert_eq!(digest.to_string(), "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed");
/// ```
pub fn digest(data: &[u8]) -> Result<Digest, Mitigated> {
    let mut hasher = Hasher::new();
    hasher.update(data);
    hasher.finalize()
}
