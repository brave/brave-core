use core::{hint::assert_unchecked, mem};

use super::SliceBackport as _;

#[cfg(feature = "xxhash3_128")]
use super::pairs_of_u64_bytes;

/// The minimum length of a secret.
pub const SECRET_MINIMUM_LENGTH: usize = 136;

#[repr(transparent)]
pub struct Secret([u8]);

type MinimumSecret = [u8; SECRET_MINIMUM_LENGTH];

impl Secret {
    #[inline]
    pub fn new(bytes: &[u8]) -> Result<&Self, Error> {
        // Safety: We check for validity before returning.
        unsafe {
            let this = Self::new_unchecked(bytes);
            if this.is_valid() {
                Ok(this)
            } else {
                Err(Error(()))
            }
        }
    }

    /// # Safety
    ///
    /// You must ensure that the secret byte length is >=
    /// SECRET_MINIMUM_LENGTH.
    #[inline]
    pub const unsafe fn new_unchecked(bytes: &[u8]) -> &Self {
        // Safety: We are `#[repr(transparent)]`. It's up to the
        // caller to ensure the length
        unsafe { mem::transmute(bytes) }
    }

    #[inline]
    #[cfg(feature = "xxhash3_64")]
    pub fn for_64(&self) -> Secret64BitView<'_> {
        Secret64BitView(self.minimum())
    }

    #[inline]
    #[cfg(feature = "xxhash3_128")]
    pub fn for_128(&self) -> Secret128BitView<'_> {
        Secret128BitView(self)
    }

    /// The prefix that every input below the 241-byte cutoff reads
    /// from. Handing the short paths a fixed-size array tells the
    /// compiler how many secret words there are to walk, which it
    /// otherwise has to rediscover from the secret's length on every
    /// call.
    #[inline]
    pub fn minimum(&self) -> &MinimumSecret {
        self.reassert_preconditions();

        self.0.first_chunk().unwrap()
    }

    #[inline]
    pub fn words_for_17_to_128(&self) -> &[[u8; 16]] {
        let (words, _) = self.minimum().bp_as_chunks();
        words
    }

    /// # Safety
    ///
    /// `i` must be less than the number of stripes in the secret
    /// ([`Self::n_stripes`][]).
    #[inline]
    pub unsafe fn stripe(&self, i: usize) -> &[u8; 64] {
        self.reassert_preconditions();

        // Safety: The caller has ensured that `i` is
        // in-bounds. `&[u8]` and `&[u8; 64]` have the same alignment.
        unsafe {
            debug_assert!(i < self.n_stripes());
            &*self.0.get_unchecked(i * 8..).as_ptr().cast()
        }
    }

    #[inline]
    pub fn last_stripe(&self) -> &[u8; 64] {
        self.reassert_preconditions();

        self.0.last_chunk().unwrap()
    }

    #[inline]
    pub fn last_stripe_secret_better_name(&self) -> &[u8; 64] {
        self.reassert_preconditions();

        self.0[self.0.len() - 71..].first_chunk().unwrap()
    }

    #[inline]
    pub fn final_secret(&self) -> &[u8; 64] {
        self.reassert_preconditions();

        self.0[11..].first_chunk().unwrap()
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.0.len()
    }

    #[inline]
    pub fn n_stripes(&self) -> usize {
        // stripes_per_block
        (self.len() - 64) / 8
    }

    #[inline(always)]
    fn reassert_preconditions(&self) {
        // Safety: The length of the bytes was checked at value
        // construction time.
        unsafe {
            debug_assert!(self.is_valid());
            assert_unchecked(self.is_valid());
        }
    }

    #[inline(always)]
    pub fn is_valid(&self) -> bool {
        self.0.len() >= SECRET_MINIMUM_LENGTH
    }
}

#[derive(Copy, Clone)]
#[cfg(feature = "xxhash3_64")]
pub struct Secret64BitView<'a>(&'a MinimumSecret);

