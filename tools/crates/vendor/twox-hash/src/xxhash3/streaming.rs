use core::hint::assert_unchecked;

use super::{large::INITIAL_ACCUMULATORS, *};

/// A buffer containing the secret bytes.
///
/// # Safety
///
/// Must always return a slice with the same number of elements.
pub unsafe trait FixedBuffer: AsRef<[u8]> {}

/// A mutable buffer to contain the secret bytes.
///
/// # Safety
///
/// Must always return a slice with the same number of elements. The
/// slice must always be the same as that returned from
/// [`AsRef::as_ref`][].
pub unsafe trait FixedMutBuffer: FixedBuffer + AsMut<[u8]> {}

// Safety: An array will never change size.
unsafe impl<const N: usize> FixedBuffer for [u8; N] {}

// Safety: An array will never change size.
unsafe impl<const N: usize> FixedMutBuffer for [u8; N] {}

// Safety: An array will never change size.
unsafe impl<const N: usize> FixedBuffer for &[u8; N] {}

// Safety: An array will never change size.
unsafe impl<const N: usize> FixedBuffer for &mut [u8; N] {}

// Safety: An array will never change size.
unsafe impl<const N: usize> FixedMutBuffer for &mut [u8; N] {}

const STRIPE_BYTES: usize = 64;
const BUFFERED_STRIPES: usize = 4;
const BUFFERED_BYTES: usize = STRIPE_BYTES * BUFFERED_STRIPES;
type Buffer = [u8; BUFFERED_BYTES];

// Ensure that a full buffer always implies we are in the 241+ byte case.
const _: () = assert!(BUFFERED_BYTES > CUTOFF);

/// Holds secret and temporary buffers that are ensured to be
/// appropriately sized.
#[derive(Clone)]
pub struct SecretBuffer<S> {
    seed: u64,
    secret: S,
    buffer: Buffer,
}

impl<S> SecretBuffer<S> {
    /// Returns the secret.
    pub fn into_secret(self) -> S {
        self.secret
    }
}

impl<S> SecretBuffer<S>
where
    S: FixedBuffer,
{
    /// Takes the seed, secret, and buffer and performs no
    /// modifications to them, only validating that the sizes are
    /// appropriate.
    pub fn new(seed: u64, secret: S) -> Result<Self, SecretTooShortError<S>> {
        match Secret::new(secret.as_ref()) {
            Ok(_) => Ok(Self {
                seed,
                secret,
                buffer: [0; BUFFERED_BYTES],
            }),
            Err(e) => Err(SecretTooShortError(e, secret)),
        }
    }

    #[inline(always)]
    #[cfg(test)]
    fn is_valid(&self) -> bool {
        let secret = self.secret.as_ref();

        secret.len() >= SECRET_MINIMUM_LENGTH
    }

    #[inline]
    fn n_stripes(&self) -> usize {
        Self::secret(&self.secret).n_stripes()
    }

    #[inline]
    fn parts(&self) -> (u64, &Secret, &Buffer) {
        (self.seed, Self::secret(&self.secret), &self.buffer)
    }

    #[inline]
    fn parts_mut(&mut self) -> (u64, &Secret, &mut Buffer) {
        (self.seed, Self::secret(&self.secret), &mut self.buffer)
    }

    fn secret(secret: &S) -> &Secret {
        let secret = secret.as_ref();
        // Safety: We established the length at construction and the
        // length is not allowed to change.
        unsafe { Secret::new_unchecked(secret) }
    }
}

impl<S> SecretBuffer<S>
where
    S: FixedMutBuffer,
{
    /// Fills the secret buffer with a secret derived from the seed
    /// and the default secret. The secret must be exactly
    /// [`DEFAULT_SECRET_LENGTH`][] bytes long.
    pub fn with_seed(seed: u64, mut secret: S) -> Result<Self, SecretWithSeedError<S>> {
        match <&mut DefaultSecret>::try_from(secret.as_mut()) {
            Ok(secret_slice) => {
                *secret_slice = DEFAULT_SECRET_RAW;
                derive_secret(seed, secret_slice);

                Ok(Self {
                    seed,
                    secret,
                    buffer: [0; BUFFERED_BYTES],
                })
            }
            Err(_) => Err(SecretWithSeedError(secret)),
        }
    }
}

