use core::mem;

use super::fallback;

#[must_use]
#[inline]
pub fn contains_null_or_utf8_4_byte_char_header(value: &[u8]) -> bool {
    test_word_any(
        value,
        fallback::contains_null_or_utf8_4_byte_char_header,
        word_contains_null_or_utf8_4_byte_char_header,
    )
}

#[must_use]
#[inline]
fn word_contains_null_or_utf8_4_byte_char_header(word: usize) -> bool {
    word_contains_null_byte(word) || word_contains_utf8_4_byte_char_header(word)
}

#[must_use]
#[inline]
pub fn contains_utf8_4_byte_char_header(value: &[u8]) -> bool {
    test_word_any(
        value,
        fallback::contains_utf8_4_byte_char_header,
        word_contains_utf8_4_byte_char_header,
    )
}

#[must_use]
#[inline]
fn word_contains_utf8_4_byte_char_header(word: usize) -> bool {
    const MASK: usize = usize_repeat_u8(0b1111_1000);
    const HEADER: usize = usize_repeat_u8(0b1111_0000);

    word_contains_null_byte((word & MASK) ^ HEADER)
}

#[must_use]
#[inline]
fn word_contains_null_byte(word: usize) -> bool {
    const LOW_MASK: usize = usize_repeat_u8(0x01);
    const HIGH_MASK: usize = usize_repeat_u8(0x80);

    (word.wrapping_sub(LOW_MASK) & !word & HIGH_MASK) != 0
}

#[must_use]
#[inline]
const fn usize_repeat_u8(byte: u8) -> usize {
    usize::from_ne_bytes([byte; USIZE_SIZE])
}

/// This is an optimized test that will use usize-at-a-time operations instead
/// of byte-at-a-time operations (when possible). It is imperative to understand
/// that some bytes might be read multiple times, so this function should only
/// be used to test for the presence of a specific byte.
///
/// `fallback` is a function that will be called if the slice is too small for
/// usize-at-a-time operations or if the platform wouldn't benefit from it.
///
/// `test` is a function that will be called with the usize value read from the
/// slice. If it returns `true`, the function will return `true` immediately.
///
/// The algorithm works as follows:
///
/// - Read the first word from the slice as an unaligned value.
/// - Align the pointer, read subsequent words until the last aligned word.
/// - Read the last word from the slice as an unaligned value.
///
/// If any of the words satisfy the test, the function will immediately return
/// `true`. Otherwise, it will return `false`.
#[must_use]
#[inline]
fn test_word_any<F, T>(value: &[u8], fallback: F, test: T) -> bool
where
    F: Fn(&[u8]) -> bool,
    T: Fn(usize) -> bool,
{
    let len = value.len();
    let start_ptr = value.as_ptr();

    let align_offset = start_ptr.align_offset(USIZE_SIZE);

    if len < USIZE_SIZE || len < align_offset || USIZE_SIZE < mem::align_of::<usize>() {
        return fallback(value);
    }

    let offset_to_aligned = if align_offset == 0 {
        USIZE_SIZE
    } else {
        align_offset
    };

    // SAFETY: We have already checked that `len` is less than `USIZE_SIZE`
    // above.
    let first_word = unsafe { start_ptr.cast::<usize>().read_unaligned() };

    if test(first_word) {
        return true;
    }

    #[allow(clippy::cast_ptr_alignment)]
    // SAFETY: word_ptr is the (properly aligned) usize ptr we use to read the
    // middle chunk of the slice.
    let mut word_ptr = unsafe { start_ptr.add(offset_to_aligned).cast::<usize>() };

    // NOTE: `byte_position` is the byte index of `word_ptr`, used for loop end
    // checks.
    let mut byte_position = offset_to_aligned;

    while byte_position < len - USIZE_SIZE {
        // SAFETY: We know `word_ptr` is properly aligned from the previous
        // offset, and said pointer points to a valid usize because of the loop
        // condition.
        let word = unsafe { word_ptr.read() };

        if test(word) {
            return true;
        }

        byte_position += USIZE_SIZE;

        // SAFETY: We know that `byte_position` is less than or equal to
        // `len - USIZE_SIZE`. At most, `word_ptr` will be one past the end
        // at most (in which it won't be used because this will be the last
        // iteration).
        word_ptr = unsafe { word_ptr.add(1) };
    }

    #[allow(clippy::cast_ptr_alignment)]
    // SAFETY: We know that `len` is greater than or equal to `USIZE_SIZE`, so
    // this should take us to USIZE_SIZE bytes before the end of the slice.
    let last_word_ptr = unsafe { start_ptr.add(len - USIZE_SIZE).cast::<usize>() };
    // SAFETY: We know there's exactly one usize left in the slice.
    let last_word = unsafe { last_word_ptr.read_unaligned() };

    test(last_word)
}

const USIZE_SIZE: usize = mem::size_of::<usize>();
