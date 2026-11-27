use rand_core::{Rng, SeedableRng};
use rand_pcg::{Lcg128Xsl64, Pcg64};

#[test]
fn test_lcg128xsl64_advancing() {
    for seed in 0..20 {
        let mut rng1 = Lcg128Xsl64::seed_from_u64(seed);
        let mut rng2 = rng1.clone();
        for _ in 0..20 {
            rng1.next_u64();
        }
        rng2.advance(20);
        assert_eq!(rng1, rng2);
    }
}

#[test]
fn test_lcg128xsl64_construction() {
    // Test that various construction techniques produce a working RNG.
    #[rustfmt::skip]
    let seed = [1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16,
            17,18,19,20, 21,22,23,24, 25,26,27,28, 29,30,31,32];
    let mut rng1 = Lcg128Xsl64::from_seed(seed);
    assert_eq!(rng1.next_u64(), 8740028313290271629);

    let mut rng2 = Lcg128Xsl64::from_rng(&mut rng1);
    assert_eq!(rng2.next_u64(), 1922280315005786345);

    let mut rng3 = Lcg128Xsl64::seed_from_u64(0);
    assert_eq!(rng3.next_u64(), 2354861276966075475);

    // This is the same as Lcg128Xsl64, so we only have a single test:
    let mut rng4 = Pcg64::seed_from_u64(0);
    assert_eq!(rng4.next_u64(), 2354861276966075475);
}

#[test]
fn test_lcg128xsl64_reference() {
    // Numbers copied from official test suite (C version).
    let mut rng = Lcg128Xsl64::new(42, 54);

    let mut results = [0u64; 6];
    for i in results.iter_mut() {
        *i = rng.next_u64();
    }
    let expected: [u64; 6] = [
        0x86b1da1d72062b68,
        0x1304aa46c9853d39,
        0xa3670e9e0dd50358,
        0xf9090e529a7dae00,
        0xc85b9fd837996f2c,
        0x606121f8e3919196,
    ];
    assert_eq!(results, expected);
}

#[test]
fn test_lcg128xsl64_advancing_restore() {
    let mut rng_orig = Lcg128Xsl64::new(1234, 567);

    for _ in 0..100 {
        rng_orig.next_u64();
    }

    let state = rng_orig.state();
    let stream = rng_orig.stream();
    let mut rng_copy = Lcg128Xsl64::from_state(state, stream);

    for _ in 0..1_000 {
        assert_eq!(rng_copy.next_u64(), rng_orig.next_u64());
    }
}

#[cfg(feature = "serde")]
#[test]
fn test_lcg128xsl64_serde() {
    use postcard;

    let mut rng = Lcg128Xsl64::seed_from_u64(0);

    let buf = postcard::to_allocvec(&rng).expect("Could not serialize");

    let mut deserialized: Lcg128Xsl64 = postcard::from_bytes(&buf).expect("Could not deserialize");

    for _ in 0..16 {
        assert_eq!(rng.next_u64(), deserialized.next_u64());
    }
}
