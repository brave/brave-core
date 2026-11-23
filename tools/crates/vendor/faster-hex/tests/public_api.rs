use faster_hex::{
    hex_check, hex_check_with_case, hex_decode, hex_decode_with_case, hex_encode, hex_encode_upper,
    CheckCase, Error,
};
use proptest::prelude::*;

#[cfg(not(miri))]
type Encoder = for<'a> fn(&[u8], &'a mut [u8]) -> Result<&'a mut str, Error>;

fn value(byte: u8, case: CheckCase) -> Option<u8> {
    match byte {
        b'0'..=b'9' => Some(byte - b'0'),
        b'a'..=b'f' if case != CheckCase::Upper => Some(byte - b'a' + 10),
        b'A'..=b'F' if case != CheckCase::Lower => Some(byte - b'A' + 10),
        _ => None,
    }
}

fn assert_capacity(error: Error, expected: usize) {
    assert!(
        matches!(error, Error::OutputTooSmall { required, .. } if required == expected),
        "{:?}",
        error
    );
}

fn assert_invalid(error: Error, expected_index: usize, expected_byte: u8) {
    assert!(
        matches!(error, Error::InvalidChar { index, byte, .. }
        if (index, byte) == (expected_index, expected_byte)),
        "{:?}",
        error
    );
}

#[test]
fn encode_returns_only_written_utf8_prefix() {
    for encode in [hex_encode, hex_encode_upper] {
        let mut dst = [0xff; 80];
        assert_eq!(encode(&[], &mut dst).unwrap(), "");
        assert_eq!(dst, [0xff; 80]);
        let text = encode(&[0xab; 32], &mut dst).unwrap();
        assert_eq!(text.len(), 64);
        assert!(text.eq_ignore_ascii_case(&"ab".repeat(32)));
        assert_eq!(&dst[64..], &[0xff; 16]);
        let mut short = [0xa5; 63];
        assert_capacity(encode(&[0; 32], &mut short).unwrap_err(), 64);
        assert_eq!(short, [0xa5; 63]);
    }
}

#[test]
fn decode_requires_room_for_all_input_and_preserves_spare_bytes() {
    for len in 0..=4 {
        let mut dst = vec![0xa5; len];
        let result = hex_decode(b"abcd", &mut dst);
        if len < 2 {
            assert_capacity(result.unwrap_err(), 2);
            assert_eq!(dst, vec![0xa5; len]);
        } else {
            assert_eq!(result.unwrap(), &[0xab, 0xcd]);
            assert_eq!(&dst[..2], &[0xab, 0xcd]);
            assert!(dst[2..].iter().all(|b| *b == 0xa5));
        }
    }
    let mut dst = [0xa5; 65];
    assert_eq!(hex_decode(b"", &mut dst).unwrap(), &[]);
    assert_eq!(dst, [0xa5; 65]);
}

#[test]
fn decoded_prefix_borrows_only_the_destination_and_is_mutable() {
    let mut dst = [0xa5; 4];
    let written = {
        let source = String::from("abcd");
        hex_decode(source.as_bytes(), &mut dst).unwrap()
    };
    assert_eq!(written, &[0xab, 0xcd]);
    written[1] = 0xff;
    assert_eq!(dst, [0xab, 0xff, 0xa5, 0xa5]);
}

#[test]
fn errors_have_stable_precedence_and_do_not_write() {
    let mut dst = [0xa5; 32];
    assert_eq!(hex_decode(b"g", &mut dst), Err(Error::OddLength));
    assert_capacity(hex_decode(b"gg", &mut []).unwrap_err(), 1);
    assert_invalid(hex_decode(b"gg", &mut dst).unwrap_err(), 0, b'g');
    assert_eq!(dst, [0xa5; 32]);
    assert!(hex_check(b"a"));
    assert!(hex_check(b""));
    assert!(!hex_check_with_case(b"a", CheckCase::Upper));
    assert_eq!(CheckCase::default(), CheckCase::None);
}

#[test]
#[cfg(feature = "defmt-03")]
fn defmt_formats_on_host_without_an_embedded_logger() {
    let error = hex_decode(b"0g", &mut [0]).unwrap_err();
    defmt::println!("{:?} {:?}", error, CheckCase::Lower);
    defmt::flush();
}

