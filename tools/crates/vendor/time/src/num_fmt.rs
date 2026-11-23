//! Formatting utilities for numbers.
//!
//! These functions are low-level, but are designed to be _extremely_ fast for their designed use
//! cases. They have strict requirements, and may not return the most ergonomic types to avoid
//! unnecessary allocations and copying.

use core::mem::MaybeUninit;
use core::ops::Deref;
use core::{ptr, slice};

#[cfg(feature = "formatting")]
use deranged::ru64;
use deranged::{ru8, ru16, ru32};

static SINGLE_DIGITS: [u8; 10] = *b"0123456789";

static ZERO_PADDED_PAIRS: [u8; 200] = *b"0001020304050607080910111213141516171819\
                                         2021222324252627282930313233343536373839\
                                         4041424344454647484950515253545556575859\
                                         6061626364656667686970717273747576777879\
                                         8081828384858687888990919293949596979899";

#[cfg(feature = "formatting")]
static SPACE_PADDED_PAIRS: [u8; 200] = *b" 0 1 2 3 4 5 6 7 8 910111213141516171819\
                                          2021222324252627282930313233343536373839\
                                          4041424344454647484950515253545556575859\
                                          6061626364656667686970717273747576777879\
                                          8081828384858687888990919293949596979899";

/// A string type with a maximum length known at compile time, stored on the stack.
///
/// Note that while the _maximum_ length is known at compile time, the string may be shorter. This
/// information is stored inline.
#[derive(Clone, Copy)]
pub(crate) struct StackStr<const MAX_LEN: usize> {
    buf: [MaybeUninit<u8>; MAX_LEN],
    len: usize,
}

impl<const MAX_LEN: usize> StackStr<MAX_LEN> {
    /// # Safety:
    ///
    /// - `buf` must be initialized for at least `len` bytes.
    /// - The first `len` bytes of `buf` must be valid UTF-8.
    #[inline]
    pub(crate) const unsafe fn new(buf: [MaybeUninit<u8>; MAX_LEN], len: usize) -> Self {
        debug_assert!(len <= MAX_LEN);
        Self { buf, len }
    }
}

impl<const MAX_LEN: usize> Deref for StackStr<MAX_LEN> {
    type Target = str;

    #[inline]
    fn deref(&self) -> &Self::Target {
        // Safety: This type can only be constructed when the caller asserts that the buffer is
        // valid UTF-8 for the first `len` bytes.
        unsafe { str_from_raw_parts(self.buf.as_ptr().cast(), self.len) }
    }
}

/// A string type with a maximum length known at compile time, stored on the stack.
///
/// Note that while the _maximum_ length is known at compile time, the string may be shorter. This
/// information is stored inline.
#[derive(Clone, Copy)]
pub(crate) struct StackTrailingStr<const MAX_LEN: usize> {
    buf: [MaybeUninit<u8>; MAX_LEN],
    start_index: usize,
}

impl<const MAX_LEN: usize> StackTrailingStr<MAX_LEN> {
    /// # Safety:
    ///
    /// - The last `MAX_LEN - start_index` bytes of `buf` must be initialized and valid UTF-8.
    #[inline]
    pub(crate) const unsafe fn new(buf: [MaybeUninit<u8>; MAX_LEN], start_index: usize) -> Self {
        debug_assert!(start_index <= MAX_LEN);
        Self { buf, start_index }
    }

    /// Return the length of `self` in bytes.
    #[inline]
    pub(crate) const fn len(&self) -> usize {
        let len = MAX_LEN - self.start_index;
        // Safety: `self.start_index` is an unsigned integer, so `len` cannot be larger than
        // `MAX_LEN` with the arithmetic above.
        unsafe { core::hint::assert_unchecked(len <= MAX_LEN) };
        len
    }
}

impl<const MAX_LEN: usize> Deref for StackTrailingStr<MAX_LEN> {
    type Target = str;

    #[inline]
    fn deref(&self) -> &Self::Target {
        // Safety: This type can only be constructed when the caller asserts that the buffer is
        // valid UTF-8 for the last `len` bytes.
        unsafe {
            str_from_raw_parts(
                self.buf.as_ptr().add(self.start_index).cast(),
                MAX_LEN - self.start_index,
            )
        }
    }
}

