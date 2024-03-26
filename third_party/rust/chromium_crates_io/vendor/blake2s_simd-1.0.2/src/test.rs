use super::*;

const EMPTY_HASH: &str = "69217a3079908094e11121d042354a7c1f55b6482ca1a51e1b250dfd1ed0eef9";
const ABC_HASH: &str = "508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982";
const ONE_BLOCK_HASH: &str = "ae09db7cd54f42b490ef09b6bc541af688e4959bb8c53f359a6f56e38ab454a3";
const THOUSAND_HASH: &str = "37e9dd47498579c5343fd282c13c62ea824cdfc9b0f4f747a41347414640f62c";

#[test]
fn test_update_state() {
    let io = &[
        (&b""[..], EMPTY_HASH),
        (&b"abc"[..], ABC_HASH),
        (&[0; BLOCKBYTES], ONE_BLOCK_HASH),
        (&[0; 1000], THOUSAND_HASH),
    ];
    // Test each input all at once.
    for &(input, output) in io {
        let hash = blake2s(input);
        assert_eq!(&hash.to_hex(), output, "hash mismatch");
    }
    // Now in two chunks. This is especially important for the ONE_BLOCK case, because it would be
    // a mistake for update() to call compress, even though the buffer is full.
    for &(input, output) in io {
        let mut state = State::new();
        let split = input.len() / 2;
        state.update(&input[..split]);
        assert_eq!(split as Count, state.count());
        state.update(&input[split..]);
        assert_eq!(input.len() as Count, state.count());
        let hash = state.finalize();
        assert_eq!(&hash.to_hex(), output, "hash mismatch");
    }
    // Now one byte at a time.
    for &(input, output) in io {
        let mut state = State::new();
        let mut count = 0;
        for &b in input {
            state.update(&[b]);
            count += 1;
            assert_eq!(count, state.count());
        }
        let hash = state.finalize();
        assert_eq!(&hash.to_hex(), output, "hash mismatch");
    }
}

#[test]
fn test_multiple_finalizes() {
    let mut state = State::new();
    assert_eq!(&state.finalize().to_hex(), EMPTY_HASH, "hash mismatch");
    assert_eq!(&state.finalize().to_hex(), EMPTY_HASH, "hash mismatch");
    assert_eq!(&state.finalize().to_hex(), EMPTY_HASH, "hash mismatch");
    state.update(b"abc");
    assert_eq!(&state.finalize().to_hex(), ABC_HASH, "hash mismatch");
    assert_eq!(&state.finalize().to_hex(), ABC_HASH, "hash mismatch");
    assert_eq!(&state.finalize().to_hex(), ABC_HASH, "hash mismatch");
}

#[cfg(feature = "std")]
#[test]
fn test_write() {
    use std::io::prelude::*;

    let mut state = State::new();
    state.write_all(&[0; 1000]).unwrap();
    let hash = state.finalize();
    assert_eq!(&hash.to_hex(), THOUSAND_HASH, "hash mismatch");
}

// You can check this case against the equivalent Python:
//
// import hashlib
// hashlib.blake2s(
//     b'foo',
//     digest_size=18,
//     key=b"bar",
//     salt=b"bazbazba",
//     person=b"bing bin",
//     fanout=2,
//     depth=3,
//     leaf_size=0x04050607,
//     node_offset=0x08090a0b0c0d,
//     node_depth=16,
//     inner_size=17,
//     last_node=True,
// ).hexdigest()
#[test]
fn test_all_parameters() {
    let mut params = Params::new();
    params
        .hash_length(18)
        // Make sure a shorter key properly overwrites a longer one.
        .key(b"not the real key")
        .key(b"bar")
        .salt(b"bazbazba")
        .personal(b"bing bin")
        .fanout(2)
        .max_depth(3)
        .max_leaf_length(0x04050607)
        .node_offset(0x08090a0b0c0d)
        .node_depth(16)
        .inner_hash_length(17)
        .last_node(true);

    // Check the State API.
    assert_eq!(
        "62361e5392ab0eb7dd27e48a6809ee82dc57",
        &params.to_state().update(b"foo").finalize().to_hex()
    );

    // Check the all-at-once API.
    assert_eq!(
        "62361e5392ab0eb7dd27e48a6809ee82dc57",
        &params.hash(b"foo").to_hex()
    );
}

#[test]
fn test_all_parameters_blake2sp() {
    let mut params = blake2sp::Params::new();
    params
        .hash_length(18)
        // Make sure a shorter key properly overwrites a longer one.
        .key(b"not the real key")
        .key(b"bar");

    // Check the State API.
    assert_eq!(
        "947d4c671e2794f5e1a57daeca97bb46ed66",
        &params.to_state().update(b"foo").finalize().to_hex()
    );

    // Check the all-at-once API.
    assert_eq!(
        "947d4c671e2794f5e1a57daeca97bb46ed66",
        &params.hash(b"foo").to_hex()
    );
}

#[test]
#[should_panic]
fn test_short_hash_length_panics() {
    Params::new().hash_length(0);
}

#[test]
#[should_panic]
fn test_long_hash_length_panics() {
    Params::new().hash_length(OUTBYTES + 1);
}

#[test]
#[should_panic]
fn test_long_key_panics() {
    Params::new().key(&[0; KEYBYTES + 1]);
}

#[test]
#[should_panic]
fn test_long_salt_panics() {
    Params::new().salt(&[0; SALTBYTES + 1]);
}

#[test]
#[should_panic]
fn test_long_personal_panics() {
    Params::new().personal(&[0; PERSONALBYTES + 1]);
}

#[test]
fn test_zero_max_depth_supported() {
    Params::new().max_depth(0);
}

#[test]
#[should_panic]
fn test_long_inner_hash_length_panics() {
    Params::new().inner_hash_length(OUTBYTES + 1);
}

#[test]
#[should_panic]
fn test_blake2sp_short_hash_length_panics() {
    blake2sp::Params::new().hash_length(0);
}

#[test]
#[should_panic]
fn test_blake2sp_long_hash_length_panics() {
    blake2sp::Params::new().hash_length(OUTBYTES + 1);
}

#[test]
#[should_panic]
fn test_blake2sp_long_key_panics() {
    blake2sp::Params::new().key(&[0; KEYBYTES + 1]);
}

#[test]
fn test_blake2sp_max_offset_ok() {
    Params::new().node_offset((1 << 48) - 1);
}

#[test]
#[should_panic]
fn test_blake2sp_offset_too_large_panics() {
    Params::new().node_offset(1 << 48);
}

#[test]
fn test_hash_from() {
    let h = blake2s(b"foo");
    assert_eq!(h, Hash::from(h.as_array()));
    assert_eq!(h, Hash::from(*h.as_array()));
}
