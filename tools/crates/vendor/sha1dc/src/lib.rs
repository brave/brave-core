#![no_std]
#![warn(missing_docs, unreachable_pub)]

//! SHA-1 is cryptographically broken, because chosen-prefix collisions against it are practical.
//! However, there are still cases where it is needed for compatibility, such as in `git`'s object
//! identifiers.
//!
//! To mitigate this security issue, this crate detects those manufactured collisions. It follows
//! the method of Marc Stevens and Dan Shumow, which finds the message blocks that a collision
//! attack produces and reports them. The [paper] describes the method, and
//! [sha1collisiondetection] is the authors' own implementation, which the tests compare against.
//!
//! Where available, the implementation uses SHA-1 hardware instructions on `x86_64` and `aarch64`,
//! as well as SIMD-enabled algorithms. Nonetheless, detection does more work per block than plain
//! SHA-1, and costs 16% to 22% of throughput, depending on the machine.
//!
//! Two modes are provided, as two separate `Hasher` structs. [`Hasher`] keeps the standard digest,
//! with output equivalent to a non-detecting SHA-1 implementation. [`mitigate::Hasher`] computes an
//! alternative digest instead. Both report a detected attack as an error.
//!
//! ## Example
//!
//! ```rust
//! use sha1dc::Hasher;
//!
//! let mut hasher = Hasher::new();
//! hasher.update(b"hello ");
//! hasher.update(b"world");
//!
//! match hasher.finalize() {
//!     Ok(digest) => println!("{digest}"),
//!     Err(collision) => println!("refusing {}", collision.digest()),
//! }
//! ```
//!
//! [sha1collisiondetection]: https://github.com/cr-marcstevens/sha1collisiondetection
//! [paper]: https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-stevens.pdf

#[cfg(feature = "std")]
extern crate std;

use core::fmt;

pub mod mitigate;

mod block;
mod mem;
mod ubc_check;

use block::Backend;

/// Size of a SHA-1 digest in bytes.
const DIGEST_SIZE: usize = 20;

/// Size of the SHA-1 compression block in bytes.
const BLOCK_SIZE: usize = 64;

const STATE_LEN: usize = 5;
const INITIAL_H: [u32; STATE_LEN] = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0];

/// How many words the expanded message schedule has.
pub(crate) const SCHEDULE_LEN: usize = 80;

/// The expanded message schedule, as a compression spilled it.
///
/// Indexed by step number, which is not always where the word sits: on `x86`
/// the array runs backwards. `sha1rnds4` hands each group of four words back
/// reversed, so storing the group at the mirrored offset undoes that for
/// nothing, where putting it in order costs a `pshufd` a group — twenty a
/// block, and worth 2.1 ns on a core with no spare slots to hide them in.
/// `aarch64` produces its words in order and mirroring would cost it those
/// same shuffles, so the layout follows the target and this type is the whole
/// of where that is known.
#[derive(Clone)]
#[repr(transparent)]
pub(crate) struct Schedule([u32; SCHEDULE_LEN]);

impl Schedule {
    pub(crate) const fn zeroed() -> Self {
        Self([0; SCHEDULE_LEN])
    }

    /// A schedule from words already in storage order, for the property
    /// tests, which check that the forms agree on any contents at all.
    ///
    /// Those need `quickcheck`, so they and this go together.
    #[cfg(all(test, feature = "std"))]
    pub(crate) const fn from_words(words: [u32; SCHEDULE_LEN]) -> Self {
        Self(words)
    }

