// BIP-350 test vectors.

#![cfg(feature = "alloc")]

use bech32::primitives::decode::{
    CheckedHrpstring, CheckedHrpstringError, ChecksumError, SegwitHrpstring, SegwitHrpstringError,
    UncheckedHrpstring,
};
use bech32::{Bech32, Bech32m};

// This is a separate test because we correctly identify this string as invalid but not for the
// reason given in the bip.
#[test]
fn bip_350_checksum_calculated_with_uppercase_form() {
    // BIP-350 states reason for error should be: "checksum calculated with uppercase form of HRP".
    let s = "M1VUXWEZ";

    assert_eq!(
        CheckedHrpstring::new::<Bech32m>(s).unwrap_err(),
        CheckedHrpstringError::Checksum(ChecksumError::InvalidResidue)
    );

    assert_eq!(
        SegwitHrpstring::new(s).unwrap_err(),
        SegwitHrpstringError::Checksum(ChecksumError::InvalidResidue)
    );
}

macro_rules! check_valid_bech32m {
    ($($test_name:ident, $valid_bech32m:literal);* $(;)?) => {
        $(
            #[test]
            fn $test_name() {
                let p = UncheckedHrpstring::new($valid_bech32m).unwrap();
                p.validate_checksum::<Bech32m>().expect("valid bech32m");
                // Valid bech32m strings are by definition invalid bech32.
                assert_eq!(p.validate_checksum::<Bech32>().unwrap_err(), ChecksumError::InvalidResidue);
            }
        )*
    }
}
check_valid_bech32m! {
    valid_bech32m_hrp_string_0, "A1LQFN3A";
    valid_bech32m_hrp_string_1, "a1lqfn3a";
    valid_bech32m_hrp_string_2, "an83characterlonghumanreadablepartthatcontainsthetheexcludedcharactersbioandnumber11sg7hg6";
    valid_bech32m_hrp_string_3, "abcdef1l7aum6echk45nj3s0wdvt2fg8x9yrzpqzd3ryx";
    valid_bech32m_hrp_string_4, "11llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllludsr8";
    valid_bech32m_hrp_string_5, "split1checkupstagehandshakeupstreamerranterredcaperredlc445v";
    valid_bech32m_hrp_string_6, "?1v759aa";
}

macro_rules! check_valid_address_roundtrip {
    ($($test_name:ident, $addr:literal);* $(;)?) => {
        $(
            #[test]
            #[cfg(feature = "alloc")]
            fn $test_name() {
                let (hrp, version, program) = bech32::segwit::decode($addr).expect("failed to decode valid address");
                let encoded = bech32::segwit::encode(hrp, version, &program).expect("failed to encode address");

                // The bips specifically say that encoder should output lowercase characters so we uppercase manually.
                if encoded != $addr {
                    let got = encoded.to_uppercase();
                    assert_eq!(got, $addr)
                }
            }
        )*
    }
}
// Note these test vectors include various witness versions.
check_valid_address_roundtrip! {
    bip_350_valid_address_roundtrip_0, "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
    bip_350_valid_address_roundtrip_1, "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sl5k7";
    bip_350_valid_address_roundtrip_2, "bc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7kt5nd6y";
    bip_350_valid_address_roundtrip_3, "BC1SW50QGDZ25J";
    bip_350_valid_address_roundtrip_4, "bc1zw508d6qejxtdg4y5r3zarvaryvaxxpcs";
    bip_350_valid_address_roundtrip_5, "tb1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesrxh6hy";
    bip_350_valid_address_roundtrip_6, "tb1pqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesf3hn0c";
    bip_350_valid_address_roundtrip_7, "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0";
}

macro_rules! check_invalid_address {
    ($($test_name:ident, $addr:literal);* $(;)?) => {
        $(
            #[test]
            #[cfg(feature = "alloc")]
            fn $test_name() {
                match SegwitHrpstring::new($addr) {
                    Err(_) => {},
                    // We do not enforce the bip specified restrictions when constructing
                    // SegwitHrpstring so must explicitly do check.
                    Ok(segwit) => assert!(!segwit.has_valid_hrp()),
                }
            }
        )*
    }
}
check_invalid_address! {
    // Invalid human-readable part
    bip_350_invalid_address_0, "tc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vq5zuyut";
    // Invalid checksums (Bech32 instead of Bech32m):
    bip_350_invalid_address_1, "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqh2y7hd";
    bip_350_invalid_address_2, "tb1z0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqglt7rf";
    bip_350_invalid_address_3, "BC1S0XLXVLHEMJA6C4DQV22UAPCTQUPFHLXM9H8Z3K2E72Q4K9HCZ7VQ54WELL";
    bip_350_invalid_address_4, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kemeawh";
    bip_350_invalid_address_5, "tb1q0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vq24jc47";
    // Invalid character in checksum
    bip_350_invalid_address_6, "bc1p38j9r5y49hruaue7wxjce0updqjuyyx0kh56v8s25huc6995vvpql3jow4";
    // Invalid witness version
    bip_350_invalid_address_7, "BC130XLXVLHEMJA6C4DQV22UAPCTQUPFHLXM9H8Z3K2E72Q4K9HCZ7VQ7ZWS8R";
    // Invalid program length (1 byte)
    bip_350_invalid_address_8, "bc1pw5dgrnzv";
    // Invalid program length (41 bytes)
    bip_350_invalid_address_9, "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7v8n0nx0muaewav253zgeav";
    // Invalid program length for witness version 0 (per BIP-141)
    bip_350_invalid_address_10, "BC1QR508D6QEJXTDG4Y5R3ZARVARYV98GJ9P";
    // Mixed case
    bip_350_invalid_address_11, "tb1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vq47Zagq";
    // zero padding of more than 4 bits
    bip_350_invalid_address_12, "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7v07qwwzcrf";
    // Non-zero padding in 8-to-5 conversion
    bip_350_invalid_address_13, "tb1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vpggkg4j";
    // Empty data section
    bip_350_invalid_address_14, "bc1gmk9yu";
}
