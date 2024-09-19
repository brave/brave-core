use std::io::{Cursor, Write};

use multihash::{
    derive::Multihash, Blake2b256, Blake2b512, Blake2s128, Blake2s256, Blake3_256, Hasher,
    Identity256, Keccak224, Keccak256, Keccak384, Keccak512, MultihashDigest, Sha1, Sha2_256,
    Sha2_512, Sha3_224, Sha3_256, Sha3_384, Sha3_512, Strobe256, Strobe512,
};

#[cfg(feature = "ripemd")]
use multihash::{Ripemd160, Ripemd256, Ripemd320};

#[derive(Clone, Copy, Debug, Eq, Multihash, PartialEq)]
#[mh(alloc_size = 64)]
pub enum Code {
    #[mh(code = 0x00, hasher = Identity256)]
    Identity,
    #[mh(code = 0x11, hasher = Sha1)]
    Sha1,
    #[mh(code = 0x12, hasher = Sha2_256)]
    Sha2_256,
    #[mh(code = 0x13, hasher = Sha2_512)]
    Sha2_512,
    #[mh(code = 0x17, hasher = Sha3_224)]
    Sha3_224,
    #[mh(code = 0x16, hasher = Sha3_256)]
    Sha3_256,
    #[mh(code = 0x15, hasher = Sha3_384)]
    Sha3_384,
    #[mh(code = 0x14, hasher = Sha3_512)]
    Sha3_512,
    #[mh(code = 0x1a, hasher = Keccak224)]
    Keccak224,
    #[mh(code = 0x1b, hasher = Keccak256)]
    Keccak256,
    #[mh(code = 0x1c, hasher = Keccak384)]
    Keccak384,
    #[mh(code = 0x1d, hasher = Keccak512)]
    Keccak512,
    #[mh(code = 0xb220, hasher = Blake2b256)]
    Blake2b256,
    #[mh(code = 0xb240, hasher = Blake2b512)]
    Blake2b512,
    #[mh(code = 0xb250, hasher = Blake2s128)]
    Blake2s128,
    #[mh(code = 0xb260, hasher = Blake2s256)]
    Blake2s256,
    #[mh(code = 0x1e, hasher = Blake3_256)]
    Blake3_256,
    #[mh(code = 0x3312e7, hasher = Strobe256)]
    Strobe256,
    #[mh(code = 0x3312e8, hasher = Strobe512)]
    Strobe512,
    #[cfg(feature = "ripemd")]
    #[mh(code = 0x1053, hasher = Ripemd160)]
    Ripemd160,
    #[cfg(feature = "ripemd")]
    #[mh(code = 0x1054, hasher = Ripemd256)]
    Ripemd256,
    #[cfg(feature = "ripemd")]
    #[mh(code = 0x1055, hasher = Ripemd320)]
    Ripemd320,
}

macro_rules! assert_encode {
   // Mutlihash enum member, Multihash code, input, Multihash as hex
   {$( $alg:ty, $code:expr, $data:expr, $expect:expr; )*} => {
       $(
           let expected = hex::decode($expect).unwrap();

           // From code
           assert_eq!(
               $code.digest($data).to_bytes(),
               expected,
               "{:?} encodes correctly (from code)", stringify!($alg)
           );

           // From incremental hashing
           let mut hasher = <$alg>::default();
           hasher.update($data);
           assert_eq!(
               $code.wrap(hasher.finalize()).unwrap().to_bytes(),
               expected,
               "{:?} encodes correctly (from hasher)", stringify!($alg)
           );
       )*
   }
}