    /// Expands 16 message words the way SHA-1 does, for the tests.
    ///
    /// Written out apart from the round code, so a test that compares against
    /// it cannot share a fault with what it checks.
    #[cfg(all(test, feature = "std"))]
    pub(crate) fn expand(m: &[u32; 16]) -> Self {
        let mut w = Self::zeroed();
        for (t, word) in m.iter().enumerate() {
            w[t] = *word;
        }
        for t in 16..SCHEDULE_LEN {
            w[t] = (w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16]).rotate_left(1);
        }
        w
    }

    /// The words as they are stored, for the property tests that compare two
    /// whole schedules.
    #[cfg(all(test, feature = "std"))]
    pub(crate) const fn words(&self) -> &[u32; SCHEDULE_LEN] {
        &self.0
    }

    /// Whether the array runs backwards.
    pub(crate) const MIRRORED: bool = cfg!(any(target_arch = "x86", target_arch = "x86_64"));

    /// Where step `t` sits.
    #[inline(always)]
    const fn at(t: usize) -> usize {
        if Self::MIRRORED {
            SCHEDULE_LEN - 1 - t
        } else {
            t
        }
    }
}

/// Where the words physically sit, for the code that moves a whole window
/// rather than one word at a time: a hardware backend's spill, and the vector
/// forms of the filter.
///
/// A target with neither — no SHA-1 instructions and no vector unit — calls
/// none of these, and reaches every word through [`Index`](core::ops::Index)
/// instead. `i586` calls two of the three, having a backend but no vector
/// filter.
#[allow(dead_code, reason = "a target without either kind calls none of these")]
impl Schedule {
    /// The `N` words of steps `T..T + N`, as they are stored: a mirrored
    /// layout hands them back in the other order.
    ///
    /// `T` and `N` are const parameters, so a window past the end of the
    /// schedule is a compile error at the call site, and the bounds check
    /// folds away.
    #[inline(always)]
    pub(crate) fn window<const T: usize, const N: usize>(&self) -> &[u32; N] {
        let at = const { Self::window_start(T, N) };
        self.0[at..at + N].try_into().unwrap()
    }

    /// The same, for a backend writing a whole window.
    #[inline(always)]
    pub(crate) fn window_mut<const T: usize, const N: usize>(&mut self) -> &mut [u32; N] {
        let at = const { Self::window_start(T, N) };
        (&mut self.0[at..at + N]).try_into().unwrap()
    }

    /// Where a window of `n` consecutive steps starting at `t` begins.
    ///
    /// A run of steps is a run of words either way round; a mirrored one
    /// starts at the other end.
    const fn window_start(t: usize, n: usize) -> usize {
        assert!(t + n <= SCHEDULE_LEN, "the window runs past the schedule");
        if Self::MIRRORED {
            SCHEDULE_LEN - n - t
        } else {
            t
        }
    }
}

impl core::ops::Index<usize> for Schedule {
    type Output = u32;

    #[inline(always)]
    fn index(&self, t: usize) -> &u32 {
        &self.0[Self::at(t)]
    }
}

impl core::ops::IndexMut<usize> for Schedule {
    #[inline(always)]
    fn index_mut(&mut self, t: usize) -> &mut u32 {
        &mut self.0[Self::at(t)]
    }
}

/// A SHA-1 digest.
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Digest([u8; DIGEST_SIZE]);

impl Digest {
    /// The digest as a byte array.
    pub const fn as_bytes(&self) -> &[u8; DIGEST_SIZE] {
        &self.0
    }

    /// Consumes the digest, returning the raw bytes.
    pub const fn to_bytes(self) -> [u8; DIGEST_SIZE] {
        self.0
    }
}

impl AsRef<[u8]> for Digest {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl From<[u8; DIGEST_SIZE]> for Digest {
    fn from(bytes: [u8; DIGEST_SIZE]) -> Self {
        Self(bytes)
    }
}

impl From<Digest> for [u8; DIGEST_SIZE] {
    fn from(digest: Digest) -> Self {
        digest.0
    }
}

impl PartialEq<[u8; DIGEST_SIZE]> for Digest {
    fn eq(&self, other: &[u8; DIGEST_SIZE]) -> bool {
        self.0 == *other
    }
}

impl PartialEq<[u8]> for Digest {
    fn eq(&self, other: &[u8]) -> bool {
        self.0 == *other
    }
}

impl fmt::LowerHex for Digest {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for byte in &self.0 {
            write!(f, "{byte:02x}")?;
        }
        Ok(())
    }
}

