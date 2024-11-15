use multihash_derive::{Hasher, MultihashDigest};

#[test]
fn ui() {
    let t = trybuild::TestCases::new();
    t.pass("tests/pass/*.rs");
    t.compile_fail("tests/fail/*.rs");
}

#[test]
fn uses_correct_hasher() {
    #[derive(Clone, Debug, Eq, PartialEq, Copy, MultihashDigest)]
    #[mh(alloc_size = 32)]
    pub enum Code {
        /// Multihash array for hash function.
        #[mh(code = 0x38b64f, hasher = multihash_codetable::Strobe256)]
        Strobe256,
    }

    let multihash1 = Code::Strobe256.digest(b"foobar");

    let mut hasher = multihash_codetable::Strobe256::default();
    hasher.update(b"foobar");
    let digest = hasher.finalize();

    let multihash2 = Multihash::wrap(0x38b64f, digest).unwrap();

    assert_eq!(multihash1, multihash2)
}
