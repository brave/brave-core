use crate::decode::{hex_check_fallback_with_case, hex_decode_fallback, hex_decode_unchecked};
use crate::encode::hex_encode_fallback;
use crate::{hex_check_with_case, hex_decode, hex_encode, hex_encode_upper, CheckCase};
use proptest::prelude::*;

#[cfg(all(unix, not(miri)))]
mod guarded;

proptest! {
    #[test]
    fn public_and_scalar_paths_agree(data in prop::collection::vec(any::<u8>(), 0..4097)) {
        for upper in [false, true] {
            let mut scalar = vec![0; data.len() * 2];
            let mut dispatched = scalar.clone();
            hex_encode_fallback(&data, &mut scalar, upper);
            if upper {
                hex_encode_upper(&data, &mut dispatched).unwrap();
            } else {
                hex_encode(&data, &mut dispatched).unwrap();
            }
            prop_assert_eq!(&scalar, &dispatched);
            let mut decoded = vec![0; data.len()];
            hex_decode_fallback(&scalar, &mut decoded);
            prop_assert_eq!(&decoded, &data);
            hex_decode(&scalar, &mut decoded).unwrap();
            prop_assert_eq!(&decoded, &data);
        }
    }

    #[test]
    fn arbitrary_character_checks_agree(data in prop::collection::vec(any::<u8>(), 0..4097)) {
        for case in [CheckCase::None, CheckCase::Lower, CheckCase::Upper] {
            prop_assert_eq!(hex_check_with_case(&data, case), hex_check_fallback_with_case(&data, case));
        }
    }
}

#[test]
fn internal_conversion_never_reads_past_short_source() {
    // Exercises the old AVX2 threshold with independently varying slice lengths.
    for source_len in [0, 1, 2, 4, 31, 32, 33, 63, 64, 65] {
        let source = vec![b'0'; source_len];
        for target_len in [0, 1, 31, 32, 33, 64, 65] {
            let mut target = vec![0xa5; target_len];
            hex_decode_unchecked(&source, &mut target);
            let written = source_len.min(target_len * 2) / 2;
            assert!(target[..written].iter().all(|b| *b == 0));
            assert!(target[written..].iter().all(|b| *b == 0xa5));
        }
    }
}

#[test]
fn scalar_encoder_preserves_incomplete_destination_pairs() {
    for source_len in 0..18 {
        let source = vec![0xaf; source_len];
        for target_len in 0..36 {
            for upper in [false, true] {
                let mut destination = vec![0xa5; target_len];
                hex_encode_fallback(&source, &mut destination, upper);
                let written = source_len.min(target_len / 2) * 2;
                let expected = if upper { b"AF" } else { b"af" };
                for pair in destination[..written].chunks_exact(2) {
                    assert_eq!(pair, expected);
                }
                assert!(destination[written..].iter().all(|&byte| byte == 0xa5));
            }
        }
    }
}

#[test]
#[cfg(all(
    not(miri),
    any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64")
))]
fn feature_detection_matches_host() {
    use crate::{vectorization_support, Vectorization};
    eprintln!("selected backend: {:?}", vectorization_support());
    for backend in crate::fuzzing::backends() {
        eprintln!("available test backend: {}", backend.name());
    }
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    if std::env::var("FASTER_HEX_REQUIRE_AVX2").as_deref() == Ok("1") {
        assert!(std::arch::is_x86_feature_detected!("sse4.1"));
        assert!(std::arch::is_x86_feature_detected!("avx2"));
        assert!(matches!(
            vectorization_support(),
            Vectorization::AVX2 | Vectorization::AVX512
        ));
    }
    match vectorization_support() {
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        Vectorization::AVX2 => assert!(std::arch::is_x86_feature_detected!("avx2")),
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        Vectorization::AVX512 => {
            assert!(std::arch::is_x86_feature_detected!("avx2"));
            assert!(std::arch::is_x86_feature_detected!("avx512f"));
            assert!(std::arch::is_x86_feature_detected!("avx512bw"));
        }
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        Vectorization::SSE41 => assert!(std::arch::is_x86_feature_detected!("sse4.1")),
        #[cfg(target_arch = "aarch64")]
        Vectorization::Neon => assert!(std::arch::is_aarch64_feature_detected!("neon")),
        Vectorization::None => {}
    }
}