impl SecretBuffer<&'static [u8; DEFAULT_SECRET_LENGTH]> {
    /// Use the default seed and secret values while allocating nothing.
    #[inline]
    pub const fn default() -> Self {
        SecretBuffer {
            seed: DEFAULT_SEED,
            secret: &DEFAULT_SECRET_RAW,
            buffer: [0; BUFFERED_BYTES],
        }
    }
}

#[derive(Clone)]
pub struct RawHasherCore<S> {
    secret_buffer: SecretBuffer<S>,
    buffer_usage: usize,
    stripe_accumulator: StripeAccumulator,
    total_bytes: usize,
}

impl<S> RawHasherCore<S> {
    pub fn new(secret_buffer: SecretBuffer<S>) -> Self {
        Self {
            secret_buffer,
            buffer_usage: 0,
            stripe_accumulator: StripeAccumulator::new(),
            total_bytes: 0,
        }
    }

    pub fn into_secret(self) -> S {
        self.secret_buffer.into_secret()
    }
}

impl<S> RawHasherCore<S>
where
    S: FixedBuffer,
{
    #[inline]
    pub fn write(&mut self, input: &[u8]) {
        let this = self;
        dispatch! {
            fn write_impl<S>(this: &mut RawHasherCore<S>, input: &[u8])
            [S: FixedBuffer]
        }
    }

    #[inline]
    pub fn finish<F>(&self, finalize: F) -> F::Output
    where
        F: Finalize,
    {
        let this = self;
        dispatch! {
            fn finish_impl<S, F>(this: &RawHasherCore<S>, finalize: F) -> F::Output
            [S: FixedBuffer, F: Finalize]
        }
    }
}

#[inline(always)]
fn write_impl<S>(vector: impl Vector, this: &mut RawHasherCore<S>, mut input: &[u8])
where
    S: FixedBuffer,
{
    if input.is_empty() {
        return;
    }

    let RawHasherCore {
        secret_buffer,
        buffer_usage,
        stripe_accumulator,
        total_bytes,
        ..
    } = this;

    let n_stripes = secret_buffer.n_stripes();
    let (_, secret, buffer) = secret_buffer.parts_mut();

    *total_bytes += input.len();

    // Safety: This is an invariant of the buffer.
    unsafe {
        debug_assert!(*buffer_usage <= buffer.len());
        assert_unchecked(*buffer_usage <= buffer.len())
    };

    // We have some previous data saved; try to fill it up and process it first
    if !buffer.is_empty() {
        let remaining = &mut buffer[*buffer_usage..];
        let n_to_copy = usize::min(remaining.len(), input.len());

        let (remaining_head, remaining_tail) = remaining.split_at_mut(n_to_copy);
        let (input_head, input_tail) = input.split_at(n_to_copy);

        remaining_head.copy_from_slice(input_head);
        *buffer_usage += n_to_copy;

        input = input_tail;

        // We did not fill up the buffer
        if !remaining_tail.is_empty() {
            return;
        }

        // We don't know this isn't the last of the data
        if input.is_empty() {
            return;
        }

        let (stripes, _) = buffer.bp_as_chunks();
        for stripe in stripes {
            stripe_accumulator.process_stripe(vector, stripe, n_stripes, secret);
        }
        *buffer_usage = 0;
    }

    debug_assert!(*buffer_usage == 0);

    // Process as much of the input data in-place as possible,
    // while leaving at least one full stripe for the
    // finalization.
    if let Some(len) = input.len().checked_sub(STRIPE_BYTES) {
        let full_block_point = (len / STRIPE_BYTES) * STRIPE_BYTES;
        // Safety: We know that `full_block_point` must be less than
        // `input.len()` as we subtracted and then integer-divided
        // (which rounds down) and then multiplied back. That's not
        // evident to the compiler and `split_at` results in a
        // potential panic.
        //
        // https://github.com/llvm/llvm-project/issues/104827
        let (stripes, remainder) = unsafe { input.split_at_unchecked(full_block_point) };
        let (stripes, _) = stripes.bp_as_chunks();

        for stripe in stripes {
            stripe_accumulator.process_stripe(vector, stripe, n_stripes, secret)
        }
        input = remainder;
    }

    // Any remaining data has to be less than the buffer, and the
    // buffer is empty so just fill up the buffer.
    debug_assert!(*buffer_usage == 0);
    debug_assert!(!input.is_empty());

    // Safety: We have parsed all the full blocks of input except one
    // and potentially a full block minus one byte. That amount of
    // data must be less than the buffer.
    let buffer_head = unsafe {
        debug_assert!(input.len() < 2 * STRIPE_BYTES);
        debug_assert!(2 * STRIPE_BYTES < buffer.len());
        buffer.get_unchecked_mut(..input.len())
    };

    buffer_head.copy_from_slice(input);
    *buffer_usage = input.len();
}