impl fmt::UpperHex for Digest {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for byte in &self.0 {
            write!(f, "{byte:02X}")?;
        }
        Ok(())
    }
}

impl fmt::Display for Digest {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::LowerHex::fmt(self, f)
    }
}

impl fmt::Debug for Digest {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Digest({self:x})")
    }
}

/// A collision attack was detected, and not mitigated.
///
/// [`Hasher::finalize`] returns this. The digest is the plain SHA-1 value, and
/// cannot be relied upon to be collision-free.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Collision {
    digest: Digest,
}

impl Collision {
    /// The plain SHA-1 digest, which cannot be relied upon to be
    /// collision-free.
    pub const fn digest(&self) -> Digest {
        self.digest
    }
}

impl fmt::Display for Collision {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("SHA-1 collision attack detected; digest is the colliding one")
    }
}

impl core::error::Error for Collision {}

/// Hashes `data`, reporting a detected collision attack.
///
/// This is the one-shot form of [`Hasher`]. The digest is plain SHA-1, so it
/// agrees with all other implementations. To get a digest no other message
/// shares, use [`mitigate::digest`].
///
/// # Examples
///
/// ```
/// let digest = sha1dc::digest(b"hello world").expect("no collision");
/// assert_eq!(digest.to_string(), "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed");
/// ```
pub fn digest(data: &[u8]) -> Result<Digest, Collision> {
    let mut hasher = Hasher::new();
    hasher.update(data);
    hasher.finalize()
}

/// State that both hashers share. They differ only in the result of a
/// detected collision, which the builder sets.
#[derive(Clone)]
struct Inner {
    /// What the compression reads and writes.
    state: State,
    /// Total number of bytes fed in so far.
    len: u64,
    /// The bytes of a block not yet complete, which are the message's last
    /// `len % BLOCK_SIZE`: see [`buffered`](Self::buffered). Kept apart from
    /// [`State`]: the compression takes the state alone, so a full buffer is
    /// compressed where it is, and the buffer is not handed to code the
    /// compiler cannot see.
    ///
    /// Every byte after those is zero, so `finalize` pads with only the
    /// marker and the length.
    buffer: [u8; BLOCK_SIZE],
}

/// What [`block::compress`] needs: the chaining value, the backend, and the
/// detection's settings and result. Everything a hasher holds but the buffer
/// and the length, which only `update` and `finalize` use.
#[derive(Clone)]
struct State {
    h: [u32; STATE_LEN],
    backend: Backend,
    /// Keeps every path scalar, including the UBC check, which the backend
    /// alone does not cover.
    scalar_only: bool,

    // What the detection needs. The names follow the C.
    safe_hash: bool,
    ubc_check: bool,
    reduced_round_collision: bool,
    /// True if a collision occurred.
    found_collision: bool,
}

impl Inner {
    fn new(builder: Builder, safe_hash: bool) -> Self {
        Self {
            state: State {
                h: INITIAL_H,
                backend: if builder.scalar_backend {
                    Backend::scalar()
                } else {
                    Backend::new()
                },
                scalar_only: builder.scalar_backend,
                safe_hash,
                ubc_check: builder.ubc_check,
                reduced_round_collision: builder.reduced_round_collisions,
                found_collision: false,
            },
            len: 0,
            buffer: [0; BLOCK_SIZE],
        }
    }

    /// How many bytes the buffer holds. Every byte fed in is either in a
    /// block compressed or in the buffer, which holds less than a block, so
    /// this is the length modulo a block. That survives `len` wrapping, as a
    /// block's size divides 2^64.
    fn buffered(&self) -> usize {
        (self.len % BLOCK_SIZE as u64) as usize
    }

