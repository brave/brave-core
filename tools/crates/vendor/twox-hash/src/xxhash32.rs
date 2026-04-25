//! The implementation of XXH32.

use core::{
    fmt,
    hash::{self, BuildHasher},
    mem,
};

use crate::{IntoU32, IntoU64};

// Keeping these constants in this form to match the C code.
const PRIME32_1: u32 = 0x9E3779B1;
const PRIME32_2: u32 = 0x85EBCA77;
const PRIME32_3: u32 = 0xC2B2AE3D;
const PRIME32_4: u32 = 0x27D4EB2F;
const PRIME32_5: u32 = 0x165667B1;

type Lane = u32;
type Lanes = [Lane; 4];
type Bytes = [u8; 16];

const BYTES_IN_LANE: usize = mem::size_of::<Bytes>();

#[derive(Clone, PartialEq)]
struct BufferData(Lanes);

impl BufferData {
    const fn new() -> Self {
        Self([0; 4])
    }

    const fn bytes(&self) -> &Bytes {
        const _: () = assert!(mem::align_of::<u8>() <= mem::align_of::<Lane>());
        // SAFETY[bytes]: The alignment of `u32` is at least that of
        // `u8` and all the values are initialized.
        unsafe { &*self.0.as_ptr().cast() }
    }

    fn bytes_mut(&mut self) -> &mut Bytes {
        // SAFETY: See SAFETY[bytes]
        unsafe { &mut *self.0.as_mut_ptr().cast() }
    }
}

impl fmt::Debug for BufferData {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_list().entries(self.0.iter()).finish()
    }
}

#[derive(Debug, Clone, PartialEq)]
struct Buffer {
    offset: usize,
    data: BufferData,
}

impl Buffer {
    const fn new() -> Self {
        Self {
            offset: 0,
            data: BufferData::new(),
        }
    }

    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn extend<'d>(&mut self, data: &'d [u8]) -> (Option<&Lanes>, &'d [u8]) {
        // Most of the slice methods we use here have `_unchecked` variants, but
        //
        // 1. this method is called one time per `Hasher::write` call
        // 2. this method early exits if we don't have anything in the buffer
        //
        // Because of this, removing the panics via `unsafe` doesn't
        // have much benefit other than reducing code size by a tiny
        // bit.

        if self.offset == 0 {
            return (None, data);
        };

        let bytes = self.data.bytes_mut();
        debug_assert!(self.offset <= bytes.len());

        let empty = &mut bytes[self.offset..];
        let n_to_copy = usize::min(empty.len(), data.len());

        let dst = &mut empty[..n_to_copy];

        let (src, rest) = data.split_at(n_to_copy);

        dst.copy_from_slice(src);
        self.offset += n_to_copy;

        debug_assert!(self.offset <= bytes.len());

        if self.offset == bytes.len() {
            self.offset = 0;
            (Some(&self.data.0), rest)
        } else {
            (None, rest)
        }
    }

    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn set(&mut self, data: &[u8]) {
        if data.is_empty() {
            return;
        }

        debug_assert_eq!(self.offset, 0);

        let n_to_copy = data.len();

        let bytes = self.data.bytes_mut();
        debug_assert!(n_to_copy < bytes.len());

        bytes[..n_to_copy].copy_from_slice(data);
        self.offset = data.len();
    }

    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn remaining(&self) -> &[u8] {
        &self.data.bytes()[..self.offset]
    }
}

#[derive(Clone, PartialEq)]
struct Accumulators(Lanes);

