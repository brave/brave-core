//! Stable hasher adapted for cross-platform independent hash.

use std::fmt;
use std::hash::Hasher;

#[cfg(test)]
mod tests;

/// Extended [`Hasher`] trait for use with [`StableHasher`].
///
/// It permits returning an arbitrary type as the [`Self::Hash`] type
/// contrary to the [`Hasher`] trait which can only return `u64`. This
/// is useful when the hasher uses a different representation.
///
/// # Example
///
/// ```
/// use std::hash::Hasher;
/// use rustc_stable_hash::ExtendedHasher;
///
/// struct BogusHasher(u128);
///
/// impl Hasher for BogusHasher {
///     fn write(&mut self, a: &[u8]) {
///         # self.0 = a.iter().fold(0u128, |acc, a| acc + (*a as u128)) + self.0;
///         // ...
///     }
///
///     fn finish(&self) -> u64 {
///         self.0 as u64 // really bogus
///     }
/// }
///
/// impl ExtendedHasher for BogusHasher {
///     type Hash = u128;
///
///     fn short_write<const LEN: usize>(&mut self, bytes: [u8; LEN]) {
///         self.write(&bytes)
///     }
///
///     fn finish(self) -> Self::Hash {
///         self.0
///     }
/// }
/// ```
pub trait ExtendedHasher: Hasher {
    /// Type returned by the hasher.
    type Hash;

    /// Optimized version of [`Hasher::write`] but for small write.
    fn short_write<const LEN: usize>(&mut self, bytes: [u8; LEN]) {
        self.write(&bytes);
    }

    /// Finalization method of the hasher to return the [`Hash`].
    fn finish(self) -> Self::Hash;
}

/// A Stable Hasher adapted for cross-platform independent hash.
///
/// When hashing something that ends up affecting properties like symbol names,
/// we want these symbol names to be calculated independently of other factors
/// like what architecture you're compiling *from*.
///
/// To that end we always convert integers to little-endian format before
/// hashing and the architecture dependent `isize` and `usize` types are
/// extended to 64 bits if needed.
///
/// # Example
///
/// ```
/// use rustc_stable_hash::hashers::{StableSipHasher128, SipHasher128Hash};
/// use rustc_stable_hash::{StableHasher, FromStableHash};
/// use std::hash::Hasher;
///
/// struct Hash128([u64; 2]);
/// impl FromStableHash for Hash128 {
///     type Hash = SipHasher128Hash;
///
///     fn from(SipHasher128Hash(hash): SipHasher128Hash) -> Hash128 {
///         Hash128(hash)
///     }
/// }
///
/// let mut hasher = StableSipHasher128::new();
/// hasher.write_usize(0xFA);
///
/// let hash: Hash128 = hasher.finish();
/// ```
#[must_use]
#[derive(Clone)]
pub struct StableHasher<H: ExtendedHasher> {
    state: H,
}

/// Trait for processing the result of the stable hashing operation.
///
/// # Example
///
/// ```
/// use rustc_stable_hash::{StableHasher, FromStableHash};
///
/// struct Hash128(u128);
///
/// impl FromStableHash for Hash128 {
///     type Hash = [u64; 2];
///
///     fn from(hash: [u64; 2]) -> Hash128 {
///         let upper: u128 = hash[0] as u128;
///         let lower: u128 = hash[1] as u128;
///
///         Hash128((upper << 64) | lower)
///     }
/// }
/// ```
pub trait FromStableHash: Sized {
    type Hash;

    /// Convert the finalized state of a [`StableHasher`] and construct
    /// an [`Self`] containing the processed hash.
    fn from(hash: Self::Hash) -> Self;
}

impl<H: ExtendedHasher + Default> StableHasher<H> {
    /// Creates a new [`StableHasher`].
    ///
    /// To be used with the [`Hasher`] implementation and [`StableHasher::finish`].
    #[inline]
    pub fn new() -> Self {
        Default::default()
    }
}

impl<H: ExtendedHasher + Default> Default for StableHasher<H> {
    /// Creates a new [`StableHasher`].
    ///
    /// To be used with the [`Hasher`] implementation and [`StableHasher::finish`].
    #[inline]
    fn default() -> Self {
        StableHasher {
            state: Default::default(),
        }
    }
}

impl<H: ExtendedHasher> StableHasher<H> {
    /// Creates a new [`StableHasher`] from an already created [`ExtendedHasher`].
    ///
    /// Useful when wanting to initialize a hasher with different parameters/keys.
    ///
    /// **Important**: Any use of the hasher before being given to a [`StableHasher`]
    /// is not covered by this crate guarentees and will make the resulting hash
    /// NOT platform independent.
    #[inline]
    pub fn with_hasher(state: H) -> Self {
        StableHasher { state }
    }

