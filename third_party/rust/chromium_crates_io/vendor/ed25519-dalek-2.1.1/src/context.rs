use crate::{InternalError, SignatureError};

/// Ed25519 contexts as used by Ed25519ph.
///
/// Contexts are domain separator strings that can be used to isolate uses of
/// the algorithm between different protocols (which is very hard to reliably do
/// otherwise) and between different uses within the same protocol.
///
/// To create a context, call either of the following:
///
/// - [`SigningKey::with_context`](crate::SigningKey::with_context)
/// - [`VerifyingKey::with_context`](crate::VerifyingKey::with_context)
///
/// For more information, see [RFC8032 § 8.3](https://www.rfc-editor.org/rfc/rfc8032#section-8.3).
///
/// # Example
///
#[cfg_attr(all(feature = "digest", feature = "rand_core"), doc = "```")]
#[cfg_attr(
    any(not(feature = "digest"), not(feature = "rand_core")),
    doc = "```ignore"
)]
/// # fn main() {
/// use ed25519_dalek::{Signature, SigningKey, VerifyingKey, Sha512};
/// # use curve25519_dalek::digest::Digest;
/// # use rand::rngs::OsRng;
/// use ed25519_dalek::{DigestSigner, DigestVerifier};
///
/// # let mut csprng = OsRng;
/// # let signing_key = SigningKey::generate(&mut csprng);
/// # let verifying_key = signing_key.verifying_key();
/// let context_str = b"Local Channel 3";
/// let prehashed_message = Sha512::default().chain_update(b"Stay tuned for more news at 7");
///
/// // Signer
/// let signing_context = signing_key.with_context(context_str).unwrap();
/// let signature = signing_context.sign_digest(prehashed_message.clone());
///
/// // Verifier
/// let verifying_context = verifying_key.with_context(context_str).unwrap();
/// let verified: bool = verifying_context
///     .verify_digest(prehashed_message, &signature)
///     .is_ok();
///
/// # assert!(verified);
/// # }
/// ```
#[derive(Clone, Debug)]
pub struct Context<'k, 'v, K> {
    /// Key this context is being used with.
    key: &'k K,

    /// Context value: a bytestring no longer than 255 octets.
    value: &'v [u8],
}

impl<'k, 'v, K> Context<'k, 'v, K> {
    /// Maximum length of the context value in octets.
    pub const MAX_LENGTH: usize = 255;

    /// Create a new Ed25519ph context.
    pub(crate) fn new(key: &'k K, value: &'v [u8]) -> Result<Self, SignatureError> {
        if value.len() <= Self::MAX_LENGTH {
            Ok(Self { key, value })
        } else {
            Err(SignatureError::from(InternalError::PrehashedContextLength))
        }
    }

    /// Borrow the key.
    pub fn key(&self) -> &'k K {
        self.key
    }

    /// Borrow the context string value.
    pub fn value(&self) -> &'v [u8] {
        self.value
    }
}

#[cfg(all(test, feature = "digest"))]
mod test {
    #![allow(clippy::unwrap_used)]

    use crate::{Signature, SigningKey, VerifyingKey};
    use curve25519_dalek::digest::Digest;
    use ed25519::signature::{DigestSigner, DigestVerifier};
    use rand::rngs::OsRng;
    use sha2::Sha512;

    #[test]
    fn context_correctness() {
        let mut csprng = OsRng;
        let signing_key: SigningKey = SigningKey::generate(&mut csprng);
        let verifying_key: VerifyingKey = signing_key.verifying_key();

        let context_str = b"Local Channel 3";
        let prehashed_message = Sha512::default().chain_update(b"Stay tuned for more news at 7");

        // Signer
        let signing_context = signing_key.with_context(context_str).unwrap();
        let signature: Signature = signing_context.sign_digest(prehashed_message.clone());

        // Verifier
        let verifying_context = verifying_key.with_context(context_str).unwrap();
        let verified: bool = verifying_context
            .verify_digest(prehashed_message, &signature)
            .is_ok();

        assert!(verified);
    }
}