/// Write a two digit integer to `buf` at `offset` and `offset + 1`.
///
/// # Safety
///
/// `buf` must be at least `offset + 2` bytes long.
#[inline]
const unsafe fn write_two_digits(buf: &mut [MaybeUninit<u8>], offset: usize, value: ru8<0, 99>) {
    // Safety: `buf` is at least `offset + 2` bytes long.
    unsafe {
        ptr::copy_nonoverlapping(
            two_digits_zero_padded(value).as_ptr().cast(),
            buf.as_mut_ptr().add(offset),
            2,
        );
    }
}

/// Write a single digit integer to `buf` at `offset`.
///
/// # Safety
///
/// `buf` must be at least `offset` bytes long.
#[inline]
const unsafe fn write_one_digit(buf: &mut [MaybeUninit<u8>], offset: usize, value: ru8<0, 9>) {
    // Safety: `buf` is at least `offset` bytes long.
    unsafe {
        ptr::copy_nonoverlapping(
            single_digit(value).as_ptr().cast(),
            buf.as_mut_ptr().add(offset),
            1,
        );
    }
}

/// # Safety
///
/// - `ptr` must be non-null and point to `len` initialized bytes of UTF-8 data.
/// - `ptr` is valid for (and not mutated during) lifetime `'a`.
#[inline]
pub(crate) const unsafe fn str_from_raw_parts<'a>(ptr: *const u8, len: usize) -> &'a str {
    // Safety: The caller must ensure that `ptr` is valid for `len` bytes and that the bytes are
    // valid UTF-8. The caller must also ensure that the lifetime `'a` is valid for the returned
    // string.
    unsafe { str::from_utf8_unchecked(slice::from_raw_parts(ptr, len)) }
}

#[inline]
const fn div_100(n: ru16<0, 9_999>) -> [ru8<0, 99>; 2] {
    const EXP: u32 = 19; // 19 is faster or equal to 12 even for 3 digits.
    const SIG: u32 = (1 << EXP) / 100 + 1;

    let n = n.get();

    let high = (n as u32 * SIG) >> EXP; // value / 100
    let low = n as u32 - high * 100;

    // Safety: `high` is guaranteed to be less than 100 and `low` is guaranteed to be less than 100
    // due to the arithmetic above.
    unsafe {
        [
            ru8::new_unchecked(high as u8),
            ru8::new_unchecked(low as u8),
        ]
    }
}

/// Obtain a string containing a single ASCII digit representing `n`.
#[inline]
pub(crate) const fn single_digit(n: ru8<0, 9>) -> &'static str {
    // Safety: We're staying within the bounds of the array. The array contains only ASCII
    // characters, so it's valid UTF-8.
    unsafe { str_from_raw_parts(SINGLE_DIGITS.as_ptr().add(n.get() as usize), 1) }
}

/// Obtain a string of one or two ASCII digits representing `n`. No leading zeros or spaces are
/// included.
#[inline]
pub(crate) const fn one_to_two_digits_no_padding(n: ru8<0, 99>) -> &'static str {
    let n = n.get();
    let is_single_digit = n < 10;
    // Safety: We're staying within the bounds of the array. The array contains only ASCII
    // characters, so it's valid UTF-8.
    unsafe {
        str_from_raw_parts(
            ZERO_PADDED_PAIRS
                .as_ptr()
                .add((n as usize) * 2 + is_single_digit as usize),
            2 - is_single_digit as usize,
        )
    }
}

/// Obtain a string of two ASCII digits representing `n`. This includes a leading zero if `n` is
/// less than 10.
#[inline]
pub(crate) const fn two_digits_zero_padded(n: ru8<0, 99>) -> &'static str {
    // Safety: We're staying within the bounds of the array. The array contains only ASCII
    // characters, so it's valid UTF-8.
    unsafe { str_from_raw_parts(ZERO_PADDED_PAIRS.as_ptr().add((n.get() as usize) * 2), 2) }
}

/// Obtain a string of two ASCII digits representing `n`. This includes a leading space if `n` is
/// less than 10.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) const fn two_digits_space_padded(n: ru8<0, 99>) -> &'static str {
    // Safety: We're staying within the bounds of the array. The array contains only ASCII
    // characters, so it's valid UTF-8.
    unsafe { str_from_raw_parts(SPACE_PADDED_PAIRS.as_ptr().add((n.get() as usize) * 2), 2) }
}

