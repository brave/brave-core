// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

use super::aead_ctx::AeadCtx;
use super::{
    Algorithm, Nonce, Tag, AES_128_GCM, AES_128_GCM_SIV, AES_192_GCM, AES_256_GCM, AES_256_GCM_SIV,
    CHACHA20_POLY1305, MAX_KEY_LEN, MAX_TAG_LEN, NONCE_LEN,
};
use crate::aws_lc::{
    EVP_AEAD_CTX_open, EVP_AEAD_CTX_open_gather, EVP_AEAD_CTX_seal, EVP_AEAD_CTX_seal_scatter,
};
use crate::error::Unspecified;
use crate::fips::indicator_check;
use crate::hkdf;
use crate::iv::FixedLength;
use core::fmt::Debug;
use core::mem::MaybeUninit;
use core::ops::RangeFrom;
use core::ptr::null;

/// The maximum length of a nonce returned by our AEAD API.
const MAX_NONCE_LEN: usize = NONCE_LEN;

/// The maximum required tag buffer needed if using AWS-LC generated nonce construction
const MAX_TAG_NONCE_BUFFER_LEN: usize = MAX_TAG_LEN + MAX_NONCE_LEN;

/// An AEAD key without a designated role or nonce sequence.
pub struct UnboundKey {
    ctx: AeadCtx,
    algorithm: &'static Algorithm,
}

#[allow(clippy::missing_fields_in_debug)]
impl Debug for UnboundKey {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> Result<(), core::fmt::Error> {
        f.debug_struct("UnboundKey")
            .field("algorithm", &self.algorithm)
            .finish()
    }
}

impl UnboundKey {
    /// Constructs an `UnboundKey`.
    /// # Errors
    /// `error::Unspecified` if `key_bytes.len() != algorithm.key_len()`.
    pub fn new(algorithm: &'static Algorithm, key_bytes: &[u8]) -> Result<Self, Unspecified> {
        Ok(Self {
            ctx: (algorithm.init)(key_bytes, algorithm.tag_len())?,
            algorithm,
        })
    }