#[test]
#[cfg(not(miri))]
fn every_character_pair_in_scalar_and_vector_sized_inputs() {
    for a in 0..=255 {
        for b in 0..=255 {
            for case in [CheckCase::None, CheckCase::Lower, CheckCase::Upper] {
                let expected = value(a, case)
                    .zip(value(b, case))
                    .map(|(hi, lo)| hi * 16 + lo);
                let mut src = [0; 66];
                for pair in src.chunks_exact_mut(2) {
                    pair.copy_from_slice(&[a, b]);
                }
                for len in [2, 64, 66] {
                    let mut dst = [0xa5; 34];
                    assert_eq!(hex_check_with_case(&src[..len], case), expected.is_some());
                    let result = hex_decode_with_case(&src[..len], &mut dst, case);
                    match expected {
                        Some(byte) => {
                            assert_eq!(result.unwrap(), vec![byte; len / 2]);
                            assert!(dst[..len / 2].iter().all(|b| *b == byte));
                            assert!(dst[len / 2..].iter().all(|b| *b == 0xa5));
                        }
                        None => {
                            let (index, byte) = if value(a, case).is_none() {
                                (0, a)
                            } else {
                                (1, b)
                            };
                            assert_invalid(result.unwrap_err(), index, byte);
                            assert_eq!(dst, [0xa5; 34]);
                        }
                    }
                }
            }
        }
    }
}

#[test]
fn strict_uppercase_fixed_hash_keeps_numeric_nibbles() {
    for digit in b'0'..=b'9' {
        let mut output = [0xa5; 32];
        hex_decode_with_case(&[digit; 64], &mut output, CheckCase::Upper).unwrap();
        assert_eq!(output, [(digit - b'0') * 17; 32]);
    }
}

#[test]
#[cfg(not(miri))]
fn vector_boundaries_and_independent_buffer_offsets() {
    for len in 0..=257 {
        let data: Vec<u8> = (0..len + 32).map(|i| (i * 37 + len) as u8).collect();
        for src_offset in 0..32 {
            let source = &data[src_offset..src_offset + len];
            let dst_offset = (src_offset * 7) % 32;
            for (encode, case) in [
                (hex_encode as Encoder, CheckCase::Lower),
                (hex_encode_upper as Encoder, CheckCase::Upper),
            ] {
                let mut encoded = vec![0xa5; len * 2 + 64];
                let range = dst_offset..dst_offset + len * 2;
                let expected = match case {
                    CheckCase::Upper => hex::encode_upper(source),
                    _ => hex::encode(source),
                };
                assert_eq!(
                    &*encode(source, &mut encoded[range.clone()]).unwrap(),
                    expected
                );
                assert!(encoded[..dst_offset]
                    .iter()
                    .chain(&encoded[range.end..])
                    .all(|b| *b == 0xa5));
                let mut decoded = vec![0xa5; len + 64];
                hex_decode_with_case(
                    &encoded[range],
                    &mut decoded[src_offset..src_offset + len],
                    case,
                )
                .unwrap();
                assert_eq!(&decoded[src_offset..src_offset + len], source);
                assert!(decoded[..src_offset]
                    .iter()
                    .chain(&decoded[src_offset + len..])
                    .all(|b| *b == 0xa5));
            }
        }
    }
}

#[test]
fn invalid_characters_at_each_position_never_commit_output() {
    for len in [2, 30, 32, 34, 62, 64, 66, 128, 130] {
        for position in 0..len {
            for invalid in [0, b'/', b':', b'@', b'G', b'`', b'g', 0x7f, 0x80, 0xff] {
                let mut src = vec![b'a'; len];
                src[position] = invalid;
                let mut dst = vec![0xa5; len / 2 + 7];
                assert_invalid(hex_decode(&src, &mut dst).unwrap_err(), position, invalid);
                assert_eq!(dst, vec![0xa5; len / 2 + 7]);
            }
            for (letter, case) in [(b'A', CheckCase::Lower), (b'a', CheckCase::Upper)] {
                let mut src = vec![b'0'; len];
                src[position] = letter;
                let mut dst = vec![0xa5; len / 2];
                assert_invalid(
                    hex_decode_with_case(&src, &mut dst, case).unwrap_err(),
                    position,
                    letter,
                );
                assert_eq!(dst, vec![0xa5; len / 2]);
            }
        }
    }
}

