//! Known-answer tests against plain SHA-1. The hasher must compute that value
//! for every message that is not part of a collision attack.

use hex_literal::hex;
use sha1dc::{Digest, Hasher};

/// Checks the digest, and that a clean message is not flagged.
#[track_caller]
fn check(data: &[u8], expected: [u8; 20]) {
    assert_eq!(sha1dc::digest(data).expect("no collision"), expected);
    assert_eq!(
        sha1dc::digest(data).expect("no collision"),
        Digest::from(expected)
    );
}

#[test]
fn nist_vectors() {
    check(b"", hex!("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
    check(b"abc", hex!("a9993e364706816aba3e25717850c26c9cd0d89d"));
    check(
        b"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        hex!("84983e441c3bd26ebaae4aa1f95129e5e54670f1"),
    );
    check(
        b"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
        hex!("a49b2446a02c645bf419f995b67091253a04a259"),
    );
    check(
        b"hello world",
        hex!("2aae6c35c94fcfb415dbe95f408b9ce91ee846ed"),
    );
}

#[test]
fn million_a() {
    check(
        &[b'a'; 1_000_000],
        hex!("34aa973cd4c4daa4f61eeb2bdbad27316534016f"),
    );
}

/// Lengths near the block boundary, where the length suffix fits or does not
/// fit.
#[test]
fn padding_boundaries() {
    let filler = |n: usize| -> Vec<u8> { (0..n).map(|i| ((i * 7 + n) % 251) as u8).collect() };
    let cases: [(usize, [u8; 20]); 12] = [
        (55, hex!("04f893eac3e6c3714b48ab7525103f7327b45271")),
        (56, hex!("5b05376842a5618186792e2ba9882a47ab8a8f61")),
        (57, hex!("11b78e8f44314c4c2fca00c65e6167899b45eb90")),
        (63, hex!("bc8885075e915702462e23b6c0f2c099b9d721ca")),
        (64, hex!("6304fccb5005d7307678d2013e0e243c3054db5c")),
        (65, hex!("044772bb5a735b181623d9a5b4d4e55dcda6e0b4")),
        (119, hex!("cd5b017eba674e96bddd0b78cbbada2dfe731030")),
        (120, hex!("8d376d7d411a376cb262704eb6cdfb956839ac8f")),
        (121, hex!("c198b1fb4de2f4f0c9b6d1006b2271c729357321")),
        (127, hex!("1b79872c0b6075124af20b6a3cff7d2ee37b4171")),
        (128, hex!("def0a6760ecba9b705b0eb5b458c81af717db1f9")),
        (129, hex!("c906373caba36b1d29b181203a5fd4ed0e0d10e5")),
    ];
    for (len, expected) in cases {
        check(&filler(len), expected);
    }
}

/// 1 MiB of pseudorandom data, which runs the multi-block paths.
#[test]
fn long_message() {
    check(
        &pseudorandom(1 << 20),
        hex!("6a706be3356371641b2ce3d990af95bf31f25564"),
    );
}

#[test]
fn reset_restores_initial_state() {
    let mut hasher = Hasher::new();
    hasher.update(b"some data that will be thrown away");
    hasher.reset();
    hasher.update(b"abc");
    assert_eq!(
        hasher.finalize().expect("no collision"),
        hex!("a9993e364706816aba3e25717850c26c9cd0d89d")
    );

    let mut hasher = Hasher::new();
    hasher.update(b"abc");
    assert_eq!(
        hasher.finalize_reset().expect("no collision"),
        hex!("a9993e364706816aba3e25717850c26c9cd0d89d")
    );
}

#[test]
fn finalize_reset_round_trips() {
    let mut hasher = Hasher::new();
    for _ in 0..3 {
        hasher.update(b"abc");
        assert_eq!(
            hasher.finalize_reset().unwrap(),
            hex!("a9993e364706816aba3e25717850c26c9cd0d89d")
        );
    }
}

#[test]
fn digest_formatting() {
    let digest = sha1dc::digest(b"abc").expect("no collision");
    assert_eq!(
        digest.to_string(),
        "a9993e364706816aba3e25717850c26c9cd0d89d"
    );
    assert_eq!(
        format!("{digest:x}"),
        "a9993e364706816aba3e25717850c26c9cd0d89d"
    );
    assert_eq!(
        format!("{digest:X}"),
        "A9993E364706816ABA3E25717850C26C9CD0D89D"
    );
    assert_eq!(
        format!("{digest:?}"),
        "Digest(a9993e364706816aba3e25717850c26c9cd0d89d)"
    );
    assert_eq!(Digest::from(digest.to_bytes()), digest);
}

#[cfg(feature = "std")]
#[test]
fn io_write() {
    use std::io::Write;

    let mut hasher = Hasher::new();
    hasher.write_all(b"hello ").unwrap();
    write!(hasher, "world").unwrap();
    assert_eq!(
        hasher.finalize().expect("no collision"),
        hex!("2aae6c35c94fcfb415dbe95f408b9ce91ee846ed")
    );
}

fn pseudorandom(len: usize) -> Vec<u8> {
    let mut seed = 0x0123_4567_89ab_cdefu64;
    (0..len)
        .map(|_| {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            (seed >> 24) as u8
        })
        .collect()
}