    fn update(&mut self, mut data: &[u8]) {
        // Before the length moves on, which is what says how much is buffered.
        let pos = self.buffered();
        self.len = self.len.wrapping_add(data.len() as u64);

        if pos > 0 {
            let take = (BLOCK_SIZE - pos).min(data.len());
            self.buffer[pos..pos + take].copy_from_slice(&data[..take]);
            data = &data[take..];

            if pos + take < BLOCK_SIZE {
                return;
            }
            // The buffer and the state are different fields, so the block is
            // compressed where it is.
            block::compress(&mut self.state, &self.buffer);
            self.buffer = [0; BLOCK_SIZE];
        }

        let blocks = data.len() / BLOCK_SIZE;
        let (full, rest) = data.split_at(blocks * BLOCK_SIZE);
        if blocks > 0 {
            block::compress(&mut self.state, full);
        }
        self.buffer[..rest.len()].copy_from_slice(rest);
    }

    fn reset(&mut self) {
        self.state.h = INITIAL_H;
        self.len = 0;
        self.buffer = [0; BLOCK_SIZE];
        // Keep the configuration.
        self.state.found_collision = false;
    }

    /// Finishes and reports whether a collision occurred. The caller's own
    /// type defines what a detection means for the digest.
    fn finish(&mut self) -> (Digest, bool) {
        let digest = self.finalize_inner();
        (digest, self.state.found_collision)
    }

    /// Pads the message and compresses the final block(s).
    ///
    /// This leaves the hasher in an invalid state. It consumes the buffer but
    /// does not update `len`. Each caller must own `self` or call
    /// [`reset`](Self::reset) immediately after.
    fn finalize_inner(&mut self) -> Digest {
        let bit_len = self.len << 3;
        let pos = self.buffered();

        // The bytes after `pos` are zero already, so the padding is only the
        // marker and the length.
        self.buffer[pos] = 0x80;

        if pos + 1 > BLOCK_SIZE - 8 {
            block::compress(&mut self.state, &self.buffer);
            self.buffer = [0; BLOCK_SIZE];
        }

        self.buffer[BLOCK_SIZE - 8..].copy_from_slice(&bit_len.to_be_bytes());
        block::compress(&mut self.state, &self.buffer);

        let mut out = [0u8; DIGEST_SIZE];
        for (chunk, v) in out.chunks_exact_mut(4).zip(self.state.h.iter()) {
            chunk.copy_from_slice(&v.to_be_bytes());
        }
        Digest(out)
    }
}

/// SHA-1 hasher that detects a collision attack without changing the digest.
///
/// Digests match all other SHA-1 implementations, which is what a shared
/// identifier needs. A detected attack gives a [`Collision`], whose digest
/// applies to two messages. Refuse that digest.
///
/// Use this mode unless you know that you need the other one.
/// [`mitigate::Hasher`] applies when you must produce a usable digest during
/// an attack.
///
/// Detection is always on. For plain SHA-1 without detection, use the `sha1`
/// crate.
#[derive(Clone)]
pub struct Hasher(Inner);

impl Default for Hasher {
    fn default() -> Self {
        Self::new()
    }
}

/// Generates the methods that both hashers share. Only the result type of
/// `finalize` differs.
macro_rules! hasher_common {
    ($ty:ident, $err:ident, $detected:expr) => {
        impl fmt::Debug for $ty {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                f.debug_struct(stringify!($ty))
                    .field("collision_detected", &self.collision_detected())
                    .finish_non_exhaustive()
            }
        }

        impl $ty {
            /// Feeds more data into the hasher.
            pub fn update(&mut self, data: &[u8]) {
                self.0.update(data);
            }

            /// Finishes hashing, reporting a detected collision attack.
            pub fn finalize(mut self) -> Result<Digest, $err> {
                let (digest, detected) = self.0.finish();
                if detected {
                    Err($detected(digest))
                } else {
                    Ok(digest)
                }
            }

            /// Finishes hashing and resets the hasher, reporting a detected
            /// collision attack.
            pub fn finalize_reset(&mut self) -> Result<Digest, $err> {
                let (digest, detected) = self.0.finish();
                self.0.reset();
                if detected {
                    Err($detected(digest))
                } else {
                    Ok(digest)
                }
            }

            /// Resets the hasher to its initial state.
            pub fn reset(&mut self) {
                self.0.reset();
            }

            /// True if a collision attack occurred in the data so far.
            ///
            /// Finalization hashes the last block, so this can become true
            /// at that point.
            pub const fn collision_detected(&self) -> bool {
                self.0.state.found_collision
            }
        }

        #[cfg(feature = "std")]
        impl std::io::Write for $ty {
            fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
                self.update(buf);
                Ok(buf.len())
            }

            fn flush(&mut self) -> std::io::Result<()> {
                Ok(())
            }
        }
    };
}

