//! XChaCha is an extended nonce variant of ChaCha

use super::{ChaChaCore, Key, Nonce, CONSTANTS, STATE_WORDS};
use cipher::{
    consts::{U10, U16, U24, U32, U4, U6, U64},
    generic_array::{typenum::Unsigned, GenericArray},
    BlockSizeUser, IvSizeUser, KeyIvInit, KeySizeUser, StreamCipherCore, StreamCipherCoreWrapper,
    StreamCipherSeekCore, StreamClosure,
};

#[cfg(feature = "zeroize")]
use cipher::zeroize::ZeroizeOnDrop;

/// Nonce type used by XChaCha variants.
pub type XNonce = GenericArray<u8, U24>;

/// XChaCha is a ChaCha20 variant with an extended 192-bit (24-byte) nonce.
///
/// The construction is an adaptation of the same techniques used by
/// XChaCha as described in the paper "Extending the Salsa20 Nonce",
/// applied to the 96-bit nonce variant of ChaCha20, and derive a
/// separate subkey/nonce for each extended nonce:
///
/// <https://cr.yp.to/snuffle/xsalsa-20081128.pdf>
///
/// No authoritative specification exists for XChaCha20, however the
/// construction has "rough consensus and running code" in the form of
/// several interoperable libraries and protocols (e.g. libsodium, WireGuard)
/// and is documented in an (expired) IETF draft:
///
/// <https://tools.ietf.org/html/draft-arciszewski-xchacha-03>
pub type XChaCha20 = StreamCipherCoreWrapper<XChaChaCore<U10>>;
/// XChaCha12 stream cipher (reduced-round variant of [`XChaCha20`] with 12 rounds)
pub type XChaCha12 = StreamCipherCoreWrapper<XChaChaCore<U6>>;
/// XChaCha8 stream cipher (reduced-round variant of [`XChaCha20`] with 8 rounds)
pub type XChaCha8 = StreamCipherCoreWrapper<XChaChaCore<U4>>;

/// The XChaCha core function.
pub struct XChaChaCore<R: Unsigned>(ChaChaCore<R>);

impl<R: Unsigned> KeySizeUser for XChaChaCore<R> {
    type KeySize = U32;
}

impl<R: Unsigned> IvSizeUser for XChaChaCore<R> {
    type IvSize = U24;
}

impl<R: Unsigned> BlockSizeUser for XChaChaCore<R> {
    type BlockSize = U64;
}

impl<R: Unsigned> KeyIvInit for XChaChaCore<R> {
    fn new(key: &Key, iv: &XNonce) -> Self {
        let subkey = hchacha::<R>(key, iv[..16].as_ref().into());
        let mut padded_iv = Nonce::default();
        padded_iv[4..].copy_from_slice(&iv[16..]);
        XChaChaCore(ChaChaCore::new(&subkey, &padded_iv))
    }
}

impl<R: Unsigned> StreamCipherCore for XChaChaCore<R> {
    #[inline(always)]
    fn remaining_blocks(&self) -> Option<usize> {
        self.0.remaining_blocks()
    }

    #[inline(always)]
    fn process_with_backend(&mut self, f: impl StreamClosure<BlockSize = Self::BlockSize>) {
        self.0.process_with_backend(f);
    }
}

impl<R: Unsigned> StreamCipherSeekCore for XChaChaCore<R> {
    type Counter = u32;

    #[inline(always)]
    fn get_block_pos(&self) -> u32 {
        self.0.get_block_pos()
    }

    #[inline(always)]
    fn set_block_pos(&mut self, pos: u32) {
        self.0.set_block_pos(pos);
    }
}

#[cfg(feature = "zeroize")]
#[cfg_attr(docsrs, doc(cfg(feature = "zeroize")))]
impl<R: Unsigned> ZeroizeOnDrop for XChaChaCore<R> {}