/// Obtain two strings of ASCII digits representing `n`. The first string is most significant. No
/// leading zeros or spaces are included.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) fn one_to_three_digits_no_padding(n: ru16<0, 999>) -> [&'static str; 2] {
    if let Some(n) = n.narrow::<0, 99>() {
        crate::hint::cold_path();
        ["", one_to_two_digits_no_padding(n.into())]
    } else {
        three_digits_zero_padded(n)
    }
}

/// Obtain two strings of ASCII digits representing `n`. The first string is the most significant.
/// Leading zeros are included if the number has fewer than 3 digits.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) const fn three_digits_zero_padded(n: ru16<0, 999>) -> [&'static str; 2] {
    let [high, low] = div_100(n.expand());
    [
        // Safety: `high` is guaranteed to be less than 10 due to the range of the input.
        single_digit(unsafe { high.narrow_unchecked() }),
        two_digits_zero_padded(low),
    ]
}

/// Obtain two strings of ASCII digits representing `n`. The first string is the most significant.
/// Leading spaces are included if the number has fewer than 3 digits.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) const fn three_digits_space_padded(n: ru16<0, 999>) -> [&'static str; 2] {
    let [high, low] = div_100(n.expand());

    if let Some(high) = high.narrow::<1, 9>() {
        [single_digit(high.expand()), two_digits_zero_padded(low)]
    } else {
        [" ", two_digits_space_padded(low)]
    }
}

/// Obtain two strings of ASCII digits representing `n`. The first string is the most significant.
/// No leading zeros or spaces are included.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) fn one_to_four_digits_no_padding(n: ru16<0, 9_999>) -> [&'static str; 2] {
    if let Some(n) = n.narrow::<0, 999>() {
        crate::hint::cold_path();
        one_to_three_digits_no_padding(n)
    } else {
        four_digits_zero_padded(n)
    }
}

/// Obtain two strings of two ASCII digits each representing `n`. The first string is the most
/// significant. Leading zeros are included if the number has fewer than 4 digits.
#[inline]
pub(crate) const fn four_digits_zero_padded(n: ru16<0, 9_999>) -> [&'static str; 2] {
    let [high, low] = div_100(n);
    [two_digits_zero_padded(high), two_digits_zero_padded(low)]
}

/// Obtain two strings of two ASCII digits each representing `n`. The first string is the most
/// significant. Leading spaces are included if the number has fewer than 4 digits.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) const fn four_digits_space_padded(n: ru16<0, 9_999>) -> [&'static str; 2] {
    let [high, low] = div_100(n);

    if high.get() == 0 {
        ["  ", two_digits_space_padded(low)]
    } else {
        [two_digits_space_padded(high), two_digits_zero_padded(low)]
    }
}

/// Obtain three strings which together represent `n`. The first string is the most significant.
/// Leading zeros are included if the number has fewer than 4 digits. The first string will be empty
/// if `n` is less than 10,000.
#[inline]
pub(crate) const fn four_to_six_digits(n: ru32<0, 999_999>) -> [&'static str; 3] {
    let n = n.get();

    let (first_two, remaining) = (n / 10_000, n % 10_000);

    let size = 2 - (first_two < 10) as usize - (first_two == 0) as usize;
    let offset = first_two as usize * 2 + 2 - size;

    // Safety: `offset` is within the bounds of the array. The array contains only ASCII characters,
    // so it's valid UTF-8.
    let first_two = unsafe { str_from_raw_parts(ZERO_PADDED_PAIRS.as_ptr().add(offset), size) };
    // Safety: `remaining` is guaranteed to be less than 10,000 due to the modulus above.
    let [second_two, last_two] =
        four_digits_zero_padded(unsafe { ru16::new_unchecked(remaining as u16) });
    [first_two, second_two, last_two]
}