#[allow(clippy::cognitive_complexity)]
#[test]
fn multihash_encode() {
    assert_encode! {
        Identity256, Code::Identity, b"beep boop", "00096265657020626f6f70";
        Sha1, Code::Sha1, b"beep boop", "11147c8357577f51d4f0a8d393aa1aaafb28863d9421";
        Sha2_256, Code::Sha2_256, b"helloworld", "1220936a185caaa266bb9cbe981e9e05cb78cd732b0b3280eb944412bb6f8f8f07af";
        Sha2_256, Code::Sha2_256, b"beep boop", "122090ea688e275d580567325032492b597bc77221c62493e76330b85ddda191ef7c";
        Sha2_512, Code::Sha2_512, b"hello world", "1340309ecc489c12d6eb4cc40f50c902f2b4d0ed77ee511a7c7a9bcd3ca86d4cd86f989dd35bc5ff499670da34255b45b0cfd830e81f605dcf7dc5542e93ae9cd76f";
        Sha3_224, Code::Sha3_224, b"hello world", "171Cdfb7f18c77e928bb56faeb2da27291bd790bc1045cde45f3210bb6c5";
        Sha3_256, Code::Sha3_256, b"hello world", "1620644bcc7e564373040999aac89e7622f3ca71fba1d972fd94a31c3bfbf24e3938";
        Sha3_384, Code::Sha3_384, b"hello world", "153083bff28dde1b1bf5810071c6643c08e5b05bdb836effd70b403ea8ea0a634dc4997eb1053aa3593f590f9c63630dd90b";
        Sha3_512, Code::Sha3_512, b"hello world", "1440840006653e9ac9e95117a15c915caab81662918e925de9e004f774ff82d7079a40d4d27b1b372657c61d46d470304c88c788b3a4527ad074d1dccbee5dbaa99a";
        Keccak224, Code::Keccak224, b"hello world", "1A1C25f3ecfebabe99686282f57f5c9e1f18244cfee2813d33f955aae568";
        Keccak256, Code::Keccak256, b"hello world", "1B2047173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad";
        Keccak384, Code::Keccak384, b"hello world", "1C3065fc99339a2a40e99d3c40d695b22f278853ca0f925cde4254bcae5e22ece47e6441f91b6568425adc9d95b0072eb49f";
        Keccak512, Code::Keccak512, b"hello world", "1D403ee2b40047b8060f68c67242175660f4174d0af5c01d47168ec20ed619b0b7c42181f40aa1046f39e2ef9efc6910782a998e0013d172458957957fac9405b67d";
        Blake2b512, Code::Blake2b512, b"hello world", "c0e40240021ced8799296ceca557832ab941a50b4a11f83478cf141f51f933f653ab9fbcc05a037cddbed06e309bf334942c4e58cdf1a46e237911ccd7fcf9787cbc7fd0";
        Blake2s256, Code::Blake2s256, b"hello world", "e0e402209aec6806794561107e594b1f6a8a6b0c92a0cba9acf5e5e93cca06f781813b0b";
        Blake2b256, Code::Blake2b256, b"hello world", "a0e40220256c83b297114d201b30179f3f0ef0cace9783622da5974326b436178aeef610";
        Blake2s128, Code::Blake2s128, b"hello world", "d0e4021037deae0226c30da2ab424a7b8ee14e83";
        Blake3_256, Code::Blake3_256, b"hello world", "1e20d74981efa70a0c880b8d8c1985d075dbcbf679b99a5f9914e5aaf96b831a9e24";
    }

    #[cfg(feature = "ripemd")]
    assert_encode! {
        Ripemd160, Code::Ripemd160, b"hello world", "d3201498c615784ccb5fe5936fbc0cbe9dfdb408d92f0f";
        Ripemd256, Code::Ripemd256, b"hello world", "d420200d375cf9d9ee95a3bb15f757c81e93bb0ad963edf69dc4d12264031814608e37";
        Ripemd320, Code::Ripemd320, b"hello world", "d520280e12fe7d075f8e319e07c106917eddb0135e9a10aefb50a8a07ccb0582ff1fa27b95ed5af57fd5c6";
    }
}

macro_rules! assert_decode {
    {$( $code:expr, $hash:expr; )*} => {
        $(
            let hash = hex::decode($hash).unwrap();
            assert_eq!(
                Multihash::from_bytes(&hash).unwrap().code(),
                u64::from($code),
                "{:?} decodes correctly", stringify!($code)
            );
        )*
    }
}

