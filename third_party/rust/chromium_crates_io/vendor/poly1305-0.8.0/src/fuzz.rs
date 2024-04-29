use universal_hash::{generic_array::GenericArray, UniversalHash};

use crate::{backend, Block, Key, BLOCK_SIZE};

/// Helper function for fuzzing the AVX2 backend.
pub fn fuzz_avx2(key: &Key, data: &[u8]) {
    let mut avx2 = backend::avx2::State::new(key);
    let mut soft = backend::soft::State::new(key);

    for (_i, chunk) in data.chunks(BLOCK_SIZE).enumerate() {
        if chunk.len() == BLOCK_SIZE {
            let block = GenericArray::from_slice(chunk);
            unsafe {
                avx2.compute_block(block, false);
            }
            soft.compute_block(block, false);
        } else {
            let mut block = Block::default();
            block[..chunk.len()].copy_from_slice(chunk);
            block[chunk.len()] = 1;
            unsafe {
                avx2.compute_block(&block, true);
            }
            soft.compute_block(&block, true);
        }

        // Check that the same tag would be derived after each chunk.
        // We add the chunk number to the assertion for debugging.
        // When fuzzing, we skip this check, and just look at the end.
        #[cfg(test)]
        assert_eq!(
            (_i + 1, unsafe { avx2.clone().finalize() }),
            (_i + 1, soft.clone().finalize()),
        );
    }

    assert_eq!(unsafe { avx2.finalize() }, soft.finalize());
}

fn avx2_fuzzer_test_case(data: &[u8]) {
    fuzz_avx2(data[0..32].into(), &data[32..]);
}

#[test]
fn crash_0() {
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000000,sig=06,src=000014,op=flip4,pos=11"
    ));
}

#[test]
fn crash_1() {
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000001,sig=06,src=000006+000014,op=splice,rep=64"
    ));
}

#[test]
fn crash_2() {
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000002,sig=06,src=000008+000014,op=splice,rep=32"
    ));
}

#[test]
fn crash_3() {
    // This input corresponds to a key of:
    //     r = 0x0f245bfc0f7fe5fc0fffff3400fb1c2b
    //     s = 0xffffff000001000040f6fff5ffffffff
    //
    // and input blocks:
    //    [0x01ea0010000a00ff108b72ffffffffffff, 0x01ffffffff245b74ff7fe5ffffff0040ff,
    //     0x01000a00ff108b7200ff04000002ffffff, 0x01ffffffffffffffffffff0000ffea0010,
    //     0x0180ffffffffffffffffffffffe3ffffff, 0x01ffffffffffffffffffffffffffffffff,
    //     0x01ffffffffffffffffffdfffff03ffffff, 0x01ffffffffff245b74ff7fe5ffffe4ffff,
    //     0x0112118b7d00ffeaffffffffffffffffff, 0x010e40eb10ffffffff1edd7f0010000a00]
    //
    // When this crash occurred, the software and AVX2 backends would generate the same
    // tags given the first seven blocks as input. Given the first eight blocks, the
    // following tags were generated:
    //
    //      |                                tag     |  low 128 bits of final accumulator
    // soft | 0x0004d01b9168ded528a9b541cc461988 - s = 0x0004d11b9167ded4e7b2b54bcc461989
    // avx2 | 0x0004d01b9168ded528a9b540cc461988 - s = 0x0004d11b9167ded4e7b2b54acc461989
    //                 difference = 0x0100000000
    //
    // This discrepancy was due to Unreduced130::reduce (as called during finalization)
    // not correctly reducing. During the reduction step, the upper limb's upper bits
    // (beyond 2^130) are added into the lower limb multiplied by 5 (for reduction modulo
    // 2^130 - 5). This is computed like so:
    //
    //     b = t_4 >> 26
    //     t_0 += b + (b << 2)
    //
    // It is possible for the upper limb to be 57+ bits; thus b << 2 can be 33+ bits.
    // However, the original reduction code was using _mm256_slli_epi32, which shifts
    // packed 32-bit integers; this was causing the upper bits of b to be lost. Switching
    // to _mm256_slli_epi64 (correctly treating b as a 64-bit field) solves the problem.
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000003,sig=06,src=000003,op=havoc,rep=64"
    ));
}

