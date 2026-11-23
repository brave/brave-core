//! Random number generator tests.

#![cfg(feature = "rng")]

use chacha20::{
    ChaCha8Rng, ChaCha12Rng, ChaCha20Rng, SerializedRngState,
    rand_core::{Rng, SeedableRng},
};
use hex_literal::hex;

const KEY: [u8; 32] = hex!("0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20");
const STREAM: u64 = 0xF0F1F2F3_F4F5F6F7;
const BLOCK_WORDS: u8 = 16;

#[test]
fn test_rng_output() {
    let mut rng = ChaCha20Rng::from_seed(KEY);
    let mut bytes = [0u8; 13];

    rng.fill_bytes(&mut bytes);
    let expected = hex!("B1697E9FC6461E1983D131CF69");
    assert_eq!(bytes, expected);

    rng.fill_bytes(&mut bytes);
    let expected = hex!("A7A3FC134F149880E8BB2B5D23");
    assert_eq!(bytes, expected);
}

#[test]
fn test_chacha_true_values_a() {
    // Test vectors 1 and 2 from
    // https://tools.ietf.org/html/draft-nir-cfrg-chacha20-poly1305-04
    let seed = [0u8; 32];
    let mut rng = ChaCha20Rng::from_seed(seed);

    let mut results = [0u32; 16];
    for i in results.iter_mut() {
        *i = rng.next_u32();
    }
    let expected = [
        0xade0b876, 0x903df1a0, 0xe56a5d40, 0x28bd8653, 0xb819d2bd, 0x1aed8da0, 0xccef36a8,
        0xc70d778b, 0x7c5941da, 0x8d485751, 0x3fe02477, 0x374ad8b8, 0xf4b8436a, 0x1ca11815,
        0x69b687c3, 0x8665eeb2,
    ];
    assert_eq!(results, expected);

    for i in results.iter_mut() {
        *i = rng.next_u32();
    }
    let expected = [
        0xbee7079f, 0x7a385155, 0x7c97ba98, 0x0d082d73, 0xa0290fcb, 0x6965e348, 0x3e53c612,
        0xed7aee32, 0x7621b729, 0x434ee69c, 0xb03371d5, 0xd539d874, 0x281fed31, 0x45fb0a51,
        0x1f0ae1ac, 0x6f4d794b,
    ];
    assert_eq!(results, expected);
}

#[test]
fn test_chacha_true_values_b() {
    // Test vector 3 from
    // https://tools.ietf.org/html/draft-nir-cfrg-chacha20-poly1305-04
    let seed = hex!("0000000000000000000000000000000000000000000000000000000000000001");
    let mut rng = ChaCha20Rng::from_seed(seed);

    // Skip block 0
    for _ in 0..16 {
        rng.next_u32();
    }

    let mut results = [0u32; 16];
    for i in results.iter_mut() {
        *i = rng.next_u32();
    }
    let expected = [
        0x2452eb3a, 0x9249f8ec, 0x8d829d9b, 0xddd4ceb1, 0xe8252083, 0x60818b01, 0xf38422b8,
        0x5aaa49c9, 0xbb00ca8e, 0xda3ba7b4, 0xc4b592d1, 0xfdf2732f, 0x4436274e, 0x2561b3c8,
        0xebdd4aa6, 0xa0136c00,
    ];
    assert_eq!(results, expected);
}