/// Obtain three strings which together represent `n`. The first string is the most significant.
/// Leading zeros are included if the number has fewer than 5 digits. The first string will be empty
/// if `n` is less than 10,000.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) const fn five_digits_zero_padded(n: ru32<0, 99_999>) -> [&'static str; 3] {
    let n = n.get();

    let (first_one, remaining) = (n / 10_000, n % 10_000);

    // Safety: `first_one` is guaranteed to be less than 10 due to the division above.
    let first_one = single_digit(unsafe { ru8::new_unchecked(first_one as u8) });
    // Safety: `remaining` is guaranteed to be less than 10,000 due to the modulus above.
    let [second_two, last_two] =
        four_digits_zero_padded(unsafe { ru16::new_unchecked(remaining as u16) });
    [first_one, second_two, last_two]
}

/// Obtain three strings which together represent `n`. The first string is the most significant.
/// Leading zeroes are included if the number has fewer than 6 digits.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) const fn six_digits_zero_padded(n: ru32<0, 999_999>) -> [&'static str; 3] {
    let n = n.get();

    let (first_two, remaining) = (n / 10_000, n % 10_000);

    // Safety: `first_two` is guaranteed to be less than 100 due to the division above.
    let first_two = two_digits_zero_padded(unsafe { ru8::new_unchecked(first_two as u8) });
    // Safety: `remaining` is guaranteed to be less than 10,000 due to the modulus above.
    let [second_two, last_two] =
        four_digits_zero_padded(unsafe { ru16::new_unchecked(remaining as u16) });
    [first_two, second_two, last_two]
}

/// Obtain five strings which together represent `n`, which is a number of nanoseconds.
///
/// This value is intended to be used after a decimal point to represent a fractional second. The
/// first string will always contain exactly one digit; the remaining four will contain two digits
/// each.
#[inline]
pub(crate) const fn subsecond_from_nanos(n: ru32<0, 999_999_999>) -> [&'static str; 5] {
    let n = n.get();
    let (digits_1_thru_5, digits_6_thru_9) = (n / 10_000, n % 10_000);
    let (digit_1, digits_2_thru_5) = (digits_1_thru_5 / 10_000, digits_1_thru_5 % 10_000);

    // Safety: The type of `n` ensures that `n` is less than 1,000,000,000. Combined with the
    // arithmetic above, this guarantees that all values are in the required ranges.
    unsafe {
        let digit_1 = single_digit(ru8::new_unchecked(digit_1 as u8));
        let [digits_2_and_3, digits_4_and_5] =
            four_digits_zero_padded(ru16::new_unchecked(digits_2_thru_5 as u16));
        let [digits_6_and_7, digits_8_and_9] =
            four_digits_zero_padded(ru16::new_unchecked(digits_6_thru_9 as u16));

        [
            digit_1,
            digits_2_and_3,
            digits_4_and_5,
            digits_6_and_7,
            digits_8_and_9,
        ]
    }
}

/// Obtain a string of 1 to 9 ASCII digits representing `n`, which is a number of nanoseconds.
///
/// This value is intended to be used after a decimal point to represent a fractional second.
/// Trailing zeros are truncated, but at least one digit is always present.
#[inline]
pub(crate) const fn truncated_subsecond_from_nanos(n: ru32<0, 999_999_999>) -> StackStr<9> {
    #[repr(C, align(8))]
    #[derive(Clone, Copy)]
    struct Digits {
        _padding: MaybeUninit<[u8; 7]>,
        digit_1: u8,
        digits_2_thru_9: [u8; 8],
    }

    let [
        digit_1,
        digits_2_and_3,
        digits_4_and_5,
        digits_6_and_7,
        digits_8_and_9,
    ] = subsecond_from_nanos(n);

    // Ensure that digits 2 thru 9 are stored as a single array that is 8-aligned. This allows the
    // conversion to a `u64` to be zero cost, resulting in a nontrivial performance improvement.
    let buf = Digits {
        _padding: MaybeUninit::uninit(),
        digit_1: digit_1.as_bytes()[0],
        digits_2_thru_9: [
            digits_2_and_3.as_bytes()[0],
            digits_2_and_3.as_bytes()[1],
            digits_4_and_5.as_bytes()[0],
            digits_4_and_5.as_bytes()[1],
            digits_6_and_7.as_bytes()[0],
            digits_6_and_7.as_bytes()[1],
            digits_8_and_9.as_bytes()[0],
            digits_8_and_9.as_bytes()[1],
        ],
    };

    // By converting the bytes into a single integer, we can effectively perform an equality check
    // against b'0' for all bytes at once. This is actually faster than using portable SIMD (even
    // with `-Ctarget-cpu=native`).
    let bitmask = u64::from_le_bytes(buf.digits_2_thru_9) ^ u64::from_le_bytes([b'0'; 8]);
    let digits_to_truncate = bitmask.leading_zeros() / 8;
    let len = 9 - digits_to_truncate as usize;

    // Safety: All bytes are initialized and valid UTF-8, and `len` represents the number of bytes
    // we wish to display (that is between 1 and 9 inclusive). `Digits` is `#[repr(C)]`, so the
    // layout is guaranteed.
    unsafe {
        StackStr::new(
            *(&raw const buf)
                .byte_add(core::mem::offset_of!(Digits, digit_1))
                .cast(),
            len,
        )
    }
}