#[test]
fn crash_4() {
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000004,sig=06,src=000022+000005,op=splice,rep=32"
    ));
}

#[test]
fn crash_5() {
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000005,sig=06,src=000008+000007,op=splice,rep=128"
    ));
}

#[test]
fn crash_6() {
    // This input corresponds to a key of:
    //     r = 0x04040404040404040404040404040404
    //     s = 0x0404040403ef04040404040404040404
    //
    // and input:
    //     [0x04, 0x04, 0x04, 0xf2]
    //
    // The input fits into a single short block:
    //     m = 0x01f2040404
    //
    // and we should have the following computation:
    //     tag = ((m * r) % p) + s
    //         = ((0x01f2040404 * 0x04040404040404040404040404040404) % p) + s
    //         = (0x7cfdfeffffffffffffffffffffffffff8302010 % ((1 << 130) - 5)) + s
    //         = 0x1f3f7fc + 0x0404040403ef04040404040404040404
    //         = 0x0404040403ef04040404040405f7fc00
    //
    // or in bytes:
    //     tag = [
    //         0x00, 0xfc, 0xf7, 0x05, 0x04, 0x04, 0x04, 0x04,
    //         0x04, 0x04, 0xef, 0x03, 0x04, 0x04, 0x04, 0x04,
    //     ];
    //
    // The crash was caused by the final modular reduction (in the `addkey` method of the
    // Goll-Gueron implementation, and `impl Add<Aligned130> for AdditionKey` here) not
    // fully carrying all bits. `Aligned130` is guaranteed to be a 130-bit integer, but is
    // not guaranteed to be an integer modulo 2^130 - 5.
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000006,sig=06,src=000005,op=havoc,rep=8"
    ));
}

#[test]
fn crash_7() {
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000007,sig=06,src=000024+000000,op=splice,rep=64"
    ));
}

#[test]
fn crash_8() {
    // This input corresponds to a key of:
    //     r = 0x0fff00fc0000000000000000006f91ab
    //     s = 0xffffffffffffffffffffffffffffffff
    //
    // and a single input block:
    //     0x01d4d4ffffffffffffffffffffffffffff
    //
    // We should have the following computation:
    //     tag = ((m * r) % p) + s
    //         = ((0x01d4d4ffffffffffffffffffffffffffff * 0x0fff00fc0000000000000000006f91ab) % p) + s
    //         = (0x1d4b7cf881ac00000000000000cc5320bf47ff03ffffffffffffffffff906e55 % ((1 << 130) - 5)) + s
    //         = 0xe3e65b3aa217000000000000008fd63d + 0xffffffffffffffffffffffffffffffff
    //         = 0x01e3e65b3aa217000000000000008fd63c mod 128
    //
    // or in bytes:
    //     tag = [
    //         0x3c, 0xd6, 0x8f, 0x00, 0x00, 0x00, 0x00, 0x00,
    //         0x00, 0x00, 0x17, 0xa2, 0x3a, 0x5b, 0xe6, 0xe3,
    //     ];
    //
    // The crash was caused by the final modular reduction (in the `addkey` method of the
    // Goll-Gueron implementation, and `impl Add<Aligned130> for AdditionKey` here). After
    // adding s, limbs 0 and 2 have carries, while limb 1 is 0xffffffff. The original
    // implementation only carried once, after which limb 1 has a carry, which was then
    // discarded. The fix was to always carry three times, to ensure that all potential
    // carry bits are carried.
    avx2_fuzzer_test_case(include_bytes!(
        "fuzz/id=000008,sig=06,src=000019,time=165655+000011,op=splice,rep=128"
    ));
}