#[test]
fn test_chacha_true_values_c() {
    // Test vector 4 from
    // https://tools.ietf.org/html/draft-nir-cfrg-chacha20-poly1305-04
    let seed = hex!("00FF000000000000000000000000000000000000000000000000000000000000");
    let expected = [
        0xfb4dd572, 0x4bc42ef1, 0xdf922636, 0x327f1394, 0xa78dea8f, 0x5e269039, 0xa1bebbc1,
        0xcaf09aae, 0xa25ab213, 0x48a6b46c, 0x1b9d9bcb, 0x092c5be6, 0x546ca624, 0x1bec45d5,
        0x87f47473, 0x96f0992e,
    ];
    let expected_end = 3 * 16;
    let mut results = [0u32; 16];

    // Test block 2 by skipping block 0 and 1
    let mut rng1 = ChaCha20Rng::from_seed(seed);
    for _ in 0..32 {
        rng1.next_u32();
    }
    for i in results.iter_mut() {
        *i = rng1.next_u32();
    }
    assert_eq!(results, expected);
    assert_eq!(rng1.get_word_pos(), expected_end);

    // Test block 2 by using `set_word_pos`
    let mut rng2 = ChaCha20Rng::from_seed(seed);
    rng2.set_word_pos(2 * 16);
    for i in results.iter_mut() {
        *i = rng2.next_u32();
    }
    assert_eq!(results, expected);
    assert_eq!(rng2.get_word_pos(), expected_end);

    // Test block 2 by using `set_block_pos` and u32
    let mut rng3 = ChaCha20Rng::from_seed(seed);
    rng3.set_block_pos(2);
    results = [0u32; 16];
    for i in results.iter_mut() {
        *i = rng3.next_u32();
    }
    assert_eq!(results, expected);
    assert_eq!(rng3.get_word_pos(), expected_end);

    // Test block 2 by using `set_block_pos` and [u8; 8]
    let mut rng4 = ChaCha20Rng::from_seed(seed);
    rng4.set_block_pos(2);
    let mut buf = [0u8; 8];
    for chunk in expected.chunks_exact(2) {
        rng4.fill_bytes(&mut buf);
        assert_eq!(buf[..4], chunk[0].to_le_bytes());
        assert_eq!(buf[4..], chunk[1].to_le_bytes());
    }
    assert_eq!(rng4.get_word_pos(), expected_end);

    // Test skipping behaviour with other types
    let mut buf = [0u8; 32];
    rng2.fill_bytes(&mut buf[..]);
    assert_eq!(rng2.get_word_pos(), expected_end + 8);
    rng2.fill_bytes(&mut buf[0..25]);
    assert_eq!(rng2.get_word_pos(), expected_end + 15);
    rng2.next_u64();
    assert_eq!(rng2.get_word_pos(), expected_end + 17);
    rng2.next_u32();
    rng2.next_u64();
    assert_eq!(rng2.get_word_pos(), expected_end + 20);
    rng2.fill_bytes(&mut buf[0..1]);
    assert_eq!(rng2.get_word_pos(), expected_end + 21);
}

#[test]
fn test_chacha_multiple_blocks() {
    let seed = hex!("0000000001000000020000000300000004000000050000000600000007000000");
    let mut rng = ChaCha20Rng::from_seed(seed);

    // Store the 17*i-th 32-bit word,
    // i.e., the i-th word of the i-th 16-word block
    let mut results = [0u32; 16];
    for i in results.iter_mut() {
        *i = rng.next_u32();
        for _ in 0..16 {
            rng.next_u32();
        }
    }
    let expected = [
        0xf225c81a, 0x6ab1be57, 0x04d42951, 0x70858036, 0x49884684, 0x64efec72, 0x4be2d186,
        0x3615b384, 0x11cfa18e, 0xd3c50049, 0x75c775f6, 0x434c6530, 0x2c5bad8f, 0x898881dc,
        0x5f1c86d9, 0xc1f8e7f4,
    ];
    assert_eq!(results, expected);
}

#[test]
fn test_chacha_true_bytes() {
    let seed = [0u8; 32];
    let mut rng = ChaCha20Rng::from_seed(seed);
    let mut results = [0u8; 32];
    rng.fill_bytes(&mut results);
    let expected = hex!("76B8E0ADA0F13D90405D6AE55386BD28BDD219B8A08DED1AA836EFCC8B770DC7");
    assert_eq!(results, expected);
}