#[cfg(feature = "xxhash3_64")]
impl<'a> Secret64BitView<'a> {
    #[inline]
    pub fn words_for_0(self) -> [u64; 2] {
        let (q, _) = self.0[56..].bp_as_chunks();
        [q[0], q[1]].map(u64::from_le_bytes)
    }

    #[inline]
    pub fn words_for_1_to_3(self) -> [u32; 2] {
        let (q, _) = self.0.bp_as_chunks();
        [q[0], q[1]].map(u32::from_le_bytes)
    }

    #[inline]
    pub fn words_for_4_to_8(self) -> [u64; 2] {
        let (q, _) = self.0[8..].bp_as_chunks();
        [q[0], q[1]].map(u64::from_le_bytes)
    }

    #[inline]
    pub fn words_for_9_to_16(self) -> [u64; 4] {
        let (q, _) = self.0[24..].bp_as_chunks();
        [q[0], q[1], q[2], q[3]].map(u64::from_le_bytes)
    }

    #[inline]
    pub fn words_for_129_to_240_part1(self) -> &'a [[u8; 16]] {
        let (ss, _) = self.0.bp_as_chunks();
        ss
    }

    #[inline]
    pub fn words_for_129_to_240_part2(self) -> &'a [[u8; 16]] {
        let (ss, _) = self.0[3..].bp_as_chunks();
        ss
    }

    #[inline]
    pub fn words_for_129_to_240_part3(self) -> &'a [u8; 16] {
        self.0[119..].first_chunk().unwrap()
    }
}

#[derive(Copy, Clone)]
#[cfg(feature = "xxhash3_128")]
pub struct Secret128BitView<'a>(&'a Secret);

#[cfg(feature = "xxhash3_128")]
impl<'a> Secret128BitView<'a> {
    #[inline]
    pub fn words_for_0(self) -> [u64; 4] {
        let (q, _) = self.minimum()[64..].bp_as_chunks();
        [q[0], q[1], q[2], q[3]].map(u64::from_le_bytes)
    }

    #[inline]
    pub fn words_for_1_to_3(self) -> [u32; 4] {
        let (q, _) = self.minimum().bp_as_chunks();
        [q[0], q[1], q[2], q[3]].map(u32::from_le_bytes)
    }

    #[inline]
    pub fn words_for_4_to_8(self) -> [u64; 2] {
        let (q, _) = self.minimum()[16..].bp_as_chunks();
        [q[0], q[1]].map(u64::from_le_bytes)
    }

    #[inline]
    pub fn words_for_9_to_16(self) -> [u64; 4] {
        let (q, _) = self.minimum()[32..].bp_as_chunks();
        [q[0], q[1], q[2], q[3]].map(u64::from_le_bytes)
    }

    #[inline]
    pub fn words_for_129_to_240_part1(self) -> &'a [[[u8; 16]; 2]] {
        pairs_of_u64_bytes(self.minimum())
    }

    #[inline]
    pub fn words_for_129_to_240_part2(self) -> &'a [[[u8; 16]; 2]] {
        pairs_of_u64_bytes(&self.minimum()[3..])
    }

    #[inline]
    pub fn words_for_129_to_240_part3(self) -> &'a [[u8; 16]; 2] {
        pairs_of_u64_bytes(&self.minimum()[103..]).first().unwrap()
    }

    #[inline]
    pub fn final_secret(self) -> &'a [u8; 64] {
        self.0.reassert_preconditions();

        let b = self.b();
        b[b.len() - 75..].first_chunk().unwrap()
    }

    #[inline]
    fn minimum(self) -> &'a MinimumSecret {
        self.0.minimum()
    }

    fn b(self) -> &'a [u8] {
        &(self.0).0
    }
}

#[derive(Debug)]
pub struct Error(());

impl core::error::Error for Error {}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(
            f,
            "The secret must have at least {SECRET_MINIMUM_LENGTH} bytes"
        )
    }
}