pub(crate) use hasher_common;

hasher_common!(Hasher, Collision, |digest| Collision { digest });

impl Hasher {
    /// Creates a hasher.
    pub fn new() -> Self {
        Builder::new().build()
    }

    /// Creates a [`Builder`].
    ///
    /// **Not public API. Exempt from semver.** The choice of hasher decides
    /// everything a caller needs. The rest is for this crate's tests.
    #[doc(hidden)]
    pub fn builder() -> Builder {
        Builder::new()
    }
}

/// Configures a hasher's collision detection.
///
/// **Not public API. Exempt from semver.** See [`Hasher::builder`].
#[doc(hidden)]
#[derive(Clone, Debug)]
pub struct Builder {
    scalar_backend: bool,
    ubc_check: bool,
    reduced_round_collisions: bool,
}

impl Default for Builder {
    fn default() -> Self {
        Self::new()
    }
}

impl Builder {
    /// Creates a builder with the default configuration. It reports only the
    /// collisions that are a threat to full SHA-1.
    pub const fn new() -> Self {
        Self {
            scalar_backend: false,
            ubc_check: true,
            reduced_round_collisions: false,
        }
    }

    /// Whether unavoidable bit conditions are used to speed up detection.
    /// Default: `true`.
    ///
    /// **Not public API. Exempt from semver.** This is an optimization. It
    /// rejects most blocks without recompression and, under the same
    /// conjecture the paper's soundness rests on (attacks stay within the
    /// DV-prescribed local collisions over steps 35 to 64), never rejects a
    /// block recompression would have flagged. Disabling it cannot detect
    /// more, only run slower. Tests use it to cross-check the filter.
    #[doc(hidden)]
    pub const fn internal_use_ubc(mut self, use_ubc: bool) -> Self {
        self.ubc_check = use_ubc;
        self
    }

    /// Whether collisions against reduced-round SHA-1 also count as detected
    /// collisions. Default: `false`.
    ///
    /// **Not public API. Exempt from semver.** These collisions are not a
    /// threat to full SHA-1, so this setting only causes false positives.
    /// Tests use it, because a reduced-round collision is cheap to make.
    #[doc(hidden)]
    pub const fn internal_reduced_round_collisions(mut self, reduced: bool) -> Self {
        self.reduced_round_collisions = reduced;
        self
    }

    /// Sets the hasher to use the scalar implementation and not the CPU
    /// SHA-1 instructions, and the scalar form of the UBC check.
    ///
    /// **Not public API. Exempt from semver.** Tests and the benchmark use it
    /// to run the scalar paths on a machine that has the instructions. Without
    /// it, those paths run only where the instructions are absent.
    #[doc(hidden)]
    pub const fn internal_scalar_backend(mut self) -> Self {
        self.scalar_backend = true;
        self
    }

    /// Builds a hasher that does not change the digest.
    pub fn build(self) -> Hasher {
        Hasher(self.inner(false))
    }

    /// Builds a hasher that mitigates a detected attack.
    pub fn build_mitigating(self) -> mitigate::Hasher {
        mitigate::Hasher(self.inner(true))
    }

    fn inner(self, safe_hash: bool) -> Inner {
        Inner::new(self, safe_hash)
    }
}