/// The HChaCha function: adapts the ChaCha core function in the same
/// manner that HSalsa adapts the Salsa function.
///
/// HChaCha takes 512-bits of input:
///
/// - Constants: `u32` x 4
/// - Key: `u32` x 8
/// - Nonce: `u32` x 4
///
/// It produces 256-bits of output suitable for use as a ChaCha key
///
/// For more information on HSalsa on which HChaCha is based, see:
///
/// <http://cr.yp.to/snuffle/xsalsa-20110204.pdf>
pub fn hchacha<R: Unsigned>(key: &Key, input: &GenericArray<u8, U16>) -> GenericArray<u8, U32> {
    let mut state = [0u32; STATE_WORDS];
    state[..4].copy_from_slice(&CONSTANTS);

    let key_chunks = key.chunks_exact(4);
    for (v, chunk) in state[4..12].iter_mut().zip(key_chunks) {
        *v = u32::from_le_bytes(chunk.try_into().unwrap());
    }
    let input_chunks = input.chunks_exact(4);
    for (v, chunk) in state[12..16].iter_mut().zip(input_chunks) {
        *v = u32::from_le_bytes(chunk.try_into().unwrap());
    }

    // R rounds consisting of R/2 column rounds and R/2 diagonal rounds
    for _ in 0..R::USIZE {
        // column rounds
        quarter_round(0, 4, 8, 12, &mut state);
        quarter_round(1, 5, 9, 13, &mut state);
        quarter_round(2, 6, 10, 14, &mut state);
        quarter_round(3, 7, 11, 15, &mut state);

        // diagonal rounds
        quarter_round(0, 5, 10, 15, &mut state);
        quarter_round(1, 6, 11, 12, &mut state);
        quarter_round(2, 7, 8, 13, &mut state);
        quarter_round(3, 4, 9, 14, &mut state);
    }

    let mut output = GenericArray::default();

    for (chunk, val) in output[..16].chunks_exact_mut(4).zip(&state[..4]) {
        chunk.copy_from_slice(&val.to_le_bytes());
    }

    for (chunk, val) in output[16..].chunks_exact_mut(4).zip(&state[12..]) {
        chunk.copy_from_slice(&val.to_le_bytes());
    }

    output
}

/// The ChaCha20 quarter round function
// for simplicity this function is copied from the software backend
fn quarter_round(a: usize, b: usize, c: usize, d: usize, state: &mut [u32; STATE_WORDS]) {
    state[a] = state[a].wrapping_add(state[b]);
    state[d] ^= state[a];
    state[d] = state[d].rotate_left(16);

    state[c] = state[c].wrapping_add(state[d]);
    state[b] ^= state[c];
    state[b] = state[b].rotate_left(12);

    state[a] = state[a].wrapping_add(state[b]);
    state[d] ^= state[a];
    state[d] = state[d].rotate_left(8);

    state[c] = state[c].wrapping_add(state[d]);
    state[b] ^= state[c];
    state[b] = state[b].rotate_left(7);
}

#[cfg(test)]
mod hchacha20_tests {
    use super::*;
    use hex_literal::hex;

    /// Test vectors from:
    /// https://tools.ietf.org/id/draft-arciszewski-xchacha-03.html#rfc.section.2.2.1
    #[test]
    fn test_vector() {
        const KEY: [u8; 32] = hex!(
            "000102030405060708090a0b0c0d0e0f"
            "101112131415161718191a1b1c1d1e1f"
        );

        const INPUT: [u8; 16] = hex!("000000090000004a0000000031415927");

        const OUTPUT: [u8; 32] = hex!(
            "82413b4227b27bfed30e42508a877d73"
            "a0f9e4d58a74a853c12ec41326d3ecdc"
        );

        let actual = hchacha::<U10>(
            GenericArray::from_slice(&KEY),
            &GenericArray::from_slice(&INPUT),
        );
        assert_eq!(actual.as_slice(), &OUTPUT);
    }
}