#[test]
fn assert_decode() {
    assert_decode! {
        Code::Identity, "000a68656c6c6f776f726c64";
        Code::Sha1, "11147c8357577f51d4f0a8d393aa1aaafb28863d9421";
        Code::Sha2_256, "1220936a185caaa266bb9cbe981e9e05cb78cd732b0b3280eb944412bb6f8f8f07af";
        Code::Sha2_256, "122090ea688e275d580567325032492b597bc77221c62493e76330b85ddda191ef7c";
        Code::Sha2_512, "1340309ecc489c12d6eb4cc40f50c902f2b4d0ed77ee511a7c7a9bcd3ca86d4cd86f989dd35bc5ff499670da34255b45b0cfd830e81f605dcf7dc5542e93ae9cd76f";
        Code::Sha3_224, "171Cdfb7f18c77e928bb56faeb2da27291bd790bc1045cde45f3210bb6c5";
        Code::Sha3_256, "1620644bcc7e564373040999aac89e7622f3ca71fba1d972fd94a31c3bfbf24e3938";
        Code::Sha3_384, "153083bff28dde1b1bf5810071c6643c08e5b05bdb836effd70b403ea8ea0a634dc4997eb1053aa3593f590f9c63630dd90b";
        Code::Sha3_512, "1440840006653e9ac9e95117a15c915caab81662918e925de9e004f774ff82d7079a40d4d27b1b372657c61d46d470304c88c788b3a4527ad074d1dccbee5dbaa99a";
        Code::Keccak224, "1A1C25f3ecfebabe99686282f57f5c9e1f18244cfee2813d33f955aae568";
        Code::Keccak256, "1B2047173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad";
        Code::Keccak384, "1C3065fc99339a2a40e99d3c40d695b22f278853ca0f925cde4254bcae5e22ece47e6441f91b6568425adc9d95b0072eb49f";
        Code::Keccak512, "1D403ee2b40047b8060f68c67242175660f4174d0af5c01d47168ec20ed619b0b7c42181f40aa1046f39e2ef9efc6910782a998e0013d172458957957fac9405b67d";
        Code::Blake2b512, "c0e40240021ced8799296ceca557832ab941a50b4a11f83478cf141f51f933f653ab9fbcc05a037cddbed06e309bf334942c4e58cdf1a46e237911ccd7fcf9787cbc7fd0";
        Code::Blake2s256, "e0e402209aec6806794561107e594b1f6a8a6b0c92a0cba9acf5e5e93cca06f781813b0b";
        Code::Blake2b256, "a0e40220256c83b297114d201b30179f3f0ef0cace9783622da5974326b436178aeef610";
        Code::Blake2s128, "d0e4021037deae0226c30da2ab424a7b8ee14e83";
        Code::Blake3_256, "1e20d74981efa70a0c880b8d8c1985d075dbcbf679b99a5f9914e5aaf96b831a9e24";
    }
    #[cfg(feature = "ripemd")]
    assert_decode! {
        Code::Ripemd160, "d3201498c615784ccb5fe5936fbc0cbe9dfdb408d92f0f";
        Code::Ripemd256, "d420200d375cf9d9ee95a3bb15f757c81e93bb0ad963edf69dc4d12264031814608e37";
        Code::Ripemd320, "d520280e12fe7d075f8e319e07c106917eddb0135e9a10aefb50a8a07ccb0582ff1fa27b95ed5af57fd5c6";
    }
}

macro_rules! assert_roundtrip {
    ($( $code:expr, $alg:ident; )*) => {
        $(
            // Hashing with one call
            {
                let hash = $code.digest(b"helloworld");
                assert_eq!(
                    Multihash::from_bytes(&hash.to_bytes()).unwrap().code(),
                    hash.code()
                );
            }
            // Hashing incrementally
            {
                let mut hasher = <$alg>::default();
                hasher.update(b"helloworld");
                let hash = $code.wrap(hasher.finalize()).unwrap();
                assert_eq!(
                    Multihash::from_bytes(&hash.to_bytes()).unwrap().code(),
                    hash.code()
                );
            }
            // Hashing as `Write` implementation
            {
                let mut hasher = <$alg>::default();
                hasher.write_all(b"helloworld").unwrap();
                let hash = $code.wrap(hasher.finalize()).unwrap();
                assert_eq!(
                    Multihash::from_bytes(&hash.to_bytes()).unwrap().code(),
                    hash.code()
                );
            }
        )*
    }
}

#[allow(clippy::cognitive_complexity)]
#[test]
fn assert_roundtrip() {
    assert_roundtrip!(
        Code::Identity, Identity256;
        Code::Sha1, Sha1;
        Code::Sha2_256, Sha2_256;
        Code::Sha2_512, Sha2_512;
        Code::Sha3_224, Sha3_224;
        Code::Sha3_256, Sha3_256;
        Code::Sha3_384, Sha3_384;
        Code::Sha3_512, Sha3_512;
        Code::Keccak224, Keccak224;
        Code::Keccak256, Keccak256;
        Code::Keccak384, Keccak384;
        Code::Keccak512, Keccak512;
        Code::Blake2b512, Blake2b512;
        Code::Blake2s256, Blake2s256;
        Code::Blake3_256, Blake3_256;
    );

    #[cfg(feature = "ripemd")]
    assert_roundtrip! {
        Code::Ripemd160, Ripemd160;
        Code::Ripemd256, Ripemd256;
        Code::Ripemd320, Ripemd320;
    }
}

