use rand_core::{Rng, SeedableRng};
use rand_pcg::{Lcg128CmDxsm64, Pcg64Dxsm};

#[test]
fn test_lcg128cmdxsm64_advancing() {
    for seed in 0..20 {
        let mut rng1 = Lcg128CmDxsm64::seed_from_u64(seed);
        let mut rng2 = rng1.clone();
        for _ in 0..20 {
            rng1.next_u64();
        }
        rng2.advance(20);
        assert_eq!(rng1, rng2);
    }
}

#[test]
fn test_lcg128cmdxsm64_construction() {
    // Test that various construction techniques produce a working RNG.
    #[rustfmt::skip]
    let seed = [1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16,
            17,18,19,20, 21,22,23,24, 25,26,27,28, 29,30,31,32];
    let mut rng1 = Lcg128CmDxsm64::from_seed(seed);
    assert_eq!(rng1.next_u64(), 12201417210360370199);

    let mut rng2 = Lcg128CmDxsm64::from_rng(&mut rng1);
    assert_eq!(rng2.next_u64(), 11487972556150888383);

    let mut rng3 = Lcg128CmDxsm64::seed_from_u64(0);
    assert_eq!(rng3.next_u64(), 4111470453933123814);

    // This is the same as Lcg128CmDxsm64, so we only have a single test:
    let mut rng4 = Pcg64Dxsm::seed_from_u64(0);
    assert_eq!(rng4.next_u64(), 4111470453933123814);
}

#[test]
fn test_lcg128cmdxsm64_reference() {
    // Numbers determined using `pcg_engines::cm_setseq_dxsm_128_64` from pcg-cpp.
    let mut rng = Lcg128CmDxsm64::new(42, 54);

    let mut results = [0u64; 6];
    for i in results.iter_mut() {
        *i = rng.next_u64();
    }
    let expected: [u64; 6] = [
        17331114245835578256,
        10267467544499227306,
        9726600296081716989,
        10165951391103677450,
        12131334649314727261,
        10134094537930450875,
    ];
    assert_eq!(results, expected);
}

#[test]
fn test_lcg128cmdxsm64_advancing_restore() {
    let mut rng_orig = Lcg128CmDxsm64::new(1234, 567);

    for _ in 0..100 {
        rng_orig.next_u64();
    }

    let state = rng_orig.state();
    let stream = rng_orig.stream();
    let mut rng_copy = Lcg128CmDxsm64::from_state(state, stream);

    for _ in 0..1_000 {
        assert_eq!(rng_copy.next_u64(), rng_orig.next_u64());
    }
}

#[cfg(feature = "serde")]
#[test]
fn test_lcg128cmdxsm64_serde() {
    use postcard;

    let mut rng = Lcg128CmDxsm64::seed_from_u64(0);

    let buf = postcard::to_allocvec(&rng).expect("Could not serialize");

    let mut deserialized: Lcg128CmDxsm64 =
        postcard::from_bytes(&buf).expect("Could not deserialize");

    for _ in 0..16 {
        assert_eq!(rng.next_u64(), deserialized.next_u64());
    }
}
