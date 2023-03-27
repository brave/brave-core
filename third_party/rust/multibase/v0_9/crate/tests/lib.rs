use multibase::{decode, encode, Base, Base::*};

fn encode_decode_assert(input: &[u8], test_cases: Vec<(Base, &str)>) {
    for (base, output) in test_cases {
        assert_eq!(encode(base, input), output);
        assert_eq!(decode(output).unwrap(), (base, input.to_vec()));
    }
}

#[test]
fn test_bases_code() {
    assert_eq!(Identity.code(), '\x00');
    assert_eq!(Base2.code(), '0');
}

#[test]
fn test_bases_from_code() {
    assert_eq!(Base::from_code('\x00').unwrap(), Identity);
    assert_eq!(Base::from_code('0').unwrap(), Base2);
}

#[test]
fn test_round_trip() {
    let test_cases: &[&str] = &[
        "helloworld",
        "we all want decentralization",
        "zdj7WfBb6j58iSJuAzDcSZgy2SxFhdpJ4H87uvMpfyN6hRGyH",
    ];

    for case in test_cases {
        let encoded = encode(Base58Btc, case);
        let decoded = decode(encoded).unwrap();
        assert_eq!(decoded, (Base58Btc, case.as_bytes().to_vec()))
    }
}

#[test]
fn test_basic() {
    let input = b"yes mani !";
    let test_cases = vec![
        (Identity, "\x00yes mani !"),
        (
            Base2,
            "001111001011001010111001100100000011011010110000101101110011010010010000000100001",
        ),
        (Base8, "7362625631006654133464440102"),
        (Base10, "9573277761329450583662625"),
        (Base16Lower, "f796573206d616e692021"),
        (Base16Upper, "F796573206D616E692021"),
        (Base32Lower, "bpfsxgidnmfxgsibb"),
        (Base32Upper, "BPFSXGIDNMFXGSIBB"),
        (Base32HexLower, "vf5in683dc5n6i811"),
        (Base32HexUpper, "VF5IN683DC5N6I811"),
        (Base32PadLower, "cpfsxgidnmfxgsibb"),
        (Base32PadUpper, "CPFSXGIDNMFXGSIBB"),
        (Base32HexPadLower, "tf5in683dc5n6i811"),
        (Base32HexPadUpper, "TF5IN683DC5N6I811"),
        (Base32Z, "hxf1zgedpcfzg1ebb"),
        (Base36Lower, "k2lcpzo5yikidynfl"),
        (Base36Upper, "K2LCPZO5YIKIDYNFL"),
        (Base58Flickr, "Z7Pznk19XTTzBtx"),
        (Base58Btc, "z7paNL19xttacUY"),
        (Base64, "meWVzIG1hbmkgIQ"),
        (Base64Pad, "MeWVzIG1hbmkgIQ=="),
        (Base64Url, "ueWVzIG1hbmkgIQ"),
        (Base64UrlPad, "UeWVzIG1hbmkgIQ=="),
    ];
    encode_decode_assert(input, test_cases);
}

#[test]
fn preserves_leading_zero() {
    let input = b"\x00yes mani !";
    let test_cases = vec![
        (Identity, "\x00\x00yes mani !"),
        (Base2, "00000000001111001011001010111001100100000011011010110000101101110011010010010000000100001"),
        (Base8, "7000745453462015530267151100204"),
        (Base10, "90573277761329450583662625"),
        (Base16Lower, "f00796573206d616e692021"),
        (Base16Upper, "F00796573206D616E692021"),
        (Base32Lower, "bab4wk4zanvqw42jaee"),
        (Base32Upper, "BAB4WK4ZANVQW42JAEE"),
        (Base32HexLower, "v01smasp0dlgmsq9044"),
        (Base32HexUpper, "V01SMASP0DLGMSQ9044"),
        (Base32PadLower, "cab4wk4zanvqw42jaee======"),
        (Base32PadUpper, "CAB4WK4ZANVQW42JAEE======"),
        (Base32HexPadLower, "t01smasp0dlgmsq9044======"),
        (Base32HexPadUpper, "T01SMASP0DLGMSQ9044======"),
        (Base32Z, "hybhskh3ypiosh4jyrr"),
        (Base36Lower, "k02lcpzo5yikidynfl"),
        (Base36Upper, "K02LCPZO5YIKIDYNFL"),
        (Base58Flickr, "Z17Pznk19XTTzBtx"),
        (Base58Btc, "z17paNL19xttacUY"),
        (Base64, "mAHllcyBtYW5pICE"),
        (Base64Pad, "MAHllcyBtYW5pICE="),
        (Base64Url, "uAHllcyBtYW5pICE"),
        (Base64UrlPad, "UAHllcyBtYW5pICE="),
    ];
    encode_decode_assert(input, test_cases);
}