/// Testing the public interface of `Multihash` and coversions to it
fn multihash_methods<H>(code: Code, prefix: &str, digest_str: &str)
where
    H: Hasher + Default,
{
    let digest = hex::decode(digest_str).unwrap();
    let expected_bytes = hex::decode(&format!("{}{}", prefix, digest_str)).unwrap();
    let mut expected_cursor = Cursor::new(&expected_bytes);
    let multihash = code.digest(b"hello world");

    assert_eq!(Multihash::wrap(code.into(), &digest).unwrap(), multihash);
    assert_eq!(multihash.code(), u64::from(code));
    assert_eq!(multihash.size() as usize, digest.len());
    assert_eq!(multihash.digest(), digest);
    assert_eq!(Multihash::read(&mut expected_cursor).unwrap(), multihash);
    assert_eq!(Multihash::from_bytes(&expected_bytes).unwrap(), multihash);
    let mut written_buf = Vec::new();
    multihash.write(&mut written_buf).unwrap();
    assert_eq!(written_buf, expected_bytes);
    assert_eq!(multihash.to_bytes(), expected_bytes);

    // Test from hasher digest conversion
    let mut hasher = H::default();
    hasher.update(b"hello world");
    let multihash_from_digest = code.wrap(hasher.finalize()).unwrap();
    assert_eq!(multihash_from_digest.code(), u64::from(code));
    assert_eq!(multihash_from_digest.size() as usize, digest.len());
    assert_eq!(multihash_from_digest.digest(), digest);
}

#[test]
fn test_multihash_methods() {
    multihash_methods::<Identity256>(Code::Identity, "000b", "68656c6c6f20776f726c64");
    multihash_methods::<Sha1>(
        Code::Sha1,
        "1114",
        "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed",
    );
    multihash_methods::<Sha2_256>(
        Code::Sha2_256,
        "1220",
        "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9",
    );
    multihash_methods::<Sha2_512, >(
      Code::Sha2_512,
     "1340",
     "309ecc489c12d6eb4cc40f50c902f2b4d0ed77ee511a7c7a9bcd3ca86d4cd86f989dd35bc5ff499670da34255b45b0cfd830e81f605dcf7dc5542e93ae9cd76f");
    multihash_methods::<Sha3_224>(
        Code::Sha3_224,
        "171C",
        "dfb7f18c77e928bb56faeb2da27291bd790bc1045cde45f3210bb6c5",
    );
    multihash_methods::<Sha3_256>(
        Code::Sha3_256,
        "1620",
        "644bcc7e564373040999aac89e7622f3ca71fba1d972fd94a31c3bfbf24e3938",
    );
    multihash_methods::<Sha3_384, >(
     Code::Sha3_384,
     "1530",
     "83bff28dde1b1bf5810071c6643c08e5b05bdb836effd70b403ea8ea0a634dc4997eb1053aa3593f590f9c63630dd90b");
    multihash_methods::<Sha3_512, >(
     Code::Sha3_512,
     "1440",
     "840006653e9ac9e95117a15c915caab81662918e925de9e004f774ff82d7079a40d4d27b1b372657c61d46d470304c88c788b3a4527ad074d1dccbee5dbaa99a");
    multihash_methods::<Keccak224>(
        Code::Keccak224,
        "1A1C",
        "25f3ecfebabe99686282f57f5c9e1f18244cfee2813d33f955aae568",
    );
    multihash_methods::<Keccak256>(
        Code::Keccak256,
        "1B20",
        "47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad",
    );
    multihash_methods::<Keccak384, >(
     Code::Keccak384,
     "1C30",
     "65fc99339a2a40e99d3c40d695b22f278853ca0f925cde4254bcae5e22ece47e6441f91b6568425adc9d95b0072eb49f");
    multihash_methods::<Keccak512, >(
     Code::Keccak512,
     "1D40",
     "3ee2b40047b8060f68c67242175660f4174d0af5c01d47168ec20ed619b0b7c42181f40aa1046f39e2ef9efc6910782a998e0013d172458957957fac9405b67d");
    multihash_methods::<Blake2b512, >(
     Code::Blake2b512,
     "c0e40240",
     "021ced8799296ceca557832ab941a50b4a11f83478cf141f51f933f653ab9fbcc05a037cddbed06e309bf334942c4e58cdf1a46e237911ccd7fcf9787cbc7fd0");
    multihash_methods::<Blake2s256>(
        Code::Blake2s256,
        "e0e40220",
        "9aec6806794561107e594b1f6a8a6b0c92a0cba9acf5e5e93cca06f781813b0b",
    );
    multihash_methods::<Blake2b256>(
        Code::Blake2b256,
        "a0e40220",
        "256c83b297114d201b30179f3f0ef0cace9783622da5974326b436178aeef610",
    );
    multihash_methods::<Blake2s128>(
        Code::Blake2s128,
        "d0e40210",
        "37deae0226c30da2ab424a7b8ee14e83",
    );
    multihash_methods::<Blake3_256>(
        Code::Blake3_256,
        "1e20",
        "d74981efa70a0c880b8d8c1985d075dbcbf679b99a5f9914e5aaf96b831a9e24",
    );
    #[cfg(feature = "ripemd")]
    {
        multihash_methods::<Ripemd160>(
            Code::Ripemd160,
            "d32014",
            "98c615784ccb5fe5936fbc0cbe9dfdb408d92f0f",
        );
        multihash_methods::<Ripemd256>(
            Code::Ripemd256,
            "d42020",
            "0d375cf9d9ee95a3bb15f757c81e93bb0ad963edf69dc4d12264031814608e37",
        );
        multihash_methods::<Ripemd320>(
            Code::Ripemd320,
            "d52028",
            "0e12fe7d075f8e319e07c106917eddb0135e9a10aefb50a8a07ccb0582ff1fa27b95ed5af57fd5c6",
        );
    }
}

