//! Implementation of hash-to-field for Scalar values

use super::HashToField;
use crate::generic_array::{typenum::U48, GenericArray};
use crate::scalar::Scalar;

impl HashToField for Scalar {
    // ceil(log2(p)) = 255, m = 1, k = 128.
    type InputLength = U48;

    fn from_okm(okm: &GenericArray<u8, U48>) -> Scalar {
        let mut bs = [0u8; 64];
        bs[16..].copy_from_slice(&okm);
        bs.reverse(); // into little endian
        Scalar::from_bytes_wide(&bs)
    }
}

#[test]
fn test_hash_to_scalar() {
    let tests: &[(&[u8], &str)] = &[
        (
            &[0u8; 48],
            "0x0000000000000000000000000000000000000000000000000000000000000000",
        ),
        (
            b"aaaaaabbbbbbccccccddddddeeeeeeffffffgggggghhhhhh",
            "0x2228450bf55d8fe62395161bd3677ff6fc28e45b89bc87e02a818eda11a8c5da",
        ),
        (
            b"111111222222333333444444555555666666777777888888",
            "0x4aa543cbd2f0c8f37f8a375ce2e383eb343e7e3405f61e438b0a15fb8899d1ae",
        ),
    ];
    for (input, expected) in tests {
        let output = format!("{:?}", Scalar::from_okm(GenericArray::from_slice(input)));
        assert_eq!(&output, expected);
    }
}