impl Accumulators {
    const fn new(seed: u32) -> Self {
        Self([
            seed.wrapping_add(PRIME32_1).wrapping_add(PRIME32_2),
            seed.wrapping_add(PRIME32_2),
            seed,
            seed.wrapping_sub(PRIME32_1),
        ])
    }

    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn write(&mut self, lanes: Lanes) {
        let [acc1, acc2, acc3, acc4] = &mut self.0;
        let [lane1, lane2, lane3, lane4] = lanes;

        *acc1 = round(*acc1, lane1.to_le());
        *acc2 = round(*acc2, lane2.to_le());
        *acc3 = round(*acc3, lane3.to_le());
        *acc4 = round(*acc4, lane4.to_le());
    }

    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn write_many<'d>(&mut self, mut data: &'d [u8]) -> &'d [u8] {
        while let Some((chunk, rest)) = data.split_first_chunk::<BYTES_IN_LANE>() {
            // SAFETY: We have the right number of bytes and are
            // handling the unaligned case.
            let lanes = unsafe { chunk.as_ptr().cast::<Lanes>().read_unaligned() };
            self.write(lanes);
            data = rest;
        }
        data
    }

    // RATIONALE: See RATIONALE[inline]
    #[inline]
    const fn finish(&self) -> u32 {
        let [acc1, acc2, acc3, acc4] = self.0;

        let acc1 = acc1.rotate_left(1);
        let acc2 = acc2.rotate_left(7);
        let acc3 = acc3.rotate_left(12);
        let acc4 = acc4.rotate_left(18);

        acc1.wrapping_add(acc2)
            .wrapping_add(acc3)
            .wrapping_add(acc4)
    }
}

impl fmt::Debug for Accumulators {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let [acc1, acc2, acc3, acc4] = self.0;
        f.debug_struct("Accumulators")
            .field("acc1", &acc1)
            .field("acc2", &acc2)
            .field("acc3", &acc3)
            .field("acc4", &acc4)
            .finish()
    }
}

/// Calculates the 32-bit hash.
///
/// ### Caution
///
/// Although this struct implements [`hash::Hasher`][], it only calculates a
/// 32-bit number, leaving the upper bits as 0. This means it is
/// unlikely to be correct to use this in places like a [`HashMap`][std::collections::HashMap].
#[derive(Debug, Clone, PartialEq)]
pub struct Hasher {
    seed: u32,
    accumulators: Accumulators,
    buffer: Buffer,
    length: u64,
}

impl Default for Hasher {
    fn default() -> Self {
        Self::with_seed(0)
    }
}

impl Hasher {
    /// Hash all data at once. If you can use this function, you may
    /// see noticable speed gains for certain types of input.
    #[must_use]
    // RATIONALE[inline]: Keeping parallel to the 64-bit
    // implementation, even though the performance gains for the
    // 32-bit version haven't been tested.
    #[inline]
    pub fn oneshot(seed: u32, data: &[u8]) -> u32 {
        let len = data.len();

        // Since we know that there's no more data coming, we don't
        // need to construct the intermediate buffers or copy data to
        // or from the buffers.

        let mut accumulators = Accumulators::new(seed);

        let data = accumulators.write_many(data);

        Self::finish_with(seed, len.into_u64(), &accumulators, data)
    }

    /// Constructs the hasher with an initial seed.
    #[must_use]
    pub const fn with_seed(seed: u32) -> Self {
        // Step 1. Initialize internal accumulators
        Self {
            seed,
            accumulators: Accumulators::new(seed),
            buffer: Buffer::new(),
            length: 0,
        }
    }

    /// The seed this hasher was created with.
    pub const fn seed(&self) -> u32 {
        self.seed
    }

    /// The total number of bytes hashed.
    pub const fn total_len(&self) -> u64 {
        self.length
    }

    /// The total number of bytes hashed, truncated to 32 bits.
    ///
    /// For the full 64-bit byte count, use [`total_len`](Self::total_len).
    pub const fn total_len_32(&self) -> u32 {
        self.length as u32
    }

    /// Returns the hash value for the values written so far. Unlike
    /// [`hash::Hasher::finish`][], this method returns the actual 32-bit
    /// value calculated, not a 64-bit value.
    #[must_use]
    // RATIONALE: See RATIONALE[inline]
    #[inline]
    pub fn finish_32(&self) -> u32 {
        Self::finish_with(
            self.seed,
            self.length,
            &self.accumulators,
            self.buffer.remaining(),
        )
    }

