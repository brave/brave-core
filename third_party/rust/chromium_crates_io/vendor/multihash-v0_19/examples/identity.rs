//! An example for how to use the "identity" hash of [`Multihash`].
//!
//! Identity hashing means we don't actually perform any hashing.
//! Instead, we just store data directly in place of the "digest".
//!
//! [`Multihash::wrap`] returns an error in case the provided digest is too big for the available space.
//! Make sure you construct a [`Multihash`] with a large enough buffer for your data.
//!
//! Typically, the way you want to use the "identity" hash is:
//! 1. Check if your data is smaller than whatever buffer size you chose.
//! 2. If yes, store the data inline.
//! 3. If no, hash it make it fit into the provided buffer.

use multihash::Multihash;

/// See <https://github.com/multiformats/multicodec/blob/master/table.csv#L2> for reference.
const IDENTITY_HASH_CODE: u64 = 0;

fn main() {
    let identity_hash = Multihash::<64>::wrap(IDENTITY_HASH_CODE, b"foobar").unwrap();
    let wrap_err = Multihash::<2>::wrap(IDENTITY_HASH_CODE, b"foobar").unwrap_err();

    assert_eq!(identity_hash.digest(), b"foobar");
    assert_eq!(wrap_err.to_string(), "Invalid multihash size 6.");
}