    #[inline]
    pub(crate) fn open_within<'in_out>(
        &self,
        nonce: Nonce,
        aad: &[u8],
        in_out: &'in_out mut [u8],
        ciphertext_and_tag: RangeFrom<usize>,
    ) -> Result<&'in_out mut [u8], Unspecified> {
        let in_prefix_len = ciphertext_and_tag.start;
        let ciphertext_and_tag_len = in_out.len().checked_sub(in_prefix_len).ok_or(Unspecified)?;
        let ciphertext_len = ciphertext_and_tag_len
            .checked_sub(self.algorithm().tag_len())
            .ok_or(Unspecified)?;
        self.check_per_nonce_max_bytes(ciphertext_len)?;

        match self.ctx {
            AeadCtx::AES_128_GCM_RANDNONCE(_) | AeadCtx::AES_256_GCM_RANDNONCE(_) => {
                self.open_combined_randnonce(nonce, aad, &mut in_out[in_prefix_len..])
            }
            _ => self.open_combined(nonce, aad.as_ref(), &mut in_out[in_prefix_len..]),
        }?;

        // shift the plaintext to the left
        in_out.copy_within(in_prefix_len..in_prefix_len + ciphertext_len, 0);

        // `ciphertext_len` is also the plaintext length.
        Ok(&mut in_out[..ciphertext_len])
    }

    #[inline]
    pub(crate) fn open_separate_gather(
        &self,
        nonce: &Nonce,
        aad: &[u8],
        in_ciphertext: &[u8],
        in_tag: &[u8],
        out_plaintext: &mut [u8],
    ) -> Result<(), Unspecified> {
        self.open_separate_gather_impl(
            nonce,
            aad,
            in_ciphertext.as_ptr(),
            in_ciphertext.len(),
            in_tag,
            out_plaintext.as_mut_ptr(),
            out_plaintext.len(),
        )
    }

    #[inline]
    pub(crate) fn open_in_place_separate_tag(
        &self,
        nonce: &Nonce,
        aad: &[u8],
        in_tag: &[u8],
        in_out: &mut [u8],
    ) -> Result<(), Unspecified> {
        let ptr = in_out.as_mut_ptr();
        let len = in_out.len();
        self.open_separate_gather_impl(nonce, aad, ptr.cast_const(), len, in_tag, ptr, len)
    }

    /// Common FFI path for `EVP_AEAD_CTX_open_gather`-based opening.
    ///
    /// `in_ciphertext` / `out_plaintext` may alias (exactly, i.e. same base
    /// pointer and length). `EVP_AEAD_CTX_open_gather` explicitly permits
    /// `out == in`, which is how `open_in_place_separate_tag` works.
    ///
    /// Callers must ensure:
    /// * `in_ciphertext` is valid for reads of `in_ciphertext_len` bytes.
    /// * `out_plaintext` is valid for writes of `out_plaintext_len` bytes.
    /// * If the two pointers alias, they must alias exactly (same base, same length).
    #[inline]
    #[allow(clippy::too_many_arguments)]
    fn open_separate_gather_impl(
        &self,
        nonce: &Nonce,
        aad: &[u8],
        in_ciphertext: *const u8,
        in_ciphertext_len: usize,
        in_tag: &[u8],
        out_plaintext: *mut u8,
        out_plaintext_len: usize,
    ) -> Result<(), Unspecified> {
        self.check_per_nonce_max_bytes(in_ciphertext_len)?;

        // ensure that the lengths match
        if in_ciphertext_len != out_plaintext_len {
            return Err(Unspecified);
        }

        unsafe {
            let aead_ctx = self.ctx.as_ref();
            let nonce = nonce.as_ref();

            if 1 != EVP_AEAD_CTX_open_gather(
                aead_ctx.as_const_ptr(),
                out_plaintext,
                nonce.as_ptr(),
                nonce.len(),
                in_ciphertext,
                in_ciphertext_len,
                in_tag.as_ptr(),
                in_tag.len(),
                aad.as_ptr(),
                aad.len(),
            ) {
                return Err(Unspecified);
            }
            Ok(())
        }
    }

    #[inline]
    pub(crate) fn seal_in_place_append_tag<'a, InOut>(
        &self,
        nonce: Option<Nonce>,
        aad: &[u8],
        in_out: &'a mut InOut,
    ) -> Result<Nonce, Unspecified>
    where
        InOut: AsMut<[u8]> + for<'in_out> Extend<&'in_out u8>,
    {
        self.check_per_nonce_max_bytes(in_out.as_mut().len())?;
        match nonce {
            Some(nonce) => self.seal_combined(nonce, aad, in_out),
            None => self.seal_combined_randnonce(aad, in_out),
        }
    }

    #[inline]
    pub(crate) fn seal_in_place_separate_tag(
        &self,
        nonce: Option<Nonce>,
        aad: &[u8],
        in_out: &mut [u8],
    ) -> Result<(Nonce, Tag), Unspecified> {
        self.check_per_nonce_max_bytes(in_out.len())?;
        match nonce {
            Some(nonce) => self.seal_separate(nonce, aad, in_out),
            None => self.seal_separate_randnonce(aad, in_out),
        }
    }

    #[inline]
    #[allow(clippy::needless_pass_by_value)]
    pub(crate) fn seal_out_of_place_scatter(
        &self,
        nonce: Nonce,
        aad: &[u8],
        in_plaintext: &[u8],
        out_ciphertext: &mut [u8],
        extra_in: &[u8],
        extra_out_and_tag: &mut [u8],
    ) -> Result<(), Unspecified> {
        self.check_per_nonce_max_bytes(in_plaintext.len() + extra_in.len())?;
        if out_ciphertext.len() != in_plaintext.len()
            || extra_out_and_tag.len() != extra_in.len() + self.algorithm().tag_len()
        {
            return Err(Unspecified);
        }

        let nonce = nonce.as_ref();
        // Set to a value the AEAD never reports on success, so the assertion below
        // catches a missing write as well as a short one.
        let mut out_tag_len = 0;

        if 1 != unsafe {
            EVP_AEAD_CTX_seal_scatter(
                self.ctx.as_ref().as_const_ptr(),
                out_ciphertext.as_mut_ptr(),
                extra_out_and_tag.as_mut_ptr(),
                &mut out_tag_len,
                extra_out_and_tag.len(),
                nonce.as_ptr(),
                nonce.len(),
                in_plaintext.as_ptr(),
                in_plaintext.len(),
                extra_in.as_ptr(),
                extra_in.len(),
                aad.as_ptr(),
                aad.len(),
            )
        } {
            return Err(Unspecified);
        }
        debug_assert_eq!(out_tag_len, extra_out_and_tag.len());
        Ok(())
    }

    #[inline]
    #[allow(clippy::needless_pass_by_value)]
    pub(crate) fn seal_in_place_separate_scatter(
        &self,
        nonce: Nonce,
        aad: &[u8],
        in_out: &mut [u8],
        extra_in: &[u8],
        extra_out_and_tag: &mut [u8],
    ) -> Result<(), Unspecified> {
        self.check_per_nonce_max_bytes(in_out.len())?;
        // ensure that the extra lengths match
        {
            let actual = extra_in.len() + self.algorithm().tag_len();
            let expected = extra_out_and_tag.len();

            if actual != expected {
                return Err(Unspecified);
            }
        }

        let nonce = nonce.as_ref();
        let mut out_tag_len = extra_out_and_tag.len();

        if 1 != unsafe {
            EVP_AEAD_CTX_seal_scatter(
                self.ctx.as_ref().as_const_ptr(),
                in_out.as_mut_ptr(),
                extra_out_and_tag.as_mut_ptr(),
                &mut out_tag_len,
                extra_out_and_tag.len(),
                nonce.as_ptr(),
                nonce.len(),
                in_out.as_ptr(),
                in_out.len(),
                extra_in.as_ptr(),
                extra_in.len(),
                aad.as_ptr(),
                aad.len(),
            )
        } {
            return Err(Unspecified);
        }
        Ok(())
    }

    /// The key's AEAD algorithm.
    #[inline]
    #[must_use]
    pub fn algorithm(&self) -> &'static Algorithm {
        self.algorithm
    }

    #[inline]
    pub(crate) fn check_per_nonce_max_bytes(&self, in_out_len: usize) -> Result<(), Unspecified> {
        if in_out_len as u64 > self.algorithm().max_input_len {
            return Err(Unspecified);
        }
        Ok(())
    }

    #[inline]
    #[allow(clippy::needless_pass_by_value)]
    fn open_combined(
        &self,
        nonce: Nonce,
        aad: &[u8],
        in_out: &mut [u8],
    ) -> Result<(), Unspecified> {
        let nonce = nonce.as_ref();

        debug_assert_eq!(nonce.len(), self.algorithm().nonce_len());

        let plaintext_len = in_out.len() - self.algorithm().tag_len();

        let mut out_len = MaybeUninit::<usize>::uninit();
        if 1 != indicator_check!(unsafe {
            EVP_AEAD_CTX_open(
                self.ctx.as_ref().as_const_ptr(),
                in_out.as_mut_ptr(),
                out_len.as_mut_ptr(),
                plaintext_len,
                nonce.as_ptr(),
                nonce.len(),
                in_out.as_ptr(),
                plaintext_len + self.algorithm().tag_len(),
                aad.as_ptr(),
                aad.len(),
            )
        }) {
            return Err(Unspecified);
        }

        Ok(())
    }

    #[inline]
    #[allow(clippy::needless_pass_by_value)]
    fn open_combined_randnonce(
        &self,
        nonce: Nonce,
        aad: &[u8],
        in_out: &mut [u8],
    ) -> Result<(), Unspecified> {
        let nonce = nonce.as_ref();

        let alg_nonce_len = self.algorithm().nonce_len();
        let alg_tag_len = self.algorithm().tag_len();

        debug_assert_eq!(nonce.len(), alg_nonce_len);
        debug_assert!(alg_tag_len + alg_nonce_len <= MAX_TAG_NONCE_BUFFER_LEN);

        let plaintext_len = in_out.len() - alg_tag_len;

        let mut tag_buffer = [0u8; MAX_TAG_NONCE_BUFFER_LEN];

        tag_buffer[..alg_tag_len]
            .copy_from_slice(&in_out[plaintext_len..plaintext_len + alg_tag_len]);
        tag_buffer[alg_tag_len..alg_tag_len + alg_nonce_len].copy_from_slice(nonce);

        let tag_slice = &tag_buffer[0..alg_tag_len + alg_nonce_len];

        if 1 != indicator_check!(unsafe {
            EVP_AEAD_CTX_open_gather(
                self.ctx.as_ref().as_const_ptr(),
                in_out.as_mut_ptr(),
                null(),
                0,
                in_out.as_ptr(),
                plaintext_len,
                tag_slice.as_ptr(),
                tag_slice.len(),
                aad.as_ptr(),
                aad.len(),
            )
        }) {
            return Err(Unspecified);
        }

        Ok(())
    }

    #[inline]
    fn seal_combined<InOut>(
        &self,
        nonce: Nonce,
        aad: &[u8],
        in_out: &mut InOut,
    ) -> Result<Nonce, Unspecified>
    where
        InOut: AsMut<[u8]> + for<'in_out> Extend<&'in_out u8>,
    {
        let plaintext_len = in_out.as_mut().len();

        let alg_tag_len = self.algorithm().tag_len();

        debug_assert!(alg_tag_len <= MAX_TAG_LEN);

        let tag_buffer = [0u8; MAX_TAG_LEN];

        in_out.extend(tag_buffer[..alg_tag_len].iter());

        // Safe `Extend` implementations are not required to grow the buffer,
        // so derive the FFI capacity from the actual post-extend slice.
        let mut_in_out = in_out.as_mut();
        let out_capacity = mut_in_out.len();
        let expected_len = plaintext_len.checked_add(alg_tag_len).ok_or(Unspecified)?;
        // Only under-growth is unsound; an exact match also fails closed on over-growth.
        if out_capacity != expected_len {
            return Err(Unspecified);
        }

        let mut out_len = MaybeUninit::<usize>::uninit();

        {
            let nonce = nonce.as_ref();

            debug_assert_eq!(nonce.len(), self.algorithm().nonce_len());

            if 1 != indicator_check!(unsafe {
                EVP_AEAD_CTX_seal(
                    self.ctx.as_ref().as_const_ptr(),
                    mut_in_out.as_mut_ptr(),
                    out_len.as_mut_ptr(),
                    out_capacity,
                    nonce.as_ptr(),
                    nonce.len(),
                    mut_in_out.as_ptr(),
                    plaintext_len,
                    aad.as_ptr(),
                    aad.len(),
                )
            }) {
                return Err(Unspecified);
            }
        }

        Ok(nonce)
    }

    #[inline]
    fn seal_combined_randnonce<InOut>(
        &self,
        aad: &[u8],
        in_out: &mut InOut,
    ) -> Result<Nonce, Unspecified>
    where
        InOut: AsMut<[u8]> + for<'in_out> Extend<&'in_out u8>,
    {
        let mut tag_buffer = [0u8; MAX_TAG_NONCE_BUFFER_LEN];

        let mut out_tag_len = MaybeUninit::<usize>::uninit();
        let plaintext_len;

        {
            // Derive both the FFI pointer and length from the same slice. `AsMut`
            // implementations are not required to return the same view across calls.
            let mut_in_out = in_out.as_mut();
            plaintext_len = mut_in_out.len();

            if 1 != indicator_check!(unsafe {
                EVP_AEAD_CTX_seal_scatter(
                    self.ctx.as_ref().as_const_ptr(),
                    mut_in_out.as_mut_ptr(),
                    tag_buffer.as_mut_ptr(),
                    out_tag_len.as_mut_ptr(),
                    tag_buffer.len(),
                    null(),
                    0,
                    mut_in_out.as_ptr(),
                    plaintext_len,
                    null(),
                    0,
                    aad.as_ptr(),
                    aad.len(),
                )
            }) {
                return Err(Unspecified);
            }
        }

        let tag_len = self.algorithm().tag_len();
        let nonce_len = self.algorithm().nonce_len();

        let nonce = Nonce(FixedLength::<NONCE_LEN>::try_from(
            &tag_buffer[tag_len..tag_len + nonce_len],
        )?);

        in_out.extend(&tag_buffer[..tag_len]);

        let expected_len = plaintext_len.checked_add(tag_len).ok_or(Unspecified)?;
        if in_out.as_mut().len() != expected_len {
            return Err(Unspecified);
        }

        Ok(nonce)
    }

    #[inline]
    fn seal_separate(
        &self,
        nonce: Nonce,
        aad: &[u8],
        in_out: &mut [u8],
    ) -> Result<(Nonce, Tag), Unspecified> {
        let mut tag = [0u8; MAX_TAG_LEN];
        let mut out_tag_len = MaybeUninit::<usize>::uninit();
        {
            let nonce = nonce.as_ref();

            debug_assert_eq!(nonce.len(), self.algorithm().nonce_len());

            if 1 != indicator_check!(unsafe {
                EVP_AEAD_CTX_seal_scatter(
                    self.ctx.as_ref().as_const_ptr(),
                    in_out.as_mut_ptr(),
                    tag.as_mut_ptr(),
                    out_tag_len.as_mut_ptr(),
                    tag.len(),
                    nonce.as_ptr(),
                    nonce.len(),
                    in_out.as_ptr(),
                    in_out.len(),
                    null(),
                    0usize,
                    aad.as_ptr(),
                    aad.len(),
                )
            }) {
                return Err(Unspecified);
            }
        }
        Ok((nonce, Tag(tag, unsafe { out_tag_len.assume_init() })))
    }

    #[inline]
    fn seal_separate_randnonce(
        &self,
        aad: &[u8],
        in_out: &mut [u8],
    ) -> Result<(Nonce, Tag), Unspecified> {
        let mut tag_buffer = [0u8; MAX_TAG_NONCE_BUFFER_LEN];

        debug_assert!(
            self.algorithm().tag_len() + self.algorithm().nonce_len() <= tag_buffer.len()
        );

        let mut out_tag_len = MaybeUninit::<usize>::uninit();

        if 1 != indicator_check!(unsafe {
            EVP_AEAD_CTX_seal_scatter(
                self.ctx.as_ref().as_const_ptr(),
                in_out.as_mut_ptr(),
                tag_buffer.as_mut_ptr(),
                out_tag_len.as_mut_ptr(),
                tag_buffer.len(),
                null(),
                0,
                in_out.as_ptr(),
                in_out.len(),
                null(),
                0usize,
                aad.as_ptr(),
                aad.len(),
            )
        }) {
            return Err(Unspecified);
        }

        let tag_len = self.algorithm().tag_len();
        let nonce_len = self.algorithm().nonce_len();

        let nonce = Nonce(FixedLength::<NONCE_LEN>::try_from(
            &tag_buffer[tag_len..tag_len + nonce_len],
        )?);

        let mut tag = [0u8; MAX_TAG_LEN];
        tag.copy_from_slice(&tag_buffer[..tag_len]);

        Ok((nonce, Tag(tag, tag_len)))
    }
}