    #[must_use]
    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn finish_with(seed: u32, len: u64, accumulators: &Accumulators, mut remaining: &[u8]) -> u32 {
        // Step 3. Accumulator convergence
        let mut acc = if len < BYTES_IN_LANE.into_u64() {
            seed.wrapping_add(PRIME32_5)
        } else {
            accumulators.finish()
        };

        // Step 4. Add input length
        //
        // "Note that, if input length is so large that it requires
        // more than 32-bits, only the lower 32-bits are added to the
        // accumulator."
        acc += len as u32;

        // Step 5. Consume remaining input
        while let Some((chunk, rest)) = remaining.split_first_chunk() {
            let lane = u32::from_ne_bytes(*chunk).to_le();

            acc = acc.wrapping_add(lane.wrapping_mul(PRIME32_3));
            acc = acc.rotate_left(17).wrapping_mul(PRIME32_4);

            remaining = rest;
        }

        for &byte in remaining {
            let lane = byte.into_u32();

            acc = acc.wrapping_add(lane.wrapping_mul(PRIME32_5));
            acc = acc.rotate_left(11).wrapping_mul(PRIME32_1);
        }

        // Step 6. Final mix (avalanche)
        acc ^= acc >> 15;
        acc = acc.wrapping_mul(PRIME32_2);
        acc ^= acc >> 13;
        acc = acc.wrapping_mul(PRIME32_3);
        acc ^= acc >> 16;

        acc
    }
}

impl hash::Hasher for Hasher {
    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn write(&mut self, data: &[u8]) {
        let len = data.len();

        // Step 2. Process stripes
        let (buffered_lanes, data) = self.buffer.extend(data);

        if let Some(&lanes) = buffered_lanes {
            self.accumulators.write(lanes);
        }

        let data = self.accumulators.write_many(data);

        self.buffer.set(data);

        self.length += len.into_u64();
    }

    // RATIONALE: See RATIONALE[inline]
    #[inline]
    fn finish(&self) -> u64 {
        Hasher::finish_32(self).into()
    }
}

// RATIONALE: See RATIONALE[inline]
#[inline]
const fn round(mut acc: u32, lane: u32) -> u32 {
    acc = acc.wrapping_add(lane.wrapping_mul(PRIME32_2));
    acc = acc.rotate_left(13);
    acc.wrapping_mul(PRIME32_1)
}

/// Constructs [`Hasher`][] for multiple hasher instances. See
/// the [usage warning][Hasher#caution].
#[derive(Clone)]
pub struct State(u32);

impl State {
    /// Constructs the hasher with an initial seed.
    pub fn with_seed(seed: u32) -> Self {
        Self(seed)
    }
}

impl BuildHasher for State {
    type Hasher = Hasher;

    fn build_hasher(&self) -> Self::Hasher {
        Hasher::with_seed(self.0)
    }
}

#[cfg(test)]
mod test {
    use core::{
        array,
        hash::{BuildHasherDefault, Hasher as _},
    };
    use std::collections::HashMap;

    use super::*;

    const _TRAITS: () = {
        const fn is_clone<T: Clone>() {}
        is_clone::<Hasher>();
        is_clone::<State>();
    };

    const EMPTY_BYTES: [u8; 0] = [];

    #[test]
    fn ingesting_byte_by_byte_is_equivalent_to_large_chunks() {
        let bytes = [0; 32];

        let mut byte_by_byte = Hasher::with_seed(0);
        for byte in bytes.chunks(1) {
            byte_by_byte.write(byte);
        }
        let byte_by_byte = byte_by_byte.finish();

        let mut one_chunk = Hasher::with_seed(0);
        one_chunk.write(&bytes);
        let one_chunk = one_chunk.finish();

        assert_eq!(byte_by_byte, one_chunk);
    }

    #[test]
    fn hash_of_nothing_matches_c_implementation() {
        let mut hasher = Hasher::with_seed(0);
        hasher.write(&EMPTY_BYTES);
        assert_eq!(hasher.finish(), 0x02cc_5d05);
    }

