//! Property tests over the public API.
//!
//! The other integration tests pin the digest at fixed inputs, against
//! constants from `hashlib` and from the C original. These instead state a
//! relation that must hold for *any* input, and let `quickcheck` look for a
//! counterexample and shrink it.
//!
//! `quickcheck` caps a generated `Vec` at the size of its generator, and the
//! default size is under one block. Each property therefore sets a size that
//! reaches the code it means to cover.

use quickcheck::{Gen, QuickCheck};
use sha1::Digest as _;
use sha1dc::{Collision, Digest, Hasher};

/// Long enough for several blocks, so that padding and the block loop both
/// run.
const LONG: usize = 4 * 1024;

/// Enough chunks to cross the internal buffer repeatedly, without making the
/// message so long that the test crawls.
const CHUNKED: usize = 64;

/// Hashes the parts with one `update` each.
fn hash_in_parts<'a>(parts: impl IntoIterator<Item = &'a [u8]>) -> Result<Digest, Collision> {
    let mut hasher = Hasher::new();
    for part in parts {
        hasher.update(part);
    }
    hasher.finalize()
}

/// A machine with the SHA-1 instructions never runs the scalar path, so only
/// a comparison against it can find a difference between the two.
#[test]
fn backends_agree_on_any_message() {
    fn prop(data: Vec<u8>) -> bool {
        let mut auto = Hasher::new();
        auto.update(&data);

        let mut scalar = Hasher::builder().internal_scalar_backend().build();
        scalar.update(&data);

        auto.finalize() == scalar.finalize()
    }
    QuickCheck::new()
        .rng(Gen::new(LONG))
        .tests(1_000)
        .quickcheck(prop as fn(Vec<u8>) -> bool);
}

/// The UBC filter only decides which blocks get the recompression check. It
/// must never change the digest.
#[test]
fn the_ubc_filter_does_not_change_the_digest() {
    fn prop(data: Vec<u8>) -> bool {
        let mut filtered = Hasher::builder().internal_use_ubc(true).build();
        filtered.update(&data);

        let mut unfiltered = Hasher::builder().internal_use_ubc(false).build();
        unfiltered.update(&data);

        filtered.finalize() == unfiltered.finalize()
    }
    QuickCheck::new()
        .rng(Gen::new(LONG))
        .tests(200)
        .quickcheck(prop as fn(Vec<u8>) -> bool);
}

/// Where the caller splits the message must not matter.
#[test]
fn chunked_updates_match_one_shot() {
    fn prop(chunks: Vec<Vec<u8>>) -> bool {
        hash_in_parts(chunks.iter().map(Vec::as_slice)) == sha1dc::digest(&chunks.concat())
    }
    QuickCheck::new()
        .rng(Gen::new(CHUNKED))
        .tests(2_000)
        .quickcheck(prop as fn(Vec<Vec<u8>>) -> bool);
}

/// The same, with chunks longer than [`chunked_updates_match_one_shot`]
/// generates. Equal chunks longer than a block alternate between filling the
/// buffer and compressing straight from the input.
#[test]
fn fixed_size_chunks_match_one_shot() {
    fn prop(data: Vec<u8>, chunk: usize) -> bool {
        hash_in_parts(data.chunks(1 + chunk % 1024)) == sha1dc::digest(&data)
    }
    QuickCheck::new()
        .rng(Gen::new(LONG))
        .tests(1_000)
        .quickcheck(prop as fn(Vec<u8>, usize) -> bool);
}

/// On a message that carries no attack, the digest must match an ordinary
/// SHA-1. This compares against a separate implementation, so it covers the
/// rounds, the padding and the length suffix together.
#[test]
fn clean_messages_match_the_sha1_crate() {
    fn prop(data: Vec<u8>) -> bool {
        let want = sha1::Sha1::digest(&data);
        match sha1dc::digest(&data) {
            Ok(got) => got.as_bytes() == &want[..],
            // No generated message carries a collision attack.
            Err(_) => false,
        }
    }
    QuickCheck::new()
        .rng(Gen::new(LONG))
        .tests(1_000)
        .quickcheck(prop as fn(Vec<u8>) -> bool);
}

/// Mitigation only alters a digest when it detects an attack, so a clean
/// message must hash the same in both modes.
#[test]
fn mitigation_leaves_clean_messages_alone() {
    fn prop(data: Vec<u8>) -> bool {
        match (sha1dc::digest(&data), sha1dc::mitigate::digest(&data)) {
            (Ok(plain), Ok(mitigating)) => plain == mitigating,
            _ => false,
        }
    }
    QuickCheck::new()
        .rng(Gen::new(LONG))
        .tests(1_000)
        .quickcheck(prop as fn(Vec<u8>) -> bool);
}

/// `reset` must leave a hasher that behaves like a new one.
#[test]
fn reset_matches_a_fresh_hasher() {
    fn prop(used: Vec<u8>, then: Vec<u8>) -> bool {
        let mut recycled = Hasher::new();
        recycled.update(&used);
        recycled.reset();
        recycled.update(&then);

        recycled.finalize() == sha1dc::digest(&then)
    }
    QuickCheck::new()
        .rng(Gen::new(LONG))
        .tests(1_000)
        .quickcheck(prop as fn(Vec<u8>, Vec<u8>) -> bool);
}
