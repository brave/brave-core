//! Padding and buffering for every message length from 0 to 600 bytes. That is
//! nine blocks. It covers every position of the length suffix and every carry
//! between `update` calls.
//!
//! The expected value is the SHA-1 of all 601 digests in sequence. Python
//! `hashlib` computed it independently.

use hex_literal::hex;
use sha1dc::Hasher;

const EXPECTED: [u8; 20] = hex!("0bf5f0239e1a04b1469bc173cebd65a2ec207fb2");

fn message(n: usize) -> Vec<u8> {
    (0..n).map(|i| ((i * 31 + n * 7) % 256) as u8).collect()
}

#[test]
fn every_length() {
    let mut acc = Hasher::new();
    for n in 0..=600 {
        acc.update(
            sha1dc::digest(&message(n))
                .expect("no collision")
                .as_bytes(),
        );
    }
    assert_eq!(acc.finalize().expect("no collision"), EXPECTED);
}

/// The same, with one byte per `update`, so each call crosses the buffer at a
/// different offset.
#[test]
fn every_length_byte_by_byte() {
    let mut acc = Hasher::new();
    for n in 0..=600 {
        let mut hasher = Hasher::new();
        for byte in message(n) {
            hasher.update(&[byte]);
        }
        acc.update(hasher.finalize().expect("no collision").as_bytes());
    }
    assert_eq!(acc.finalize().expect("no collision"), EXPECTED);
}

/// The same, with one hasher and `finalize_reset`, which must restore the
/// length counter and the buffer exactly.
#[test]
fn every_length_through_finalize_reset() {
    let mut acc = Hasher::new();
    let mut hasher = Hasher::new();
    for n in 0..=600 {
        hasher.update(&message(n));
        acc.update(hasher.finalize_reset().expect("no collision").as_bytes());
    }
    assert_eq!(acc.finalize().expect("no collision"), EXPECTED);
}

/// The same, split into two `update` calls at every possible point.
#[test]
fn every_split_point() {
    for n in [0usize, 1, 55, 56, 63, 64, 65, 119, 120, 128, 191, 192, 256] {
        let data = message(n);
        let expected = sha1dc::digest(&data).expect("no collision");
        for split in 0..=n {
            let mut hasher = Hasher::new();
            hasher.update(&data[..split]);
            hasher.update(&data[split..]);
            assert_eq!(
                hasher.finalize().expect("no collision"),
                expected,
                "len {n}, split at {split}"
            );
        }
    }
}