    #[test]
    fn hash_of_single_byte_matches_c_implementation() {
        let mut hasher = Hasher::with_seed(0);
        hasher.write(&[42]);
        assert_eq!(hasher.finish(), 0xe0fe_705f);
    }

    #[test]
    fn hash_of_multiple_bytes_matches_c_implementation() {
        let mut hasher = Hasher::with_seed(0);
        hasher.write(b"Hello, world!\0");
        assert_eq!(hasher.finish(), 0x9e5e_7e93);
    }

    #[test]
    fn hash_of_multiple_chunks_matches_c_implementation() {
        let bytes: [u8; 100] = array::from_fn(|i| i as u8);
        let mut hasher = Hasher::with_seed(0);
        hasher.write(&bytes);
        assert_eq!(hasher.finish(), 0x7f89_ba44);
    }

    #[test]
    fn hash_with_different_seed_matches_c_implementation() {
        let mut hasher = Hasher::with_seed(0x42c9_1977);
        hasher.write(&EMPTY_BYTES);
        assert_eq!(hasher.finish(), 0xd6bf_8459);
    }

    #[test]
    fn hash_with_different_seed_and_multiple_chunks_matches_c_implementation() {
        let bytes: [u8; 100] = array::from_fn(|i| i as u8);
        let mut hasher = Hasher::with_seed(0x42c9_1977);
        hasher.write(&bytes);
        assert_eq!(hasher.finish(), 0x6d2f_6c17);
    }

    #[test]
    fn hashes_with_different_offsets_are_the_same() {
        let bytes = [0x7c; 4096];
        let expected = Hasher::oneshot(0, &[0x7c; 64]);

        let the_same = bytes
            .windows(64)
            .map(|w| {
                let mut hasher = Hasher::with_seed(0);
                hasher.write(w);
                hasher.finish_32()
            })
            .all(|h| h == expected);
        assert!(the_same);
    }

    // This test validates wraparound/truncation behavior for very
    // large inputs of a 32-bit hash, but runs very slowly in the
    // normal "cargo test" build config since it hashes 4.3GB of
    // data. It runs reasonably quick under "cargo test --release".
    #[ignore]
    #[test]
    fn length_overflows_32bit() {
        // Hash 4.3 billion (4_300_000_000) bytes, which overflows a u32.
        let bytes200: [u8; 200] = array::from_fn(|i| i as _);

        let mut hasher = Hasher::with_seed(0);
        for _ in 0..(4_300_000_000 / bytes200.len()) {
            hasher.write(&bytes200);
        }

        assert_eq!(hasher.total_len(), 0x0000_0001_004c_cb00);
        assert_eq!(hasher.total_len_32(), 0x004c_cb00);

        // compared against the C implementation
        assert_eq!(hasher.finish(), 0x1522_4ca7);
    }

    #[test]
    fn can_be_used_in_a_hashmap_with_a_default_seed() {
        let mut hash: HashMap<_, _, BuildHasherDefault<Hasher>> = Default::default();
        hash.insert(42, "the answer");
        assert_eq!(hash.get(&42), Some(&"the answer"));
    }
}

#[cfg(feature = "random")]
#[cfg_attr(docsrs, doc(cfg(feature = "random")))]
mod random_impl {
    use super::*;

    /// Constructs a randomized seed and reuses it for multiple hasher
    /// instances. See the [usage warning][Hasher#caution].
    #[derive(Clone)]
    pub struct RandomState(State);

    impl Default for RandomState {
        fn default() -> Self {
            Self::new()
        }
    }

    impl RandomState {
        fn new() -> Self {
            Self(State::with_seed(rand::random()))
        }
    }

    impl BuildHasher for RandomState {
        type Hasher = Hasher;

        fn build_hasher(&self) -> Self::Hasher {
            self.0.build_hasher()
        }
    }

    #[cfg(test)]
    mod test {
        use std::collections::HashMap;

