// BIP-173 test vectors.

#![cfg(feature = "alloc")]

use bech32::primitives::decode::{
    CheckedHrpstring, ChecksumError, SegwitHrpstring, UncheckedHrpstring,
};
use bech32::{Bech32, Bech32m, ByteIterExt, Fe32IterExt};

// This is a separate test because we correctly identify this string as invalid but not for the
// reason given in the bip.
#[test]
fn bip_173_checksum_calculated_with_uppercase_form() {
    use bech32::primitives::decode::{CheckedHrpstringError, ChecksumError, SegwitHrpstringError};

    // BIP-173 states reason for error should be: "checksum calculated with uppercase form of HRP".
    let s = "A1G7SGD8";

    assert_eq!(
        CheckedHrpstring::new::<Bech32>(s).unwrap_err(),
        CheckedHrpstringError::Checksum(ChecksumError::InvalidResidue)
    );

    assert_eq!(
        SegwitHrpstring::new(s).unwrap_err(),
        SegwitHrpstringError::Checksum(ChecksumError::InvalidResidue)
    );
}

macro_rules! check_valid_bech32 {
    ($($test_name:ident, $valid_bech32:literal);* $(;)?) => {
        $(
            #[test]
            fn $test_name() {
                let p = UncheckedHrpstring::new($valid_bech32).unwrap();
                p.validate_checksum::<Bech32>().expect("valid bech32");
                // Valid bech32 strings are by definition invalid bech32m.
                assert_eq!(p.validate_checksum::<Bech32m>().unwrap_err(), ChecksumError::InvalidResidue);
            }
        )*
    }
}
check_valid_bech32! {
    valid_bech32_hrp_string_0, "A12UEL5L";
    valid_bech32_hrp_string_a, "a12uel5l";
    valid_bech32_hrp_string_1, "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs";
    valid_bech32_hrp_string_2, "abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw";
    valid_bech32_hrp_string_3, "11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqc8247j";
    valid_bech32_hrp_string_4, "split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w";
    valid_bech32_hrp_string_b, "?1ezyfcl";
}

macro_rules! check_valid_address_roundtrip {
    ($($test_name:ident, $addr:literal);* $(;)?) => {
        $(
            #[test]
            fn $test_name() {
                // We cannot use encode/decode for all test vectors because according to BIP-173 the
                // bech32 checksum algorithm can be used with any witness version, and this is
                // tested by the test vectors. However when BIP-350 came into effect only witness
                // version 0 uses bech32 (and this is enforced by encode/decode).
                if let Ok((hrp, bech32::Fe32::Q, program)) = bech32::segwit::decode($addr) {
                    let encoded = bech32::segwit::encode_v0(hrp, &program).expect("failed to encode address");
                    // The bips specifically say that encoder should output lowercase characters so we uppercase manually.
                    if encoded != $addr {
                        let got = encoded.to_uppercase();
                        assert_eq!(got, $addr)
                    }
                }

                let hrpstring = SegwitHrpstring::new_bech32($addr).expect("valid address");
                let hrp = hrpstring.hrp();
                let witness_version = hrpstring.witness_version();

                let encoded = hrpstring.byte_iter().bytes_to_fes().with_checksum::<Bech32>(&hrp.into()).with_witness_version(witness_version).chars().collect::<String>();

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
    bip_173_valid_address_roundtrip_0, "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
    bip_173_valid_address_roundtrip_1, "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sl5k7";
    bip_173_valid_address_roundtrip_2, "bc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7k7grplx";
    bip_173_valid_address_roundtrip_3, "BC1SW50QA3JX3S";
    bip_173_valid_address_roundtrip_4, "bc1zw508d6qejxtdg4y5r3zarvaryvg6kdaj";
    bip_173_valid_address_roundtrip_5, "tb1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesrxh6hy";
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
    bip_173_invalid_address_0, "tc1qw508d6qejxtdg4y5r3zarvary0c5xw7kg3g4ty";
    // Invalid checksum
    bip_173_invalid_address_1, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t5";
    // Invalid witness version
    bip_173_invalid_address_2, "BC13W508D6QEJXTDG4Y5R3ZARVARY0C5XW7KN40WF2";
    // Invalid program length
    bip_173_invalid_address_3, "bc1rw5uspcuh";
    // Invalid program length
    bip_173_invalid_address_4, "bc10w508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7kw5rljs90";
    // Invalid program length for witness version 0 (per BIP-141)
    bip_173_invalid_address_5, "BC1QR508D6QEJXTDG4Y5R3ZARVARYV98GJ9P";
    // Mixed case
    bip_173_invalid_address_6, "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sL5k7";
    // zero padding of more than 4 bits
    bip_173_invalid_address_7, "bc1zw508d6qejxtdg4y5r3zarvaryvqyzf3du";
    // Non-zero padding in 8-to-5 conversion
    bip_173_invalid_address_8, "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3pjxtptv";
    // Empty data section
    bip_173_invalid_address_14, "bc1gmk9yu";
}
