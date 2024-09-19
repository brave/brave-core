use digest::dev::{feed_rand_16mib, fixed_reset_test};
use digest::new_test;
use hex_literal::hex;
use ripemd::{Digest, Ripemd128, Ripemd160, Ripemd256, Ripemd320};

// Test vectors from FIPS 180-1 and from the [RIPEMD webpage][1].
//
// [1] https://homes.esat.kuleuven.be/~bosselae/ripemd160.html
new_test!(ripemd128_main, "ripemd128", Ripemd128, fixed_reset_test);
new_test!(ripemd160_main, "ripemd160", Ripemd160, fixed_reset_test);
new_test!(ripemd256_main, "ripemd256", Ripemd256, fixed_reset_test);
new_test!(ripemd320_main, "ripemd320", Ripemd320, fixed_reset_test);

#[test]
fn ripemd128_1mil_a() {
    let mut h = Ripemd128::new();
    let buf = [b'a'; 1000];
    for _ in 0..1000 {
        h.update(&buf[..]);
    }
    assert_eq!(
        h.finalize(),
        hex!("4a7f5723f954eba1216c9d8f6320431f").into()
    );
}

#[test]
fn ripemd128_rand() {
    let mut h = Ripemd128::new();
    feed_rand_16mib(&mut h);
    assert_eq!(
        h.finalize()[..],
        hex!("01eb52529bcec15bd0cb4040ec998632")[..]
    );
}

#[test]
fn ripemd160_1mil_a() {
    let mut h = Ripemd160::new();
    let buf = [b'a'; 1000];
    for _ in 0..1000 {
        h.update(&buf[..]);
    }
    assert_eq!(
        h.finalize(),
        hex!("52783243c1697bdbe16d37f97f68f08325dc1528").into()
    );
}

#[test]
fn ripemd160_rand() {
    let mut h = Ripemd160::new();
    feed_rand_16mib(&mut h);
    assert_eq!(
        h.finalize()[..],
        hex!("bcd8c672932125776af3c60eeeb58bbaf206f386")[..]
    );
}

#[test]
fn ripemd256_1mil_a() {
    let mut h = Ripemd256::new();
    let buf = [b'a'; 1000];
    for _ in 0..1000 {
        h.update(&buf[..]);
    }
    assert_eq!(
        h.finalize(),
        hex!("ac953744e10e31514c150d4d8d7b677342e33399788296e43ae4850ce4f97978").into()
    );
}

#[test]
fn ripemd256_rand() {
    let mut h = Ripemd256::new();
    feed_rand_16mib(&mut h);
    assert_eq!(
        h.finalize()[..],
        hex!("6492ffe075896441b737900bdf58fc960e77477e42a2a61bc02c66fd689b69d0")[..]
    );
}

#[test]
#[rustfmt::skip]
fn ripemd320_1mil_a() {
    let mut h = Ripemd320::new();
    let buf = [b'a'; 1000];
    for _ in 0..1000 {
        h.update(&buf[..]);
    }
    assert_eq!(
        h.finalize(),
        hex!("
            bdee37f4371e20646b8b0d862dda16292ae36f40
            965e8c8509e63d1dbddecc503e2b63eb9245bb66
        ").into()
    );
}

#[test]
#[rustfmt::skip]
fn ripemd320_rand() {
    let mut h = Ripemd320::new();
    feed_rand_16mib(&mut h);
    assert_eq!(
        h.finalize()[..],
        hex!("
            3a905312162c5c173639f6cc1cdf51d14e8bda02
            865767592e26d9343fbec348ce55ce39b4b4b56f
        ")[..]
    );
}