    /// Returns the typed-hash value for the values written.
    ///
    /// The resulting typed-hash value is constructed from an
    /// [`FromStableHash`] implemenation.
    ///
    /// To be used in-place of [`Hasher::finish`].
    #[inline]
    #[must_use]
    pub fn finish<W: FromStableHash<Hash = H::Hash>>(self) -> W {
        W::from(self.state.finish())
    }
}

impl<H: ExtendedHasher + fmt::Debug> fmt::Debug for StableHasher<H> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?}", self.state)
    }
}

impl<H: ExtendedHasher> Hasher for StableHasher<H> {
    /// Returns a combined hash.
    ///
    /// For greater precision use instead [`StableHasher::finish`].
    fn finish(&self) -> u64 {
        Hasher::finish(&self.state)
    }

    #[inline]
    fn write(&mut self, bytes: &[u8]) {
        self.state.write(bytes);
    }

    #[cfg(feature = "nightly")]
    #[inline]
    fn write_str(&mut self, s: &str) {
        self.state.write_str(s);
    }

    #[cfg(feature = "nightly")]
    #[inline]
    fn write_length_prefix(&mut self, len: usize) {
        // Our impl for `usize` will extend it if needed.
        self.write_usize(len);
    }

    #[inline]
    fn write_u8(&mut self, i: u8) {
        self.state.write_u8(i);
    }

    #[inline]
    fn write_u16(&mut self, i: u16) {
        self.state.short_write(i.to_le_bytes());
    }

    #[inline]
    fn write_u32(&mut self, i: u32) {
        self.state.short_write(i.to_le_bytes());
    }

    #[inline]
    fn write_u64(&mut self, i: u64) {
        self.state.short_write(i.to_le_bytes());
    }

    #[inline]
    fn write_u128(&mut self, i: u128) {
        self.write_u64(i as u64);
        self.write_u64((i >> 64) as u64);
    }

    #[inline]
    fn write_usize(&mut self, i: usize) {
        // Always treat usize as u64 so we get the same results on 32 and 64 bit
        // platforms. This is important for symbol hashes when cross compiling,
        // for example.
        self.state.short_write((i as u64).to_le_bytes());
    }

    #[inline]
    fn write_i8(&mut self, i: i8) {
        self.state.write_i8(i);
    }

    #[inline]
    fn write_i16(&mut self, i: i16) {
        self.state.short_write((i as u16).to_le_bytes());
    }

    #[inline]
    fn write_i32(&mut self, i: i32) {
        self.state.short_write((i as u32).to_le_bytes());
    }

    #[inline]
    fn write_i64(&mut self, i: i64) {
        self.state.short_write((i as u64).to_le_bytes());
    }

    #[inline]
    fn write_i128(&mut self, i: i128) {
        self.state.write(&(i as u128).to_le_bytes());
    }

    #[inline]
    fn write_isize(&mut self, i: isize) {
        // Always treat isize as a 64-bit number so we get the same results on 32 and 64 bit
        // platforms. This is important for symbol hashes when cross compiling,
        // for example. Sign extending here is preferable as it means that the
        // same negative number hashes the same on both 32 and 64 bit platforms.
        let value = i as u64;

        // Cold path
        #[cold]
        #[inline(never)]
        fn hash_value<H: ExtendedHasher>(state: &mut H, value: u64) {
            state.write_u8(0xFF);
            state.short_write(value.to_le_bytes());
        }

        // `isize` values often seem to have a small (positive) numeric value in practice.
        // To exploit this, if the value is small, we will hash a smaller amount of bytes.
        // However, we cannot just skip the leading zero bytes, as that would produce the same hash
        // e.g. if you hash two values that have the same bit pattern when they are swapped.
        // See https://github.com/rust-lang/rust/pull/93014 for context.
        //
        // Therefore, we employ the following strategy:
        // 1) When we encounter a value that fits within a single byte (the most common case), we
        // hash just that byte. This is the most common case that is being optimized. However, we do
        // not do this for the value 0xFF, as that is a reserved prefix (a bit like in UTF-8).
        // 2) When we encounter a larger value, we hash a "marker" 0xFF and then the corresponding
        // 8 bytes. Since this prefix cannot occur when we hash a single byte, when we hash two
        // `isize`s that fit within a different amount of bytes, they should always produce a different
        // byte stream for the hasher.
        if value < 0xFF {
            self.state.write_u8(value as u8);
        } else {
            hash_value(&mut self.state, value);
        }
    }
}