#[inline(always)]
fn finish_impl<S, F>(vector: impl Vector, this: &RawHasherCore<S>, finalize: F) -> F::Output
where
    S: FixedBuffer,
    F: Finalize,
{
    let RawHasherCore {
        ref secret_buffer,
        buffer_usage,
        mut stripe_accumulator,
        total_bytes,
    } = *this;

    let n_stripes = secret_buffer.n_stripes();
    let (seed, secret, buffer) = secret_buffer.parts();

    // Safety: This is an invariant of the buffer.
    unsafe {
        debug_assert!(buffer_usage <= buffer.len());
        assert_unchecked(buffer_usage <= buffer.len())
    };

    if total_bytes > CUTOFF {
        let input = &buffer[..buffer_usage];

        // Ingest final stripes
        let (stripes, remainder) = stripes_with_tail(input);
        for stripe in stripes {
            stripe_accumulator.process_stripe(vector, stripe, n_stripes, secret);
        }

        let mut temp = [0; 64];

        let last_stripe = match input.last_chunk() {
            Some(chunk) => chunk,
            None => {
                let n_to_reuse = 64 - input.len();
                let to_reuse = buffer.len() - n_to_reuse;

                let (temp_head, temp_tail) = temp.split_at_mut(n_to_reuse);
                temp_head.copy_from_slice(&buffer[to_reuse..]);
                temp_tail.copy_from_slice(input);

                &temp
            }
        };

        finalize.large(
            vector,
            stripe_accumulator.accumulator,
            remainder,
            last_stripe,
            secret,
            total_bytes,
        )
    } else {
        finalize.small(DEFAULT_SECRET, seed, &buffer[..total_bytes])
    }
}

pub trait Finalize {
    type Output;

    fn small(&self, secret: &Secret, seed: u64, input: &[u8]) -> Self::Output;

    fn large(
        &self,
        vector: impl Vector,
        acc: [u64; 8],
        last_block: &[u8],
        last_stripe: &[u8; 64],
        secret: &Secret,
        len: usize,
    ) -> Self::Output;
}

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub mod with_alloc {
    use ::alloc::boxed::Box;

    use super::*;

    // Safety: A plain slice will never change size.
    unsafe impl FixedBuffer for Box<[u8]> {}

    // Safety: A plain slice will never change size.
    unsafe impl FixedMutBuffer for Box<[u8]> {}

    type AllocSecretBuffer = SecretBuffer<Box<[u8]>>;

    impl AllocSecretBuffer {
        /// Allocates the secret and temporary buffers and fills them
        /// with the default seed and secret values.
        pub fn allocate_default() -> Self {
            Self {
                seed: DEFAULT_SEED,
                secret: DEFAULT_SECRET_RAW.to_vec().into(),
                buffer: [0; BUFFERED_BYTES],
            }
        }

        /// Allocates the secret and temporary buffers and uses the
        /// provided seed to construct the secret value.
        pub fn allocate_with_seed(seed: u64) -> Self {
            let mut secret = DEFAULT_SECRET_RAW;
            derive_secret(seed, &mut secret);

            Self {
                seed,
                secret: secret.to_vec().into(),
                buffer: [0; BUFFERED_BYTES],
            }
        }

        /// Allocates the temporary buffer and uses the provided seed
        /// and secret buffer.
        pub fn allocate_with_seed_and_secret(
            seed: u64,
            secret: impl Into<Box<[u8]>>,
        ) -> Result<Self, SecretTooShortError<Box<[u8]>>> {
            Self::new(seed, secret.into())
        }
    }

    pub type AllocRawHasher = RawHasherCore<Box<[u8]>>;

    impl AllocRawHasher {
        pub fn allocate_default() -> Self {
            Self::new(SecretBuffer::allocate_default())
        }

        pub fn allocate_with_seed(seed: u64) -> Self {
            Self::new(SecretBuffer::allocate_with_seed(seed))
        }

        pub fn allocate_with_seed_and_secret(
            seed: u64,
            secret: impl Into<Box<[u8]>>,
        ) -> Result<Self, SecretTooShortError<Box<[u8]>>> {
            SecretBuffer::allocate_with_seed_and_secret(seed, secret).map(Self::new)
        }
    }
}