impl From<AeadCtx> for UnboundKey {
    fn from(value: AeadCtx) -> Self {
        let algorithm = match value {
            AeadCtx::AES_128_GCM(_)
            | AeadCtx::AES_128_GCM_TLS12(_)
            | AeadCtx::AES_128_GCM_TLS13(_)
            | AeadCtx::AES_128_GCM_RANDNONCE(_) => &AES_128_GCM,
            AeadCtx::AES_192_GCM(_) => &AES_192_GCM,
            AeadCtx::AES_128_GCM_SIV(_) => &AES_128_GCM_SIV,
            AeadCtx::AES_256_GCM(_)
            | AeadCtx::AES_256_GCM_RANDNONCE(_)
            | AeadCtx::AES_256_GCM_TLS12(_)
            | AeadCtx::AES_256_GCM_TLS13(_) => &AES_256_GCM,
            AeadCtx::AES_256_GCM_SIV(_) => &AES_256_GCM_SIV,
            AeadCtx::CHACHA20_POLY1305(_) => &CHACHA20_POLY1305,
        };
        Self {
            ctx: value,
            algorithm,
        }
    }
}

impl From<hkdf::Okm<'_, &'static Algorithm>> for UnboundKey {
    fn from(okm: hkdf::Okm<&'static Algorithm>) -> Self {
        let mut key_bytes = [0; MAX_KEY_LEN];
        let key_bytes = &mut key_bytes[..okm.len().key_len];
        let algorithm = *okm.len();
        okm.fill(key_bytes).unwrap();
        Self::new(algorithm, key_bytes).unwrap()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct NormalBuffer(Vec<u8>);

    impl AsMut<[u8]> for NormalBuffer {
        fn as_mut(&mut self) -> &mut [u8] {
            self.0.as_mut_slice()
        }
    }

    impl<'a> Extend<&'a u8> for NormalBuffer {
        fn extend<T: IntoIterator<Item = &'a u8>>(&mut self, iter: T) {
            self.0.extend(iter);
        }
    }

