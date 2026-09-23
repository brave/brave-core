use std::hash::Hash;

use super::*;
use crate::{SipHasher128Hash, StableSipHasher128};

// The tests below compare the computed hashes to particular expected values
// in order to test that we produce the same results on different platforms,
// regardless of endianness and `usize` and `isize` size differences (this
// of course assumes we run these tests on platforms that differ in those
// ways). The expected values depend on the hashing algorithm used, so they
// need to be updated whenever StableHasher changes its hashing algorithm.

#[derive(Debug, PartialEq)]
struct TestHash([u64; 2]);

impl FromStableHash for TestHash {
    type Hash = SipHasher128Hash;

    fn from(SipHasher128Hash(hash): Self::Hash) -> TestHash {
        TestHash(hash)
    }
}

#[test]
fn test_hash_integers() {
    // Test that integers are handled consistently across platforms.
    let test_u8 = 0xAB_u8;
    let test_u16 = 0xFFEE_u16;
    let test_u32 = 0x445577AA_u32;
    let test_u64 = 0x01234567_13243546_u64;
    let test_u128 = 0x22114433_66557788_99AACCBB_EEDDFF77_u128;
    let test_usize = 0xD0C0B0A0_usize;

    let test_i8 = -100_i8;
    let test_i16 = -200_i16;
    let test_i32 = -300_i32;
    let test_i64 = -400_i64;
    let test_i128 = -500_i128;
    let test_isize = -600_isize;

    let mut h = StableSipHasher128::new();
    test_u8.hash(&mut h);
    test_u16.hash(&mut h);
    test_u32.hash(&mut h);
    test_u64.hash(&mut h);
    test_u128.hash(&mut h);
    test_usize.hash(&mut h);
    test_i8.hash(&mut h);
    test_i16.hash(&mut h);
    test_i32.hash(&mut h);
    test_i64.hash(&mut h);
    test_i128.hash(&mut h);
    test_isize.hash(&mut h);

    // This depends on the hashing algorithm. See note at top of file.
    let expected = TestHash([13997337031081104755, 6178945012502239489]);

    assert_eq!(expected, h.finish());
}

#[test]
fn test_hash_usize() {
    // Test that usize specifically is handled consistently across platforms.
    let test_usize = 0xABCDEF01_usize;

    let mut h = StableSipHasher128::new();
    test_usize.hash(&mut h);

    // This depends on the hashing algorithm. See note at top of file.
    let expected = TestHash([12037165114281468837, 3094087741167521712]);

    assert_eq!(expected, h.finish());
}

#[test]
fn test_hash_isize() {
    // Test that isize specifically is handled consistently across platforms.
    let test_isize = -7_isize;

    let mut h = StableSipHasher128::new();
    test_isize.hash(&mut h);

    // This depends on the hashing algorithm. See note at top of file.
    let expected = TestHash([3979067582695659080, 2322428596355037273]);

    assert_eq!(expected, h.finish());
}

fn hash<T: Hash>(t: &T) -> TestHash {
    let mut h = StableSipHasher128::new();
    t.hash(&mut h);
    h.finish()
}

// Check that the `isize` hashing optimization does not produce the same hash when permuting two
// values.
#[test]
fn test_isize_compression() {
    fn check_hash(a: u64, b: u64) {
        let hash_a = hash(&(a as isize, b as isize));
        let hash_b = hash(&(b as isize, a as isize));
        assert_ne!(
            hash_a, hash_b,
            "The hash stayed the same when permuting values `{a}` and `{b}`!",
        );
    }

    check_hash(0xAA, 0xAAAA);
    check_hash(0xFF, 0xFFFF);
    check_hash(0xAAAA, 0xAAAAAA);
    check_hash(0xAAAAAA, 0xAAAAAAAA);
    check_hash(0xFF, 0xFFFFFFFFFFFFFFFF);
    check_hash(u64::MAX /* -1 */, 1);
}

#[test]
fn test_cloned_hasher_output() {
    // Test that integers are handled consistently across platforms.
    let test_u8 = 0xAB_u8;
    let test_u16 = 0xFFEE_u16;
    let test_u32 = 0x445577AA_u32;

    let mut h1 = StableSipHasher128::new();
    test_u8.hash(&mut h1);
    test_u16.hash(&mut h1);

    let h2 = h1.clone();
    let mut h3 = h1.clone();
    // Make sure the cloned hasher can be fed more values.
    test_u32.hash(&mut h3);

    let h1_hash: TestHash = h1.finish();
    assert_eq!(h1_hash, h2.finish());
    assert_ne!(h1_hash, h3.finish());
}

#[test]
fn test_hash_trait_finish() {
    fn hash<H: Hasher>(h: &H) -> u64 {
        h.finish()
    }

    // Test that integers are handled consistently across platforms.
    let test_u8 = 0xAB_u8;
    let test_u16 = 0xFFEE_u16;
    let test_u32 = 0x445577AA_u32;

    let mut h1 = StableSipHasher128::new();
    test_u8.hash(&mut h1);
    test_u16.hash(&mut h1);
    test_u32.hash(&mut h1);

    assert_eq!(hash(&h1), hash(&h1));
    assert_eq!(hash(&h1), 13655241286414701638);
}
