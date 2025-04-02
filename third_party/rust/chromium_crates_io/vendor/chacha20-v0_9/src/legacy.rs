//! Legacy version of ChaCha20 with a 64-bit nonce

use super::{ChaChaCore, Key, Nonce};
use cipher::{
    consts::{U10, U32, U64, U8},
    generic_array::GenericArray,
    BlockSizeUser, IvSizeUser, KeyIvInit, KeySizeUser, StreamCipherCore, StreamCipherCoreWrapper,
    StreamCipherSeekCore, StreamClosure,
};

#[cfg(feature = "zeroize")]
use cipher::zeroize::ZeroizeOnDrop;

/// Nonce type used by [`ChaCha20Legacy`].
pub type LegacyNonce = GenericArray<u8, U8>;

/// The ChaCha20 stream cipher (legacy "djb" construction with 64-bit nonce).
///
/// **WARNING:** this implementation uses 32-bit counter, while the original
/// implementation uses 64-bit counter. In other words, it does
/// not allow encrypting of more than 256 GiB of data.
pub type ChaCha20Legacy = StreamCipherCoreWrapper<ChaCha20LegacyCore>;

/// The ChaCha20 stream cipher (legacy "djb" construction with 64-bit nonce).
pub struct ChaCha20LegacyCore(ChaChaCore<U10>);

impl KeySizeUser for ChaCha20LegacyCore {
    type KeySize = U32;
}

impl IvSizeUser for ChaCha20LegacyCore {
    type IvSize = U8;
}

impl BlockSizeUser for ChaCha20LegacyCore {
    type BlockSize = U64;
}

impl KeyIvInit for ChaCha20LegacyCore {
    #[inline(always)]
    fn new(key: &Key, iv: &LegacyNonce) -> Self {
        let mut padded_iv = Nonce::default();
        padded_iv[4..].copy_from_slice(iv);
        ChaCha20LegacyCore(ChaChaCore::new(key, &padded_iv))
    }
}

impl StreamCipherCore for ChaCha20LegacyCore {
    #[inline(always)]
    fn remaining_blocks(&self) -> Option<usize> {
        self.0.remaining_blocks()
    }

    #[inline(always)]
    fn process_with_backend(&mut self, f: impl StreamClosure<BlockSize = Self::BlockSize>) {
        self.0.process_with_backend(f);
    }
}

impl StreamCipherSeekCore for ChaCha20LegacyCore {
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
impl ZeroizeOnDrop for ChaCha20LegacyCore {}