#[cfg(feature = "alloc")]
pub use with_alloc::AllocRawHasher;

/// Tracks which stripe we are currently on to know which part of the
/// secret we should be using.
#[derive(Copy, Clone)]
pub struct StripeAccumulator {
    pub accumulator: [u64; 8],
    current_stripe: usize,
}

impl StripeAccumulator {
    pub fn new() -> Self {
        Self {
            accumulator: INITIAL_ACCUMULATORS,
            current_stripe: 0,
        }
    }

    #[inline]
    pub fn process_stripe(
        &mut self,
        vector: impl Vector,
        stripe: &[u8; 64],
        n_stripes: usize,
        secret: &Secret,
    ) {
        let Self {
            accumulator,
            current_stripe,
            ..
        } = self;

        // For each stripe

        // Safety: The number of stripes is determined by the
        // block size, which is determined by the secret size.
        let secret_stripe = unsafe { secret.stripe(*current_stripe) };
        vector.accumulate(accumulator, stripe, secret_stripe);

        *current_stripe += 1;

        // After a full block's worth
        if *current_stripe == n_stripes {
            let secret_end = secret.last_stripe();
            vector.round_scramble(accumulator, secret_end);

            *current_stripe = 0;
        }
    }
}

/// The provided secret was not exactly [`DEFAULT_SECRET_LENGTH`][]
/// bytes.
pub struct SecretWithSeedError<S>(S);

impl<S> SecretWithSeedError<S> {
    /// Returns the secret.
    pub fn into_secret(self) -> S {
        self.0
    }
}

impl<S> core::error::Error for SecretWithSeedError<S> {}

impl<S> core::fmt::Debug for SecretWithSeedError<S> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_tuple("SecretWithSeedError").finish()
    }
}

impl<S> core::fmt::Display for SecretWithSeedError<S> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(
            f,
            "The secret must be exactly {DEFAULT_SECRET_LENGTH} bytes"
        )
    }
}

/// The provided secret was not at least [`SECRET_MINIMUM_LENGTH`][]
/// bytes.
pub struct SecretTooShortError<S>(secret::Error, S);

impl<S> SecretTooShortError<S> {
    /// Returns the secret.
    pub fn into_secret(self) -> S {
        self.1
    }
}

impl<S> core::error::Error for SecretTooShortError<S> {}

impl<S> core::fmt::Debug for SecretTooShortError<S> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_tuple("SecretTooShortError").finish()
    }
}

impl<S> core::fmt::Display for SecretTooShortError<S> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        self.0.fmt(f)
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn secret_buffer_default_is_valid() {
        assert!(SecretBuffer::default().is_valid());
    }

    #[test]
    fn secret_buffer_allocate_default_is_valid() {
        assert!(SecretBuffer::allocate_default().is_valid())
    }

    #[test]
    fn secret_buffer_allocate_with_seed_is_valid() {
        assert!(SecretBuffer::allocate_with_seed(0xdead_beef).is_valid())
    }
}