    struct NoGrowBuffer(Vec<u8>);

    impl AsMut<[u8]> for NoGrowBuffer {
        fn as_mut(&mut self) -> &mut [u8] {
            self.0.as_mut_slice()
        }
    }

    impl<'a> Extend<&'a u8> for NoGrowBuffer {
        fn extend<T: IntoIterator<Item = &'a u8>>(&mut self, _iter: T) {}
    }

    struct ShortExtendBuffer(Vec<u8>);

    impl AsMut<[u8]> for ShortExtendBuffer {
        fn as_mut(&mut self) -> &mut [u8] {
            self.0.as_mut_slice()
        }
    }

    impl<'a> Extend<&'a u8> for ShortExtendBuffer {
        fn extend<T: IntoIterator<Item = &'a u8>>(&mut self, iter: T) {
            self.0.extend(iter.into_iter().take(1));
        }
    }

    struct ShrinkingBuffer(Vec<u8>);

    impl AsMut<[u8]> for ShrinkingBuffer {
        fn as_mut(&mut self) -> &mut [u8] {
            self.0.as_mut_slice()
        }
    }

    impl<'a> Extend<&'a u8> for ShrinkingBuffer {
        fn extend<T: IntoIterator<Item = &'a u8>>(&mut self, _iter: T) {
            let new_len = self.0.len().saturating_sub(1);
            self.0.truncate(new_len);
        }
    }

    fn test_key() -> UnboundKey {
        UnboundKey::new(&AES_128_GCM, &[0x42u8; 16]).unwrap()
    }

    fn test_randnonce_key() -> UnboundKey {
        UnboundKey::from(
            AeadCtx::aes_128_gcm_randnonce(
                &[0x42u8; 16],
                AES_128_GCM.tag_len(),
                AES_128_GCM.nonce_len(),
            )
            .unwrap(),
        )
    }

    fn test_nonce() -> Nonce {
        Nonce::try_assume_unique_for_key(&[0x24u8; NONCE_LEN]).unwrap()
    }

    #[test]
    fn seal_combined_normal_extend_succeeds_and_roundtrips() {
        let key = test_key();
        let plaintext = b"seal_combined soundness regression test".to_vec();
        let mut in_out = NormalBuffer(plaintext.clone());

        let nonce = key
            .seal_combined(test_nonce(), &[], &mut in_out)
            .expect("a normal, Vec-like Extend impl must succeed");

        assert_eq!(in_out.0.len(), plaintext.len() + key.algorithm().tag_len());

        let opened: &[u8] = key
            .open_within(nonce, &[], &mut in_out.0, 0..)
            .expect("the sealed output must open back to the original plaintext");
        assert_eq!(opened, plaintext.as_slice());
    }

    #[test]
    fn seal_combined_rejects_no_grow_extend() {
        let key = test_key();
        let plaintext = b"some plaintext".to_vec();
        let original_len = plaintext.len();
        let mut in_out = NoGrowBuffer(plaintext);

        let result = key.seal_combined(test_nonce(), &[], &mut in_out);

        assert!(
            result.is_err(),
            "a no-op Extend impl must not be trusted to have appended the tag"
        );
        assert_eq!(in_out.0.len(), original_len);
    }

    #[test]
    fn seal_combined_rejects_short_extend() {
        let key = test_key();
        let mut in_out = ShortExtendBuffer(b"some plaintext".to_vec());

        let result = key.seal_combined(test_nonce(), &[], &mut in_out);

        assert!(
            result.is_err(),
            "an Extend impl that appends fewer bytes than the tag length must be rejected"
        );
    }

    #[test]
    fn seal_combined_rejects_shrinking_extend() {
        let key = test_key();
        let mut in_out = ShrinkingBuffer(b"some plaintext".to_vec());

        let result = key.seal_combined(test_nonce(), &[], &mut in_out);

        assert!(
            result.is_err(),
            "an Extend impl that shrinks the collection must be rejected"
        );
    }

    #[test]
    fn seal_combined_randnonce_normal_extend_succeeds_and_roundtrips() {
        let key = test_randnonce_key();
        let plaintext = b"seal_combined_randnonce soundness regression test".to_vec();
        let mut in_out = NormalBuffer(plaintext.clone());

        let nonce = key
            .seal_combined_randnonce(&[], &mut in_out)
            .expect("a normal, Vec-like Extend impl must succeed");

        assert_eq!(in_out.0.len(), plaintext.len() + key.algorithm().tag_len());

        let opened = key
            .open_within(nonce, &[], &mut in_out.0, 0..)
            .expect("the sealed output must open back to the original plaintext");
        assert_eq!(opened, plaintext.as_slice());
    }

    #[test]
    fn seal_combined_randnonce_rejects_no_grow_extend() {
        let key = test_randnonce_key();
        let plaintext = b"some plaintext".to_vec();
        let original_len = plaintext.len();
        let mut in_out = NoGrowBuffer(plaintext);

        let result = key.seal_combined_randnonce(&[], &mut in_out);

        assert!(
            result.is_err(),
            "a no-op Extend impl must not be trusted to have appended the tag"
        );
        assert_eq!(in_out.0.len(), original_len);
    }

    #[test]
    fn seal_combined_randnonce_rejects_short_extend() {
        let key = test_randnonce_key();
        let mut in_out = ShortExtendBuffer(b"some plaintext".to_vec());

        let result = key.seal_combined_randnonce(&[], &mut in_out);

        assert!(
            result.is_err(),
            "an Extend impl that appends fewer bytes than the tag length must be rejected"
        );
    }

    #[test]
    fn seal_combined_randnonce_rejects_shrinking_extend() {
        let key = test_randnonce_key();
        let mut in_out = ShrinkingBuffer(b"some plaintext".to_vec());

        let result = key.seal_combined_randnonce(&[], &mut in_out);

        assert!(
            result.is_err(),
            "an Extend impl that shrinks the collection must be rejected"
        );
    }
}