/// Format a `u64` into a string with no padding.
#[inline]
pub(crate) const fn u64_pad_none(value: u64) -> StackTrailingStr<20> {
    let mut bytes = [MaybeUninit::uninit(); 20];

    let mut offset = 20;
    let mut remain = value;

    while remain > 999 {
        offset -= 4;
        let quad = remain % 1_00_00;
        remain /= 1_00_00;
        // Safety: `quad` is guaranteed to be less than 10,000 due to the modulus above.
        let [pair1, pair2] = div_100(unsafe { ru16::new_unchecked(quad as u16) });
        // Safety: `buf` is at least `offset + 4` bytes long.
        unsafe {
            write_two_digits(&mut bytes, offset, pair1);
            write_two_digits(&mut bytes, offset + 2, pair2);
        }
    }

    if remain > 9 {
        offset -= 2;

        // Safety: `remain` is guaranteed to be less than 10,000 due to the loop above.
        let [last, pair] = div_100(unsafe { ru16::new_unchecked(remain as u16) });
        remain = last.get() as u64;
        // Safety: `buf` is at least `offset + 2` bytes long.
        unsafe { write_two_digits(&mut bytes, offset, pair) };
    }

    if remain != 0 || value == 0 {
        offset -= 1;

        let last = remain as u8 & 15;
        // Safety: `offset` is known to be in bounds, and the value is known to be less than 10 due
        // to the conditionals and bitwise AND above.
        unsafe { write_one_digit(&mut bytes, offset, ru8::new_unchecked(last)) };
    }

    // Safety: All bytes starting at `offset` are initialized and valid UTF-8.
    unsafe { StackTrailingStr::new(bytes, offset) }
}

/// Format a `u128` into a string with no padding.
#[inline]
#[cfg(feature = "formatting")]
pub(crate) const fn u128_pad_none(value: u128) -> StackTrailingStr<39> {
    let mut bytes = [MaybeUninit::uninit(); 39];

    // Take the 16 least-significant decimals.
    let (quot_1e16, mod_1e16) = div_rem_1e16(value);
    let (mut remain, mut offset) = if quot_1e16 == 0 {
        (mod_1e16.get(), 39)
    } else {
        // Write digits at buf[23..39].
        // Safety: `bytes` is 39 bytes long, so writing at offset 23 for 16 bytes is sound.
        unsafe { enc_16lsd::<23>(&mut bytes, mod_1e16) };

        // Take another 16 decimals.
        let (quot2, mod2) = div_rem_1e16(quot_1e16);
        if quot2 == 0 {
            (mod2.get(), 23)
        } else {
            // Write digits at buf[7..23].
            // Safety: `bytes` is 39 bytes long, so writing at offset 7 for 16 bytes is sound.
            unsafe { enc_16lsd::<7>(&mut bytes, mod2) };
            // Safety: `quot2`` has at most 7 decimals remaining after two 1e16 divisions.
            (quot2 as u64, 7)
        }
    };

    // Format per four digits from the lookup table.
    while remain > 999 {
        offset -= 4;

        // pull two pairs
        let quad = remain % 1_00_00;
        remain /= 1_00_00;
        // Safety: `quad` is guaranteed to be less than 10,000 due to the modulus above.
        let [pair1, pair2] = div_100(unsafe { ru16::new_unchecked(quad as u16) });
        // Safety: `buf` is at least `offset + 4` bytes long.
        unsafe {
            write_two_digits(&mut bytes, offset, pair1);
            write_two_digits(&mut bytes, offset + 2, pair2);
        }
    }

    // Format per two digits from the lookup table.
    if remain > 9 {
        offset -= 2;

        // Safety: `remain` is guaranteed to be less than 10,000 due to the loop above.
        let [last, pair] = div_100(unsafe { ru16::new_unchecked(remain as u16) });
        remain = last.get() as u64;
        // Safety: `buf` is at least `offset + 2` bytes long.
        unsafe { write_two_digits(&mut bytes, offset, pair) };
    }

    // Format the last remaining digit, if any.
    if remain != 0 || value == 0 {
        offset -= 1;

        // Either the compiler sees that remain < 10, or it prevents a boundary check up next.
        let last = remain as u8 & 15;
        // Safety: `offset` is known to be in bounds, and the value is known to be less than 10 due
        // to the conditionals and bitwise AND above.
        unsafe { write_one_digit(&mut bytes, offset, ru8::new_unchecked(last)) };
    }

    // Safety: All bytes starting at `offset` are initialized and valid UTF-8.
    unsafe { StackTrailingStr::new(bytes, offset) }
}