#[test]
fn test_chacha_construction() {
    let seed = hex!("0000000000000000010000000000000002000000000000000300000000000000");
    let mut rng1 = ChaCha20Rng::from_seed(seed);
    assert_eq!(rng1.next_u32(), 137206642);

    assert_eq!(rng1.get_seed(), seed);

    let mut rng2 = ChaCha20Rng::from_rng(&mut rng1);
    assert_eq!(rng2.next_u32(), 1325750369);
}

#[test]
fn test_chacha_nonce() {
    use hex_literal::hex;
    // Test vector 5 from
    // https://www.rfc-editor.org/rfc/rfc8439#section-2.3.2
    let seed = hex!("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    let mut rng = ChaCha20Rng::from_seed(seed);

    rng.set_stream(0x000000004a000000);
    rng.set_block_pos(u64::from_le_bytes(hex!("0000000000000009")));

    // The test vectors omit the first 64-bytes of the keystream
    let mut discard_first_64 = [0u8; 64];
    rng.fill_bytes(&mut discard_first_64);

    let mut results = [0u32; 16];
    for i in results.iter_mut() {
        *i = rng.next_u32();
    }
    let expected = [
        0xe4e7f110, 0x15593bd1, 0x1fdd0f50, 0xc47120a3, 0xc7f4d1c7, 0x0368c033, 0x9aaa2204,
        0x4e6cd4c3, 0x466482d2, 0x09aa9f07, 0x05d7c214, 0xa2028bd9, 0xd19c12b5, 0xb94e16de,
        0xe883d0cb, 0x4e3c50a2,
    ];

    assert_eq!(results, expected);
}

#[test]
fn test_chacha_nonce_2() {
    // Test vector 5 from
    // https://tools.ietf.org/html/draft-nir-cfrg-chacha20-poly1305-04
    // Although we do not support setting a nonce, we try it here anyway so
    // we can use this test vector.
    let seed = [0u8; 32];
    let mut rng = ChaCha20Rng::from_seed(seed);
    // 96-bit nonce in LE order is: 0,0,0,0, 0,0,0,0, 0,0,0,2
    rng.set_stream(2u64 << (24 + 32));

    let mut results = [0u32; 16];
    for i in results.iter_mut() {
        *i = rng.next_u32();
    }
    let expected = [
        0x374dc6c2, 0x3736d58c, 0xb904e24a, 0xcd3f93ef, 0x88228b1a, 0x96a4dfb3, 0x5b76ab72,
        0xc727ee54, 0x0e0e978a, 0xf3145c95, 0x1b748ea8, 0xf786c297, 0x99c28f5f, 0x628314e8,
        0x398a19fa, 0x6ded1b53,
    ];
    assert_eq!(results, expected);
}

/// Test vector 9 from https://github.com/pyca/cryptography/blob/main/vectors/cryptography_vectors/ciphers/ChaCha20/counter-overflow.txt
#[test]
fn counter_wrap_1() {
    let mut rng = ChaCha20Rng::from_seed([0u8; 32]);
    let block_pos = 18446744073709551615;
    assert_eq!(block_pos, u64::MAX);
    rng.set_block_pos(block_pos);

    let mut output = [0u8; 64 * 3];
    rng.fill_bytes(&mut output);
    let expected = hex!(
        "d7918cd8620cf832532652c04c01a553092cfb32e7b3f2f5467ae9674a2e9eec17368e"
        "c8027a357c0c51e6ea747121fec45284be0f099d2b3328845607b1768976b8e0ada0f1"
        "3d90405d6ae55386bd28bdd219b8a08ded1aa836efcc8b770dc7da41597c5157488d77"
        "24e03fb8d84a376a43b8f41518a11cc387b669b2ee65869f07e7be5551387a98ba977c"
        "732d080dcb0f29a048e3656912c6533e32ee7aed29b721769ce64e43d57133b074d839"
        "d531ed1f28510afb45ace10a1f4b794d6f"
    );
    assert_eq!(expected, output);
}

/// Counts how many bytes were incorrect, and returns:
///
/// (`index_of_first_incorrect_word`, `num_incorrect_bytes`)
fn count_incorrect_bytes(expected: &[u8], output: &[u8]) -> (Option<usize>, u32) {
    assert_eq!(expected.len(), output.len());
    let mut num_incorrect_bytes = 0;
    let mut index_of_first_incorrect_word = None;
    expected
        .iter()
        .enumerate()
        .zip(output.iter())
        .for_each(|((i, a), b)| {
            if a.ne(b) {
                if index_of_first_incorrect_word.is_none() {
                    index_of_first_incorrect_word = Some(i / 4);
                }
                num_incorrect_bytes += 1;
            }
        });
    (index_of_first_incorrect_word, num_incorrect_bytes)
}

/// Test vector 8 from https://github.com/pyca/cryptography/blob/main/vectors/cryptography_vectors/ciphers/ChaCha20/counter-overflow.txt
#[test]
fn counter_overflow_and_diagnostics() {
    let mut rng = ChaCha20Rng::from_seed([0u8; 32]);
    let block_pos = 4294967295;
    assert_eq!(block_pos, u64::from(u32::MAX));
    rng.set_block_pos(4294967295);

    let mut output = [0u8; 64 * 4];
    rng.fill_bytes(&mut output[..64 * 3]);
    let block_before_overflow = hex!(
        "ace4cd09e294d1912d4ad205d06f95d9c2f2bfcf453e8753f128765b62215f4d92c74f"
        "2f626c6a640c0b1284d839ec81f1696281dafc3e684593937023b58b1d"
    );
    let first_block_after_overflow = hex!(
        "3db41d3aa0d329285de6f225e6e24bd59c9a17006943d5c9b680e3873bdc683a581946"
        "9899989690c281cd17c96159af0682b5b903468a61f50228cf09622b5a"
    );
    let second_block_after_overflow = hex!(
        "46f0f6efee15c8f1b198cb49d92b990867905159440cc723916dc0012826981039ce17"
        "66aa2542b05db3bd809ab142489d5dbfe1273e7399637b4b3213768aaa"
    );
    assert!(
        output[..64].eq(&block_before_overflow),
        "The first parblock was incorrect before overflow, indicating that ChaCha was not implemented correctly for this backend. Check the rounds() fn or the functions that it calls"
    );

    rng.set_block_pos(u64::from(u32::MAX) - 1);
    let mut skipped_blocks = [0u8; 64 * 3];
    rng.fill_bytes(&mut skipped_blocks);
    rng.fill_bytes(&mut output[64 * 3..]);

    output.chunks_exact(64).enumerate().skip(1).zip(&[first_block_after_overflow, second_block_after_overflow, second_block_after_overflow]).for_each(|((i, a), b)| {
        let (index_of_first_incorrect_word, num_incorrect_bytes) = count_incorrect_bytes(a, b);
        let msg = if num_incorrect_bytes == 0 {
            "The block was correct and this will not be shown"
        } else if num_incorrect_bytes > 32 {
            "Most of the block was incorrect, indicating an issue with the counter using 32-bit addition towards the beginning of fn rounds()"
        } else if num_incorrect_bytes <= 8 && matches!(index_of_first_incorrect_word, Some(12 | 13)) {
            "When the state was added to the results/res buffer at the end of fn rounds, the counter was probably incremented in 32-bit fashion for this parblock"
        } else {
            // this is probably unreachable in the event of a failed assertion, but it depends on the seed
            "Some of the block was incorrect"
        };
        assert!(a.eq(b), "PARBLOCK #{} uses incorrect counter addition\nDiagnostic = {}\nnum_incorrect_bytes = {}\nindex_of_first_incorrect_word = {:?}", i + 1, msg, num_incorrect_bytes, index_of_first_incorrect_word);
    });
}

#[test]
fn test_wrapping_add() {
    let mut rng = ChaCha20Rng::from_seed(KEY);
    // test counter wrapping-add
    rng.set_word_pos((1 << 68) - 65);
    let mut output = [3u8; 1280];
    rng.fill_bytes(&mut output);

    assert_ne!(output, [0u8; 1280]);

    assert!(rng.get_word_pos() < 2000);
    assert!(rng.get_word_pos() != 0);
}

#[test]
fn test_chacha_clone_streams() {
    let seed = [
        0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0, 7, 0,
        0, 0,
    ];
    let mut rng1 = ChaCha20Rng::from_seed(seed);
    let mut rng2 = ChaCha20Rng::from_seed(seed);
    for _ in 0..16 {
        assert_eq!(rng1.next_u64(), rng2.next_u64());
    }

    rng1.set_stream(51);
    assert_eq!(rng1.get_stream(), 51);
    assert_eq!(rng2.get_stream(), 0);
    let mut fill_1 = [0u8; 7];
    rng1.fill_bytes(&mut fill_1);
    let mut fill_2 = [0u8; 7];
    rng2.fill_bytes(&mut fill_2);
    assert_ne!(fill_1, fill_2);
    for _ in 0..7 {
        assert!(rng1.next_u64() != rng2.next_u64());
    }
    rng2.set_stream(51); // switch part way through block
    for _ in 7..16 {
        assert_ne!(rng1.next_u64(), rng2.next_u64());
    }
    rng1.set_stream(51);
    rng2.set_stream(51);
    for _ in 0..16 {
        assert_eq!(rng1.next_u64(), rng2.next_u64());
    }
}

#[test]
fn test_chacha_word_pos_wrap_exact() {
    let mut rng = ChaCha20Rng::from_seed(Default::default());
    // refilling the buffer in set_word_pos will wrap the block counter to 0
    let last_block = (1 << 68) - u128::from(4 * BLOCK_WORDS);
    rng.set_word_pos(last_block);
    assert_eq!(rng.get_word_pos(), last_block);
}

#[test]
fn test_chacha_word_pos_wrap_excess() {
    let mut rng = ChaCha20Rng::from_seed(Default::default());
    // refilling the buffer in set_word_pos will wrap the block counter past 0
    let last_block = (1 << 68) - u128::from(BLOCK_WORDS);
    rng.set_word_pos(last_block);
    assert_eq!(rng.get_word_pos(), last_block);
}

#[test]
fn test_chacha_word_pos_zero() {
    let mut rng = ChaCha20Rng::from_seed(Default::default());
    assert_eq!(rng.get_word_pos(), 0);
    rng.set_word_pos(0);
    assert_eq!(rng.get_word_pos(), 0);
}

#[test]
#[allow(trivial_casts)]
fn test_trait_objects() {
    use rand_core::CryptoRng;

    let seed = Default::default();
    let mut rng1 = ChaCha20Rng::from_seed(seed);
    let rng2 = &mut ChaCha20Rng::from_seed(seed) as &mut dyn CryptoRng;
    for _ in 0..1000 {
        assert_eq!(rng1.next_u64(), rng2.next_u64());
    }
}

/// If this test fails, the backend may not be
/// performing 64-bit addition.
#[test]
fn counter_wrapping_64_bit_counter() {
    let mut rng = ChaCha20Rng::from_seed([0u8; 32]);

    // get first four blocks and word pos
    let mut first_blocks = [0u8; 64 * 4];
    rng.fill_bytes(&mut first_blocks);
    let first_blocks_end_word_pos = rng.get_word_pos();
    let first_blocks_end_block_counter = rng.get_block_pos();

    // get first four blocks after wrapping
    rng.set_block_pos(u64::MAX);
    let mut result = [0u8; 64 * 5];
    rng.fill_bytes(&mut result);
    assert_eq!(first_blocks_end_word_pos, rng.get_word_pos());
    assert_eq!(first_blocks_end_block_counter, rng.get_block_pos());

    if first_blocks[0..64 * 4].ne(&result[64..]) {
        for (i, (a, b)) in first_blocks.iter().zip(result.iter().skip(64)).enumerate() {
            assert!(!a.ne(b), "i = {}\na = {}\nb = {}", i, a, b);
        }
    }
    assert_eq!(&first_blocks[0..64 * 4], &result[64..]);
}

/// If this test fails, the backend may be doing 32-bit addition.
#[test]
fn counter_not_wrapping_at_32_bits() {
    let mut rng = ChaCha20Rng::from_seed([0u8; 32]);

    // get first four blocks and word pos
    let mut first_blocks = [0u8; 64 * 4];
    rng.fill_bytes(&mut first_blocks);
    let first_blocks_end_word_pos = rng.get_word_pos();

    // get first four blocks after the supposed overflow
    rng.set_block_pos(u64::from(u32::MAX));
    let mut result = [0u8; 64 * 5];
    rng.fill_bytes(&mut result);
    assert_ne!(first_blocks_end_word_pos, rng.get_word_pos());
    assert_eq!(
        rng.get_word_pos(),
        first_blocks_end_word_pos + (1 << 32) * u128::from(BLOCK_WORDS)
    );
    assert_ne!(&first_blocks[0..64 * 4], &result[64..]);
}

#[test]
fn test_chacha8rng_serde_roundtrip() {
    for skip_words in 0..100 {
        let mut rng = ChaCha8Rng::from_seed(KEY);
        rng.set_stream(STREAM);
        for _ in 0..skip_words {
            let _ = rng.next_u32();
        }
        let state = rng.serialize_state();
        let mut rng2 = ChaCha8Rng::deserialize_state(&state);
        for _ in 0..100 {
            assert_eq!(rng.next_u32(), rng2.next_u32());
        }
    }
}

#[test]
fn test_chacha12rng_serde_roundtrip() {
    for skip_words in 0..100 {
        let mut rng = ChaCha12Rng::from_seed(KEY);
        rng.set_stream(STREAM);
        for _ in 0..skip_words {
            let _ = rng.next_u32();
        }
        let state = rng.serialize_state();
        let mut rng2 = ChaCha12Rng::deserialize_state(&state);
        for _ in 0..100 {
            assert_eq!(rng.next_u32(), rng2.next_u32());
        }
    }
}

#[test]
fn test_chacha20rng_serde_roundtrip() {
    for skip_words in 0..100 {
        let mut rng = ChaCha20Rng::from_seed(KEY);
        rng.set_stream(STREAM);
        for _ in 0..skip_words {
            let _ = rng.next_u32();
        }
        let state = rng.serialize_state();
        let mut rng2 = ChaCha20Rng::deserialize_state(&state);
        for _ in 0..100 {
            assert_eq!(rng.next_u32(), rng2.next_u32());
        }
    }
}

#[test]
fn test_rng_serialized_state_stability() {
    const EXPECTED: SerializedRngState = hex!(
        "0102030405060708090A0B0C0D0E0F10"
        "1112131415161718191A1B1C1D1E1F20"
        "F7F6F5F4F3F2F1F06400000000000000"
        "00"
    );
    let mut rng = ChaCha8Rng::from_seed(KEY);
    rng.set_stream(STREAM);
    for _ in 0..100 {
        let _ = rng.next_u32();
    }
    let state = rng.serialize_state();
    assert_eq!(state, EXPECTED);

    let mut rng = ChaCha12Rng::from_seed(KEY);
    rng.set_stream(STREAM);
    for _ in 0..100 {
        let _ = rng.next_u32();
    }
    let state = rng.serialize_state();
    assert_eq!(state, EXPECTED);

    let mut rng = ChaCha20Rng::from_seed(KEY);
    rng.set_stream(STREAM);
    for _ in 0..100 {
        let _ = rng.next_u32();
    }
    let state = rng.serialize_state();
    assert_eq!(state, EXPECTED);
}