#[test]
fn diagnostics_identify_the_first_invalid_byte_for_each_policy() {
    let mut dst = [0xa5; 4];
    for (case, index, byte) in [
        (CheckCase::None, 4, b'g'),
        (CheckCase::Lower, 1, b'A'),
        (CheckCase::Upper, 3, b'b'),
    ] {
        assert_invalid(
            hex_decode_with_case(b"0A0bg!ff", &mut dst, case).unwrap_err(),
            index,
            byte,
        );
        assert_eq!(dst, [0xa5; 4]);
    }
    assert_invalid(hex_decode("é".as_bytes(), &mut dst).unwrap_err(), 0, 0xc3);
    assert_eq!(dst, [0xa5; 4]);
}

#[test]
fn diagnostic_block_skipping_keeps_byte_offsets_and_first_error() {
    for len in [64, 66, 128, 130, 8192] {
        for position in [0, 31, 32, 63, 64, 65, len - 1] {
            if position >= len {
                continue;
            }
            for case in [CheckCase::None, CheckCase::Lower, CheckCase::Upper] {
                let mut input = vec![b'0'; len];
                input[len - 1] = 0xff;
                input[position] = b'!';
                let mut output = vec![0xa5; len / 2 + 3];
                assert_invalid(
                    hex_decode_with_case(&input, &mut output, case).unwrap_err(),
                    position,
                    b'!',
                );
                assert!(output.iter().all(|&b| b == 0xa5));
            }
        }
    }
}

proptest! {
    #![proptest_config(if cfg!(miri) {
        ProptestConfig { cases: 8, failure_persistence: None, ..ProptestConfig::default() }
    } else {
        ProptestConfig::default()
    })]

    #[test]
    fn arbitrary_bytes_and_independent_lengths(
        src in prop::collection::vec(any::<u8>(), 0..2049),
        dst_len in 0usize..1032,
    ) {
        for case in [CheckCase::None, CheckCase::Lower, CheckCase::Upper] {
            let mut dst = vec![0xa5; dst_len];
            let result = hex_decode_with_case(&src, &mut dst, case);
            if !src.len().is_multiple_of(2) {
                prop_assert_eq!(result.unwrap_err(), Error::OddLength);
            } else if dst_len < src.len() / 2 {
                assert_capacity(result.unwrap_err(), src.len() / 2);
            } else if let Some((index, &byte)) = src.iter().enumerate().find(|(_, b)| value(**b, case).is_none()) {
                assert_invalid(result.unwrap_err(), index, byte);
            } else {
                let expected: Vec<_> = src.chunks_exact(2)
                    .map(|p| value(p[0], case).unwrap() * 16 + value(p[1], case).unwrap()).collect();
                prop_assert_eq!(result.unwrap(), expected);
                prop_assert!(dst[src.len() / 2..].iter().all(|&b| b == 0xa5));
                continue;
            }
            prop_assert_eq!(dst, vec![0xa5; dst_len]);
        }
    }

    // Uniform random bytes almost always fail validation near the beginning.
    // Generate valid long inputs first, then damage a selected byte or length.
    #[test]
    fn generated_hex_covers_success_and_controlled_failures(
        data in prop::collection::vec(any::<u8>(), 0..1025),
        source_offset in 0usize..32,
        destination_offset in 0usize..32,
        spare in 0usize..33,
        position in any::<usize>(),
    ) {
        for upper in [false, true] {
            let text = if upper { hex::encode_upper(&data) } else { hex::encode(&data) };
            let case = if upper { CheckCase::Upper } else { CheckCase::Lower };
            let mut source = vec![0xff; source_offset];
            source.extend_from_slice(text.as_bytes());
            let source = &mut source[source_offset..];
            let mut output = vec![0xa5; destination_offset + data.len() + spare];
            let written = hex_decode_with_case(source, &mut output[destination_offset..], case).unwrap();
            prop_assert_eq!(written, &data);
            prop_assert!(output[..destination_offset].iter().chain(&output[destination_offset + data.len()..]).all(|&b| b == 0xa5));

            // Mix case independently of the encoder, keeping numeric nibbles intact.
            for (index, byte) in source.iter_mut().enumerate() {
                if index % 3 == 0 { *byte = byte.to_ascii_uppercase(); }
                else { *byte = byte.to_ascii_lowercase(); }
            }
            output.fill(0xa5);
            prop_assert_eq!(hex_decode(source, &mut output[destination_offset..]).unwrap(), &data);
            prop_assert!(output[..destination_offset].iter().chain(&output[destination_offset + data.len()..]).all(|&b| b == 0xa5));

            if !source.is_empty() {
                let damaged = position % source.len();
                source[damaged] = 0xff;
                for case in [CheckCase::None, CheckCase::Lower, CheckCase::Upper] {
                    let (index, &byte) = source.iter().enumerate().find(|(_, b)| value(**b, case).is_none()).unwrap();
                    output.fill(0xa5);
                    assert_invalid(hex_decode_with_case(source, &mut output[destination_offset..], case).unwrap_err(), index, byte);
                    prop_assert!(output.iter().all(|&b| b == 0xa5));
                    assert_capacity(hex_decode_with_case(source, &mut output[destination_offset..][..data.len() - 1], case).unwrap_err(), data.len());
                    prop_assert!(output.iter().all(|&b| b == 0xa5));
                    prop_assert_eq!(hex_decode_with_case(&source[..source.len() - 1], &mut output[destination_offset..], case), Err(Error::OddLength));
                    prop_assert!(output.iter().all(|&b| b == 0xa5));
                }
            }
        }
    }
}