/// Encodes the 16 least-significant decimals of n into `buf[OFFSET..OFFSET + 16]`.
///
/// # Safety
///
/// - `buf` must be at least `OFFSET + 16` bytes long.
#[cfg(feature = "formatting")]
const unsafe fn enc_16lsd<const OFFSET: usize>(
    buf: &mut [MaybeUninit<u8>],
    n: ru64<0, 9999_9999_9999_9999>,
) {
    // Consume the least-significant decimals from a working copy.
    let mut remain = n.get();

    let mut quad_index = 3;
    while quad_index >= 1 {
        let quad = remain % 1_00_00;
        remain /= 1_00_00;
        // Safety: `quad` is guaranteed to be less than 10,000 due to the modulus above.
        let [pair1, pair2] = div_100(unsafe { ru16::new_unchecked(quad as u16) });
        // Safety: `buf` is at least `quad_index * 4 + OFFSET + 4` bytes long.
        unsafe {
            write_two_digits(buf, quad_index * 4 + OFFSET, pair1);
            write_two_digits(buf, quad_index * 4 + OFFSET + 2, pair2);
        }
        quad_index -= 1;
    }

    // Safety: `remain` is guaranteed to be less than 10,000 due the range of `n` and the arithmetic
    // in the loop above.
    let [pair1, pair2] = div_100(unsafe { ru16::new_unchecked(remain as u16) });
    // Safety: `buf` is at least `OFFSET + 4` bytes long.
    unsafe {
        write_two_digits(buf, OFFSET, pair1);
        write_two_digits(buf, OFFSET + 2, pair2);
    }
}

// Euclidean division plus remainder with constant 1e16 basically consumes 16
// decimals from n.
#[cfg(feature = "formatting")]
const fn div_rem_1e16(n: u128) -> (u128, ru64<0, 9999_9999_9999_9999>) {
    const D: u128 = 1_0000_0000_0000_0000;
    if n < D {
        // Safety: We just checked that `n` is in range.
        return (0, unsafe { ru64::new_unchecked(n as u64) });
    }

    const M_HIGH: u128 = 76_624_777_043_294_442_917_917_351_357_515_459_181;
    const SH_POST: u8 = 51;

    // n.widening_mul(M_HIGH).1 >> SH_POST
    let quot = mulhi(n, M_HIGH) >> SH_POST;
    let rem = n - quot * D;
    // Safety: The arithmetic above ensures that `rem` is in range.
    (quot, unsafe { ru64::new_unchecked(rem as u64) })
}

/// Multiply unsigned 128 bit integers, return upper 128 bits of the result
#[inline]
#[cfg(feature = "formatting")]
const fn mulhi(x: u128, y: u128) -> u128 {
    let x_lo = x as u64;
    let x_hi = (x >> 64) as u64;
    let y_lo = y as u64;
    let y_hi = (y >> 64) as u64;

    // handle possibility of overflow
    let carry = (x_lo as u128 * y_lo as u128) >> 64;
    let m = x_lo as u128 * y_hi as u128 + carry;
    let high1 = m >> 64;

    let m_lo = m as u64;
    let high2 = (x_hi as u128 * y_lo as u128 + m_lo as u128) >> 64;

    x_hi as u128 * y_hi as u128 + high1 + high2
}