#[test]
#[should_panic]
fn test_long_identity_hash() {
    // The identity hash allocates if the input size is bigger than the maximum size
    let input = b"abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz";
    Code::Identity.digest(input);
}

#[test]
fn multihash_errors() {
    assert!(
        Multihash::from_bytes(&[]).is_err(),
        "Should error on empty data"
    );
    assert!(
        Multihash::from_bytes(&[1, 2, 3]).is_err(),
        "Should error on invalid multihash"
    );
    assert!(
        Multihash::from_bytes(&[1, 2, 3]).is_err(),
        "Should error on invalid prefix"
    );
    assert!(
        Multihash::from_bytes(&[0x12, 0x20, 0xff]).is_err(),
        "Should error on correct prefix with wrong digest"
    );
    let identity_code: u8 = 0x00;
    let identity_length = 3;
    assert!(
        Multihash::from_bytes(&[identity_code, identity_length, 1, 2, 3, 4]).is_err(),
        "Should error on wrong hash length"
    );
}

#[test]
fn blak3_non_default_digest() {
    use multihash::Blake3Hasher;
    const DIGEST_SIZE: usize = 16;
    pub struct ContentHasher(Blake3Hasher<DIGEST_SIZE>);

    pub struct ContentHash([u8; DIGEST_SIZE]);

    impl ContentHasher {
        fn new() -> ContentHasher {
            ContentHasher(Blake3Hasher::default())
        }

        fn write(&mut self, input: &[u8]) {
            self.0.update(input);
        }

        fn finish(&mut self) -> ContentHash {
            let hash = multihash::Code::Blake3_256.wrap(self.0.finalize()).unwrap();
            let resized_hash = hash.resize::<DIGEST_SIZE>().unwrap();

            let mut content = ContentHash([0u8; DIGEST_SIZE]);
            content.0.copy_from_slice(resized_hash.digest());
            content
        }

        fn reset(&mut self) {
            self.0.reset();
        }
    }

    let mut hasher = ContentHasher::new();
    hasher.write("foobar".as_bytes());
    let content_hash = hasher.finish();
    hasher.reset();

    let expected = hex::decode("aa51dcd43d5c6c5203ee16906fd6b35d").unwrap();
    assert_eq!(&content_hash.0, expected.as_slice())
}
