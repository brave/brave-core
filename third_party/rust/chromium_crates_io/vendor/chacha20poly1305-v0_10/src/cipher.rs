//! Core AEAD cipher implementation for (X)ChaCha20Poly1305.

use ::cipher::{StreamCipher, StreamCipherSeek};
use aead::generic_array::GenericArray;
use aead::Error;
use poly1305::{
    universal_hash::{KeyInit, UniversalHash},
    Poly1305,
};
use zeroize::Zeroize;

use super::Tag;

/// Size of a ChaCha20 block in bytes
const BLOCK_SIZE: usize = 64;

/// Maximum number of blocks that can be encrypted with ChaCha20 before the
/// counter overflows.
const MAX_BLOCKS: usize = core::u32::MAX as usize;

/// ChaCha20Poly1305 instantiated with a particular nonce
pub(crate) struct Cipher<C>
where
    C: StreamCipher + StreamCipherSeek,
{
    cipher: C,
    mac: Poly1305,
}

impl<C> Cipher<C>
where
    C: StreamCipher + StreamCipherSeek,
{
    /// Instantiate the underlying cipher with a particular nonce
    pub(crate) fn new(mut cipher: C) -> Self {
        // Derive Poly1305 key from the first 32-bytes of the ChaCha20 keystream
        let mut mac_key = poly1305::Key::default();
        cipher.apply_keystream(&mut *mac_key);

        let mac = Poly1305::new(GenericArray::from_slice(&*mac_key));
        mac_key.zeroize();

        // Set ChaCha20 counter to 1
        cipher.seek(BLOCK_SIZE as u64);

        Self { cipher, mac }
    }

    /// Encrypt the given message in-place, returning the authentication tag
    pub(crate) fn encrypt_in_place_detached(
        mut self,
        associated_data: &[u8],
        buffer: &mut [u8],
    ) -> Result<Tag, Error> {
        if buffer.len() / BLOCK_SIZE >= MAX_BLOCKS {
            return Err(Error);
        }

        self.mac.update_padded(associated_data);

        // TODO(tarcieri): interleave encryption with Poly1305
        // See: <https://github.com/RustCrypto/AEADs/issues/74>
        self.cipher.apply_keystream(buffer);
        self.mac.update_padded(buffer);

        self.authenticate_lengths(associated_data, buffer)?;
        Ok(self.mac.finalize())
    }

    /// Decrypt the given message, first authenticating ciphertext integrity
    /// and returning an error if it's been tampered with.
    pub(crate) fn decrypt_in_place_detached(
        mut self,
        associated_data: &[u8],
        buffer: &mut [u8],
        tag: &Tag,
    ) -> Result<(), Error> {
        if buffer.len() / BLOCK_SIZE >= MAX_BLOCKS {
            return Err(Error);
        }

        self.mac.update_padded(associated_data);
        self.mac.update_padded(buffer);
        self.authenticate_lengths(associated_data, buffer)?;

        // This performs a constant-time comparison using the `subtle` crate
        if self.mac.verify(tag).is_ok() {
            // TODO(tarcieri): interleave decryption with Poly1305
            // See: <https://github.com/RustCrypto/AEADs/issues/74>
            self.cipher.apply_keystream(buffer);
            Ok(())
        } else {
            Err(Error)
        }
    }

    /// Authenticate the lengths of the associated data and message
    fn authenticate_lengths(&mut self, associated_data: &[u8], buffer: &[u8]) -> Result<(), Error> {
        let associated_data_len: u64 = associated_data.len().try_into().map_err(|_| Error)?;
        let buffer_len: u64 = buffer.len().try_into().map_err(|_| Error)?;

        let mut block = GenericArray::default();
        block[..8].copy_from_slice(&associated_data_len.to_le_bytes());
        block[8..].copy_from_slice(&buffer_len.to_le_bytes());
        self.mac.update(&[block]);

        Ok(())
    }
}
