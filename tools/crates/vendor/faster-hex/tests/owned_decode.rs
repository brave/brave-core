use faster_hex::{hex_decode_array, hex_decode_array_with_case, CheckCase, Error};

fn array_roundtrip<const N: usize>() {
    let bytes: [u8; N] = core::array::from_fn(|index| (index * 73) as u8);
    let lower = hex::encode(bytes);
    let upper = hex::encode_upper(bytes);
    assert_eq!(hex_decode_array::<N>(lower.as_bytes()).unwrap(), bytes);
    assert_eq!(hex_decode_array::<N>(upper.as_bytes()).unwrap(), bytes);
    for (text, case) in [(&lower, CheckCase::Lower), (&upper, CheckCase::Upper)] {
        assert_eq!(
            hex_decode_array_with_case::<N>(text.as_bytes(), case).unwrap(),
            bytes
        );
    }
    let too_long = format!("{lower}00");
    assert!(matches!(
        hex_decode_array::<N>(too_long.as_bytes()),
        Err(Error::LengthMismatch { expected, actual, .. }) if (expected, actual) == (N, N + 1)
    ));
    if N != 0 {
        assert!(matches!(
            hex_decode_array::<N>(&lower.as_bytes()[..lower.len() - 2]),
            Err(Error::LengthMismatch { expected, actual, .. }) if (expected, actual) == (N, N - 1)
        ));
    }
}

#[test]
fn arrays_cover_empty_hashes_and_vector_boundaries() {
    array_roundtrip::<0>();
    array_roundtrip::<1>();
    array_roundtrip::<4>();
    array_roundtrip::<7>();
    array_roundtrip::<8>();
    array_roundtrip::<15>();
    array_roundtrip::<16>();
    array_roundtrip::<17>();
    array_roundtrip::<31>();
    array_roundtrip::<32>();
    array_roundtrip::<33>();
    array_roundtrip::<63>();
    array_roundtrip::<64>();
    array_roundtrip::<65>();
    array_roundtrip::<129>();
    array_roundtrip::<256>();
}

#[test]
fn arrays_reject_lengths_before_characters_and_preserve_byte_diagnostics() {
    assert_eq!(hex_decode_array::<0>(b"g"), Err(Error::OddLength));
    assert_eq!(hex_decode_array::<32>(b"g"), Err(Error::OddLength));
    assert!(matches!(
        hex_decode_array::<0>(b"gg"),
        Err(Error::LengthMismatch {
            expected: 0,
            actual: 1,
            ..
        })
    ));
    assert!(matches!(
        hex_decode_array::<32>(b"gg"),
        Err(Error::LengthMismatch {
            expected: 32,
            actual: 1,
            ..
        })
    ));
    assert!(matches!(
        hex_decode_array::<1>(b"0g"),
        Err(Error::InvalidChar {
            index: 1,
            byte: b'g',
            ..
        })
    ));
    assert!(matches!(
        hex_decode_array::<1>("é".as_bytes()),
        Err(Error::InvalidChar {
            index: 0,
            byte: 0xc3,
            ..
        })
    ));
    for (case, index, byte) in [
        (CheckCase::None, 4, b'g'),
        (CheckCase::Lower, 1, b'A'),
        (CheckCase::Upper, 3, b'b'),
    ] {
        assert!(matches!(
            hex_decode_array_with_case::<4>(b"0A0bg!ff", case),
            Err(Error::InvalidChar { index: actual_index, byte: actual_byte, .. })
                if (actual_index, actual_byte) == (index, byte)
        ));
    }
    // The result owns its data and does not borrow the temporary input string.
    let decoded: [u8; 4] = {
        let text = String::from("001122ff");
        hex_decode_array(text.as_bytes()).unwrap()
    };
    assert_eq!(decoded, [0, 0x11, 0x22, 0xff]);
}

#[cfg(feature = "alloc")]
#[test]
fn vectors_decode_complete_inputs_for_every_case_policy() {
    use faster_hex::{hex_decode_vec, hex_decode_vec_with_case};
    for len in [0, 1, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 256, 4096] {
        let bytes: Vec<_> = (0..len).map(|index| (index * 73) as u8).collect();
        let lower = hex::encode(&bytes);
        let upper = hex::encode_upper(&bytes);
        assert_eq!(hex_decode_vec(lower.as_bytes()).unwrap(), bytes);
        assert_eq!(hex_decode_vec(upper.as_bytes()).unwrap(), bytes);
        for (text, case) in [(&lower, CheckCase::Lower), (&upper, CheckCase::Upper)] {
            assert_eq!(
                hex_decode_vec_with_case(text.as_bytes(), case).unwrap(),
                bytes
            );
        }
    }
    let decoded: Vec<u8> = {
        let source = String::from("0001");
        hex_decode_vec(source.as_bytes()).unwrap()
    };
    assert_eq!(decoded, [0, 1]);
}

#[cfg(feature = "alloc")]
#[test]
fn vectors_keep_slice_decoder_error_precedence_and_grammar() {
    use faster_hex::{hex_decode_vec, hex_decode_vec_with_case};
    assert_eq!(hex_decode_vec(b"g"), Err(Error::OddLength));
    for input in [b"0x00".as_slice(), b"00 0", b"0\n", &[0xff, b'0']] {
        assert!(matches!(
            hex_decode_vec(input),
            Err(Error::InvalidChar { .. })
        ));
    }
    for (case, index, byte) in [
        (CheckCase::None, 4, b'g'),
        (CheckCase::Lower, 1, b'A'),
        (CheckCase::Upper, 3, b'b'),
    ] {
        assert!(matches!(
            hex_decode_vec_with_case(b"0A0bg!ff", case),
            Err(Error::InvalidChar { index: actual_index, byte: actual_byte, .. })
                if (actual_index, actual_byte) == (index, byte)
        ));
    }
}