        use super::*;

        const _: () = {
            const fn is_clone<T: Clone>() {}
            is_clone::<Hasher>();
            is_clone::<RandomState>();
        };

        #[test]
        fn can_be_used_in_a_hashmap_with_a_random_seed() {
            let mut hash: HashMap<_, _, RandomState> = Default::default();
            hash.insert(42, "the answer");
            assert_eq!(hash.get(&42), Some(&"the answer"));
        }
    }
}

#[cfg(feature = "random")]
#[cfg_attr(docsrs, doc(cfg(feature = "random")))]
pub use random_impl::*;

#[cfg(feature = "serialize")]
#[cfg_attr(docsrs, doc(cfg(feature = "serialize")))]
mod serialize_impl {
    use serde::{Deserialize, Serialize};

    use super::*;

    impl<'de> Deserialize<'de> for Hasher {
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: serde::Deserializer<'de>,
        {
            let shim = Deserialize::deserialize(deserializer)?;

            let Shim {
                total_len,
                seed,
                core,
                buffer,
                buffer_usage,
            } = shim;
            let Core { v1, v2, v3, v4 } = core;

            let mut buffer_data = BufferData::new();
            buffer_data.bytes_mut().copy_from_slice(&buffer);

            Ok(Hasher {
                seed,
                accumulators: Accumulators([v1, v2, v3, v4]),
                buffer: Buffer {
                    offset: buffer_usage,
                    data: buffer_data,
                },
                length: total_len,
            })
        }
    }

    impl Serialize for Hasher {
        fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: serde::Serializer,
        {
            let Hasher {
                seed,
                ref accumulators,
                ref buffer,
                length,
            } = *self;
            let [v1, v2, v3, v4] = accumulators.0;
            let Buffer { offset, ref data } = *buffer;
            let buffer = *data.bytes();

            let shim = Shim {
                total_len: length,
                seed,
                core: Core { v1, v2, v3, v4 },
                buffer,
                buffer_usage: offset,
            };

            shim.serialize(serializer)
        }
    }

    #[derive(Serialize, Deserialize)]
    struct Shim {
        total_len: u64,
        seed: u32,
        core: Core,
        buffer: [u8; 16],
        buffer_usage: usize,
    }

    #[derive(Serialize, Deserialize)]
    struct Core {
        v1: u32,
        v2: u32,
        v3: u32,
        v4: u32,
    }

    #[cfg(test)]
    mod test {
        use std::hash::Hasher as _;

        use super::*;

        type Result<T = (), E = serde_json::Error> = core::result::Result<T, E>;

        #[test]
        fn test_serialization_cycle() -> Result {
            let mut hasher = Hasher::with_seed(0);
            hasher.write(b"Hello, world!\0");
            let _ = hasher.finish();

            let serialized = serde_json::to_string(&hasher)?;
            let unserialized: Hasher = serde_json::from_str(&serialized)?;
            assert_eq!(hasher, unserialized);
            Ok(())
        }

        #[test]
        fn test_serialization_stability() -> Result {
            let mut hasher = Hasher::with_seed(0);
            hasher.write(b"Hello, world!\0");
            let _ = hasher.finish();

            let expected_serialized = r#"{
                "total_len": 14,
                "seed": 0,
                "core": {
                  "v1": 606290984,
                  "v2": 2246822519,
                  "v3": 0,
                  "v4": 1640531535
                },
                "buffer": [
                  72,  101, 108, 108, 111, 44, 32, 119,
                  111, 114, 108, 100, 33,  0,  0,  0
                ],
                "buffer_usage": 14
            }"#;

            let unserialized: Hasher = serde_json::from_str(expected_serialized)?;
            assert_eq!(hasher, unserialized);

            let expected_value: serde_json::Value = serde_json::from_str(expected_serialized)?;
            let actual_value = serde_json::to_value(&hasher)?;
            assert_eq!(expected_value, actual_value);

            Ok(())
        }
    }
}