#[cfg(feature = "alloc")]
#[test]
fn strings_and_append_reuse_capacity_preserving_unicode_prefix() {
    use faster_hex::{hex_append, hex_append_upper, hex_string, hex_string_upper};
    for len in [
        0, 1, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129,
    ] {
        let input: Vec<_> = (0..len).map(|i| (i * 73 + 19) as u8).collect();
        assert_eq!(hex_string(&input), hex::encode(&input));
        assert_eq!(hex_string_upper(&input), hex::encode_upper(&input));
        let mut appended = String::from("前缀:");
        hex_append(&input, &mut appended);
        assert_eq!(appended, format!("前缀:{}", hex::encode(&input)));
    }
    assert_eq!(hex_string(&[0xab; 32]), "ab".repeat(32));
    assert_eq!(hex_string_upper(&[0xab; 32]), "AB".repeat(32));
    let mut dst = String::with_capacity(256);
    dst.push_str("前缀:");
    let pointer = dst.as_ptr();
    assert_eq!(&*hex_append(&[0xab; 32], &mut dst), "ab".repeat(32));
    assert_eq!(&*hex_append_upper(&[0xcd; 32], &mut dst), "CD".repeat(32));
    assert_eq!(hex_append(&[], &mut dst), "");
    assert_eq!(dst, format!("前缀:{}{}", "ab".repeat(32), "CD".repeat(32)));
    assert_eq!(dst.as_ptr(), pointer);
    let mut growing = String::from("prefix:");
    hex_append(&[0xff; 128], &mut growing);
    assert_eq!(growing, format!("prefix:{}", "ff".repeat(128)));
}

#[cfg(feature = "heapless-09")]
#[test]
fn heapless_capacity_is_fallible_and_features_are_additive() {
    use faster_hex::heapless_09::{hex_string, hex_string_upper};
    assert_eq!(hex_string::<0>(&[]).unwrap().as_str(), "");
    assert_eq!(hex_string::<8>(&[0xab]).unwrap().as_str(), "ab");
    assert_eq!(hex_string_upper::<2>(&[0xab]).unwrap().as_str(), "AB");
    assert_capacity(hex_string::<1>(&[0xab]).unwrap_err(), 2);
    assert_capacity(hex_string_upper::<0>(&[0xab]).unwrap_err(), 2);
    #[cfg(feature = "alloc")]
    assert_eq!(faster_hex::hex_string(&[0xab]), "ab");
}

#[cfg(feature = "heapless-09")]
#[test]
fn heapless_checks_exact_odd_and_vector_boundary_capacities() {
    fn check<const N: usize>() {
        for len in [0, N / 2, N / 2 + 1, N + 1] {
            let input: Vec<_> = (0..len).map(|i| (i * 73) as u8).collect();
            for upper in [false, true] {
                let result = if upper {
                    faster_hex::heapless_09::hex_string_upper::<N>(&input)
                } else {
                    faster_hex::heapless_09::hex_string::<N>(&input)
                };
                if len * 2 > N {
                    assert_capacity(result.unwrap_err(), len * 2);
                } else {
                    let expected = if upper {
                        hex::encode_upper(&input)
                    } else {
                        hex::encode(&input)
                    };
                    assert_eq!(result.unwrap().as_str(), expected);
                }
            }
        }
    }
    check::<0>();
    check::<1>();
    check::<2>();
    check::<3>();
    check::<15>();
    check::<16>();
    check::<17>();
    check::<31>();
    check::<32>();
    check::<33>();
    check::<127>();
    check::<128>();
    check::<129>();
    check::<513>();
}