fn owned_block_errors<const N: usize>() {
    let mut input = vec![b'1'; N * 2];
    for (case, invalid) in [
        (CheckCase::None, b'g'),
        (CheckCase::Lower, b'A'),
        (CheckCase::Upper, b'a'),
    ] {
        assert_eq!(
            hex_decode_array_with_case::<N>(&input, case).unwrap(),
            [0x11; N]
        );
        for position in [0, 1, 63, 64, 127, 128, 2047, 2048, N * 2 - 1] {
            input[position] = invalid;
            let error = hex_decode_array_with_case::<N>(&input, case).unwrap_err();
            assert!(matches!(error, Error::InvalidChar { index, byte, .. }
                if (index, byte) == (position, invalid)));
            #[cfg(feature = "alloc")]
            assert_eq!(
                faster_hex::hex_decode_vec_with_case(&input, case),
                Err(error)
            );
            // A later failure must never hide an earlier case violation.
            input[0] = invalid;
            assert!(matches!(
                hex_decode_array_with_case::<N>(&input, case),
                Err(Error::InvalidChar { index: 0, .. })
            ));
            input[0] = b'1';
            input[position] = b'1';
        }
    }
}

#[test]
fn owned_outputs_report_errors_across_complete_blocks_and_tails() {
    owned_block_errors::<1025>();
    owned_block_errors::<4096>();
}

proptest::proptest! {
    #![proptest_config(if cfg!(miri) {
        proptest::test_runner::Config { cases: 8, failure_persistence: None, ..Default::default() }
    } else {
        proptest::test_runner::Config::default()
    })]

    #[test]
    fn arbitrary_input_matches_slice_errors_after_array_length_validation(
        src in proptest::collection::vec(proptest::prelude::any::<u8>(), 0..130),
    ) {
        use proptest::prelude::*;
        for case in [CheckCase::None, CheckCase::Lower, CheckCase::Upper] {
            let mut output = vec![0; src.len() / 2];
            let expected = faster_hex::hex_decode_with_case(&src, &mut output, case).map(|bytes| bytes.to_vec());
            #[cfg(feature = "alloc")]
            prop_assert_eq!(faster_hex::hex_decode_vec_with_case(&src, case), expected.clone());
            let array = hex_decode_array_with_case::<32>(&src, case);
            if !src.len().is_multiple_of(2) {
                prop_assert_eq!(array, Err(Error::OddLength));
            } else if src.len() / 2 != 32 {
                prop_assert!(matches!(array, Err(Error::LengthMismatch { expected: 32, actual, .. }) if actual == src.len() / 2),
                             "array must reject both short and long input");
            } else {
                prop_assert_eq!(array.map(|bytes| bytes.to_vec()), expected);
            }
        }
    }

    #[test]
    fn owned_decoders_match_independently_generated_hashes(
        bytes in proptest::array::uniform32(proptest::prelude::any::<u8>()),
        offset in 0usize..32,
        position in 0usize..64,
        invalid in proptest::sample::select(vec![0, b'/', b':', b'G', b'g', 0x7f, 0x80, 0xff]),
    ) {
        use proptest::prelude::*;
        for (upper, case) in [(false, CheckCase::Lower), (true, CheckCase::Upper)] {
            let text = if upper { hex::encode_upper(bytes) } else { hex::encode(bytes) };
            let mut storage = vec![0xff; offset];
            storage.extend_from_slice(text.as_bytes());
            let input = &mut storage[offset..];
            prop_assert_eq!(hex_decode_array_with_case::<32>(input, case).unwrap(), bytes);
            #[cfg(feature = "alloc")]
            prop_assert_eq!(faster_hex::hex_decode_vec_with_case(input, case).unwrap(), bytes);
            let short = hex_decode_array_with_case::<4>(&input[..8], case).unwrap();
            prop_assert_eq!(short.as_slice(), &bytes[..4]);
            let short_position = position % 8;
            let original = input[short_position];
            input[short_position] = invalid;
            prop_assert!(matches!(hex_decode_array_with_case::<4>(&input[..8], case),
                Err(Error::InvalidChar { index, byte, .. }) if (index, byte) == (short_position, invalid)),
                "short array must identify its first invalid byte");
            input[short_position] = original;
            input[position] = invalid;
            let error = hex_decode_array_with_case::<32>(input, case).unwrap_err();
            prop_assert!(matches!(error, Error::InvalidChar { index, byte, .. } if (index, byte) == (position, invalid)), "wrong diagnostic: {error:?}");
            #[cfg(feature = "alloc")]
            prop_assert_eq!(faster_hex::hex_decode_vec_with_case(input, case), Err(error));
            // Error precedence is independent of the contents, even when bad input
            // lies near the end of a SIMD-sized hash.
            prop_assert!(matches!(hex_decode_array_with_case::<31>(input, case), Err(Error::LengthMismatch { expected: 31, actual: 32, .. })), "wrong length error");
            prop_assert_eq!(hex_decode_array_with_case::<32>(&input[..63], case), Err(Error::OddLength));
        }
    }
}
