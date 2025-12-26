use core::mem;

const _: () = {
    let static_data =
        mem::size_of_val(&crate::POW10_SIGNIFICANDS) + mem::size_of_val(&crate::DIGITS2);
    assert!(static_data == 10072); // 9.8K
};

#[cfg(target_endian = "little")]
#[test]
fn utilities() {
    let countl_zero = u64::leading_zeros;
    assert_eq!(countl_zero(0), 64);
    assert_eq!(countl_zero(1), 63);
    assert_eq!(countl_zero(!0), 0);

    assert_eq!(crate::count_trailing_nonzeros(0x00000000_00000000), 0);
    assert_eq!(crate::count_trailing_nonzeros(0x00000000_00000001), 1);
    assert_eq!(crate::count_trailing_nonzeros(0x00000000_00000009), 1);
    assert_eq!(crate::count_trailing_nonzeros(0x00090000_09000000), 7);
    assert_eq!(crate::count_trailing_nonzeros(0x01000000_00000000), 8);
    assert_eq!(crate::count_trailing_nonzeros(0x09000000_00000000), 8);
}

#[test]
fn umul192_upper64_inexact_to_odd() {
    let (hi, lo) = crate::POW10_SIGNIFICANDS[0];
    assert_eq!(
        crate::umul192_upper64_inexact_to_odd(hi, lo, 0x1234567890abcdef << 1),
        0x24554a3ce60a45f5,
    );
    assert_eq!(
        crate::umul192_upper64_inexact_to_odd(hi, lo, 0x1234567890abce16 << 1),
        0x24554a3ce60a4643,
    );
}
