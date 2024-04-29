use aes::Aes256;
use num_integer::Integer;
use proptest::prelude::*;

use super::{BinaryNumeralString, FlexibleNumeralString, NumeralStringError, Radix, FF1};

prop_compose! {
    fn valid_radix()(radix in 2u32..=(1 << 16)) -> (u32, u16, usize) {
        let max_numeral = (radix - 1) as u16;
        let min_len = match Radix::from_u32(radix)
            .unwrap()
            .check_ns_length(0)
            .unwrap_err()
        {
            NumeralStringError::TooShort { min_len, .. } => min_len,
            _ => unreachable!(),
        };
        (radix, max_numeral, min_len)
    }
}

prop_compose! {
    fn flexible_ns()(
        (radix, max_numeral, min_len) in valid_radix(),
    )(
        radix in Just(radix),
        ns in prop::collection::vec(0..=max_numeral, min_len..100),
    ) -> (u32, FlexibleNumeralString) {
        (radix, ns.into())
    }
}

prop_compose! {
    fn binary_ns()(
        // This enables even-length and odd-length numeral strings to be independently
        // shrunk, taking advantage of knowledge about how `BinaryNumeralString` is
        // implemented.
        odd_len in prop::bool::ANY,
        ns in prop::collection::vec(prop::num::u8::ANY, 4..100),
    ) -> BinaryNumeralString {
        let mut ns = &ns[..];

        if ns.len().is_odd() != odd_len {
            // `proptest` shrinks `vec` from the front, so pruning the front element makes
            // failures easier to interpret.
            ns = &ns[1..];
        }

        BinaryNumeralString::from_bytes_le(ns)
    }
}

proptest! {
    #[test]
    fn doesnt_crash(
        key in prop::array::uniform32(prop::num::u8::ANY),
        (radix, _, _) in valid_radix(),
    ) {
        assert!(matches!(FF1::<Aes256>::new(&key, radix), Ok(_)));
    }

    #[test]
    fn flexible_round_trip(
        (radix, ns) in flexible_ns(),
        tweak in prop::collection::vec(prop::num::u8::ANY, 0..100),
    ) {
        let key = [0; 32];
        let ff = FF1::<Aes256>::new(&key, radix).unwrap();
        let ct = ff.encrypt(&tweak, &ns).unwrap();
        let pt = ff.decrypt(&tweak, &ct).unwrap();
        assert_eq!(Vec::from(pt), Vec::from(ns));
    }

    #[test]
    fn binary_round_trip(
        ns in binary_ns(),
        tweak in prop::collection::vec(prop::num::u8::ANY, 0..100),
    ) {
        let key = [0; 32];
        let ff = FF1::<Aes256>::new(&key, 2).unwrap();
        let ct = ff.encrypt(&tweak, &ns).unwrap();
        let pt = ff.decrypt(&tweak, &ct).unwrap();
        assert_eq!(pt.to_bytes_le(), ns.to_bytes_le());
    }
}
