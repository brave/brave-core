use rand_core::{Rng, SeedableRng};
use rand_pcg::{Lcg64Xsh32, Pcg32};

#[test]
fn test_lcg64xsh32_advancing() {
    for seed in 0..20 {
        let mut rng1 = Lcg64Xsh32::seed_from_u64(seed);
        let mut rng2 = rng1.clone();
        for _ in 0..20 {
            rng1.next_u32();
        }
        rng2.advance(20);
        assert_eq!(rng1, rng2);
    }
}

#[test]
fn test_lcg64xsh32_construction() {
    // Test that various construction techniques produce a working RNG.
    let seed = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
    let mut rng1 = Lcg64Xsh32::from_seed(seed);
    assert_eq!(rng1.next_u64(), 1204678643940597513);

    let mut rng2 = Lcg64Xsh32::from_rng(&mut rng1);
    assert_eq!(rng2.next_u64(), 12384929573776311845);

    let mut rng3 = Lcg64Xsh32::seed_from_u64(0);
    assert_eq!(rng3.next_u64(), 18195738587432868099);

    // This is the same as Lcg64Xsh32, so we only have a single test:
    let mut rng4 = Pcg32::seed_from_u64(0);
    assert_eq!(rng4.next_u64(), 18195738587432868099);
}

#[test]
fn test_lcg64xsh32_reference() {
    // Numbers copied from official test suite.
    let mut rng = Lcg64Xsh32::new(42, 54);

    let mut results = [0u32; 6];
    for i in results.iter_mut() {
        *i = rng.next_u32();
    }
    let expected: [u32; 6] = [
        0xa15c02b7, 0x7b47f409, 0xba1d3330, 0x83d2f293, 0xbfa4784b, 0xcbed606e,
    ];
    assert_eq!(results, expected);
}

#[test]
fn test_lcg64xsh32_advancing_restore() {
    let mut rng_orig = Lcg64Xsh32::new(1234, 567);

    for _ in 0..100 {
        rng_orig.next_u64();
    }

    let state = rng_orig.state();
    let stream = rng_orig.stream();
    let mut rng_copy = Lcg64Xsh32::from_state(state, stream);

    for _ in 0..1_000 {
        assert_eq!(rng_copy.next_u64(), rng_orig.next_u64());
    }
}

#[cfg(feature = "serde")]
#[test]
fn test_lcg64xsh32_serde() {
    use postcard;
    let mut rng = Lcg64Xsh32::seed_from_u64(0);

    let buf = postcard::to_allocvec(&rng).expect("Could not serialize");

    let mut deserialized: Lcg64Xsh32 = postcard::from_bytes(&buf).expect("Could not deserialize");

    for _ in 0..16 {
        assert_eq!(rng.next_u64(), deserialized.next_u64());
    }
}
