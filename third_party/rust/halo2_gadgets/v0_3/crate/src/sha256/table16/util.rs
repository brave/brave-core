use halo2_proofs::circuit::Value;

pub const MASK_EVEN_32: u32 = 0x55555555;

/// The sequence of bits representing a u64 in little-endian order.
///
/// # Panics
///
/// Panics if the expected length of the sequence `NUM_BITS` exceeds
/// 64.
pub fn i2lebsp<const NUM_BITS: usize>(int: u64) -> [bool; NUM_BITS] {
    /// Takes in an FnMut closure and returns a constant-length array with elements of
    /// type `Output`.
    fn gen_const_array<Output: Copy + Default, const LEN: usize>(
        closure: impl FnMut(usize) -> Output,
    ) -> [Output; LEN] {
        gen_const_array_with_default(Default::default(), closure)
    }

    fn gen_const_array_with_default<Output: Copy, const LEN: usize>(
        default_value: Output,
        closure: impl FnMut(usize) -> Output,
    ) -> [Output; LEN] {
        let mut ret: [Output; LEN] = [default_value; LEN];
        for (bit, val) in ret.iter_mut().zip((0..LEN).map(closure)) {
            *bit = val;
        }
        ret
    }

    assert!(NUM_BITS <= 64);
    gen_const_array(|mask: usize| (int & (1 << mask)) != 0)
}

/// Returns the integer representation of a little-endian bit-array.
/// Panics if the number of bits exceeds 64.
pub fn lebs2ip<const K: usize>(bits: &[bool; K]) -> u64 {
    assert!(K <= 64);
    bits.iter()
        .enumerate()
        .fold(0u64, |acc, (i, b)| acc + if *b { 1 << i } else { 0 })
}

/// Helper function that interleaves a little-endian bit-array with zeros
/// in the odd indices. That is, it takes the array
///         [b_0, b_1, ..., b_n]
/// to
///         [b_0, 0, b_1, 0, ..., b_n, 0].
/// Panics if bit-array is longer than 16 bits.
pub fn spread_bits<const DENSE: usize, const SPREAD: usize>(
    bits: impl Into<[bool; DENSE]>,
) -> [bool; SPREAD] {
    assert_eq!(DENSE * 2, SPREAD);
    assert!(DENSE <= 16);

    let bits: [bool; DENSE] = bits.into();
    let mut spread = [false; SPREAD];

    for (idx, bit) in bits.iter().enumerate() {
        spread[idx * 2] = *bit;
    }

    spread
}

/// Negates the even bits in a spread bit-array.
pub fn negate_spread<const LEN: usize>(arr: [bool; LEN]) -> [bool; LEN] {
    assert_eq!(LEN % 2, 0);

    let mut neg = arr;
    for even_idx in (0..LEN).step_by(2) {
        let odd_idx = even_idx + 1;
        assert!(!arr[odd_idx]);

        neg[even_idx] = !arr[even_idx];
    }

    neg
}

/// Returns even bits in a bit-array
pub fn even_bits<const LEN: usize, const HALF: usize>(bits: [bool; LEN]) -> [bool; HALF] {
    assert_eq!(LEN % 2, 0);
    let mut even_bits = [false; HALF];
    for idx in 0..HALF {
        even_bits[idx] = bits[idx * 2]
    }
    even_bits
}

/// Returns odd bits in a bit-array
pub fn odd_bits<const LEN: usize, const HALF: usize>(bits: [bool; LEN]) -> [bool; HALF] {
    assert_eq!(LEN % 2, 0);
    let mut odd_bits = [false; HALF];
    for idx in 0..HALF {
        odd_bits[idx] = bits[idx * 2 + 1]
    }
    odd_bits
}

/// Given a vector of words as vec![(lo: u16, hi: u16)], returns their sum: u32, along
/// with a carry bit.
pub fn sum_with_carry(words: Vec<(Value<u16>, Value<u16>)>) -> (Value<u32>, Value<u64>) {
    let words_lo: Value<Vec<u64>> = words.iter().map(|(lo, _)| lo.map(|lo| lo as u64)).collect();
    let words_hi: Value<Vec<u64>> = words.iter().map(|(_, hi)| hi.map(|hi| hi as u64)).collect();

    let sum: Value<u64> = {
        let sum_lo: Value<u64> = words_lo.map(|vec| vec.iter().sum());
        let sum_hi: Value<u64> = words_hi.map(|vec| vec.iter().sum());
        sum_lo.zip(sum_hi).map(|(lo, hi)| lo + (1 << 16) * hi)
    };

    let carry = sum.map(|sum| sum >> 32);
    let sum = sum.map(|sum| sum as u32);

    (sum, carry)
}