#[cfg(not(miri))]
mod kernels {
    use super::*;
    use crate::fuzzing::backends;

    #[test]
    fn forced_backends_preserve_order_at_independent_alignments() {
        for backend in backends() {
            let name = backend.name();
            for len in [
                0, 1, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 95, 96, 97, 127, 128, 129,
            ] {
                for source_offset in 0..64 {
                    let binary: Vec<u8> = (0..len + 64).map(|i| (i * 73 + i / 7) as u8).collect();
                    let input = &binary[source_offset..][..len];
                    for upper in [false, true] {
                        let expected = if upper {
                            hex::encode_upper(input)
                        } else {
                            hex::encode(input)
                        };
                        let mut hex_source = vec![0; source_offset + len * 2];
                        hex_source[source_offset..].copy_from_slice(expected.as_bytes());
                        for target_offset in 0..64 {
                            let mut encoded = vec![0xa5; len * 2 + 64];
                            backend.encode(input, &mut encoded[target_offset..][..len * 2], upper);
                            assert_eq!(
                                &encoded[target_offset..][..len * 2],
                                expected.as_bytes(),
                                "{name}, len={len}"
                            );
                            assert!(encoded[..target_offset]
                                .iter()
                                .chain(&encoded[target_offset + len * 2..])
                                .all(|&b| b == 0xa5));
                            let mut decoded = vec![0xa5; len + 64];
                            assert!(backend.decode(
                                &hex_source[source_offset..],
                                &mut decoded[target_offset..][..len],
                                CheckCase::None,
                            ));
                            assert_eq!(
                                &decoded[target_offset..][..len],
                                input,
                                "{name}, len={len}"
                            );
                            assert!(decoded[..target_offset]
                                .iter()
                                .chain(&decoded[target_offset + len..])
                                .all(|&b| b == 0xa5));
                            decoded.fill(0xa5);
                            assert!(backend.decode_owned(
                                &hex_source[source_offset..],
                                &mut decoded[target_offset..][..len],
                                CheckCase::None,
                            ));
                            assert_eq!(&decoded[target_offset..][..len], input, "{name}");
                            assert!(decoded[..target_offset]
                                .iter()
                                .chain(&decoded[target_offset + len..])
                                .all(|&b| b == 0xa5));
                        }
                    }
                }
            }
        }
    }

    #[test]
    fn forced_backends_check_every_byte_in_every_lane_before_writing() {
        for backend in backends() {
            let name = backend.name();
            for len in [
                2, 4, 6, 8, 10, 12, 14, 16, 32, 64, 66, 128, 130, 190, 192, 194,
            ] {
                for position in 0..len {
                    for byte in 0..=255 {
                        // Vary neighboring digits to catch bits leaking between
                        // byte lanes when a kernel uses wider shifts.
                        let mut input: Vec<_> =
                            (0..len).map(|i| b'0' + (i / 4 % 8) as u8).collect();
                        input[position] = byte;
                        for case in [CheckCase::None, CheckCase::Lower, CheckCase::Upper] {
                            let valid = hex_check_fallback_with_case(&input, case);
                            let actual = backend.check(&input, case);
                            assert_eq!(actual, valid, "{name}, {len}/{position}/{byte}/{case:?}");
                            let mut output = vec![0xa5; len / 2];
                            let decoded = backend.decode(&input, &mut output, case);
                            if valid {
                                let mut expected = vec![0; len / 2];
                                hex_decode_fallback(&input, &mut expected);
                                assert!(decoded);
                                assert_eq!(output, expected, "{name}");
                            } else {
                                assert!(!decoded);
                                assert!(output.iter().all(|&b| b == 0xa5), "{}", name);
                            }
                            let mut owned = vec![0xa5; len / 2];
                            assert_eq!(backend.decode_owned(&input, &mut owned, case), valid);
                            if valid {
                                assert_eq!(owned, output, "{name}");
                            }
                        }
                    }
                }
            }
        }
    }
}