#[test]
fn preserves_two_leading_zeroes() {
    let input = b"\x00\x00yes mani !";
    let test_cases = vec![
        (Identity, "\x00\x00\x00yes mani !"),
        (Base2, "0000000000000000001111001011001010111001100100000011011010110000101101110011010010010000000100001"),
        (Base8, "700000171312714403326055632220041"),
        (Base10, "900573277761329450583662625"),
        (Base16Lower, "f0000796573206d616e692021"),
        (Base16Upper, "F0000796573206D616E692021"),
        (Base32Lower, "baaahszltebwwc3tjeaqq"),
        (Base32Upper, "BAAAHSZLTEBWWC3TJEAQQ"),
        (Base32HexLower, "v0007ipbj41mm2rj940gg"),
        (Base32HexUpper, "V0007IPBJ41MM2RJ940GG"),
        (Base32PadLower, "caaahszltebwwc3tjeaqq===="),
        (Base32PadUpper, "CAAAHSZLTEBWWC3TJEAQQ===="),
        (Base32HexPadLower, "t0007ipbj41mm2rj940gg===="),
        (Base32HexPadUpper, "T0007IPBJ41MM2RJ940GG===="),
        (Base32Z, "hyyy813murbssn5ujryoo"),
        (Base36Lower, "k002lcpzo5yikidynfl"),
        (Base36Upper, "K002LCPZO5YIKIDYNFL"),
        (Base58Flickr, "Z117Pznk19XTTzBtx"),
        (Base58Btc, "z117paNL19xttacUY"),
        (Base64, "mAAB5ZXMgbWFuaSAh"),
        (Base64Pad, "MAAB5ZXMgbWFuaSAh"),
        (Base64Url, "uAAB5ZXMgbWFuaSAh"),
        (Base64UrlPad, "UAAB5ZXMgbWFuaSAh"),

    ];
    encode_decode_assert(input, test_cases);
}

#[test]
fn case_insensitivity() {
    let input = b"hello world";
    let test_cases = vec![
        (Base16Lower, "f68656c6c6f20776F726C64"),
        (Base16Upper, "F68656c6c6f20776F726C64"),
        (Base32Lower, "bnbswy3dpeB3W64TMMQ"),
        (Base32Upper, "Bnbswy3dpeB3W64TMMQ"),
        (Base32HexLower, "vd1imor3f41RMUSJCCG"),
        (Base32HexUpper, "Vd1imor3f41RMUSJCCG"),
        (Base32PadLower, "cnbswy3dpeB3W64TMMQ======"),
        (Base32PadUpper, "Cnbswy3dpeB3W64TMMQ======"),
        (Base32HexPadLower, "td1imor3f41RMUSJCCG======"),
        (Base32HexPadUpper, "Td1imor3f41RMUSJCCG======"),
        (Base36Lower, "kfUvrsIvVnfRbjWaJo"),
        (Base36Upper, "KfUVrSIVVnFRbJWAJo"),
    ];
    for (base, output) in test_cases {
        assert_eq!(decode(output).unwrap(), (base, input.to_vec()));
    }
}
