//! Tests for ChaCha20 (IETF and "djb" versions) as well as XChaCha20
use chacha20::{ChaCha20, ChaCha20Legacy, XChaCha20};

// IETF version of ChaCha20 (96-bit nonce)
cipher::stream_cipher_test!(chacha20_core, "chacha20", ChaCha20);
cipher::stream_cipher_seek_test!(chacha20_seek, ChaCha20);
cipher::stream_cipher_seek_test!(xchacha20_seek, XChaCha20);
cipher::stream_cipher_seek_test!(chacha20legacy_seek, ChaCha20Legacy);

mod chacha20test {
    use chacha20::{ChaCha20, Key, Nonce};
    use cipher::{KeyIvInit, StreamCipher};
    use hex_literal::hex;

    //
    // ChaCha20 test vectors from:
    // <https://datatracker.ietf.org/doc/html/rfc8439#section-2.4.2>
    //

    const KEY: [u8; 32] = hex!("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");

    const IV: [u8; 12] = hex!("000000000000004a00000000");

    const PLAINTEXT: [u8; 114] = hex!(
        "
        4c616469657320616e642047656e746c
        656d656e206f662074686520636c6173
        73206f66202739393a20496620492063
        6f756c64206f6666657220796f75206f
        6e6c79206f6e652074697020666f7220
        746865206675747572652c2073756e73
        637265656e20776f756c642062652069
        742e
        "
    );

    const KEYSTREAM: [u8; 114] = hex!(
        "
        224f51f3401bd9e12fde276fb8631ded8c131f823d2c06
        e27e4fcaec9ef3cf788a3b0aa372600a92b57974cded2b
        9334794cba40c63e34cdea212c4cf07d41b769a6749f3f
        630f4122cafe28ec4dc47e26d4346d70b98c73f3e9c53a
        c40c5945398b6eda1a832c89c167eacd901d7e2bf363
        "
    );

    const CIPHERTEXT: [u8; 114] = hex!(
        "
        6e2e359a2568f98041ba0728dd0d6981
        e97e7aec1d4360c20a27afccfd9fae0b
        f91b65c5524733ab8f593dabcd62b357
        1639d624e65152ab8f530c359f0861d8
        07ca0dbf500d6a6156a38e088a22b65e
        52bc514d16ccf806818ce91ab7793736
        5af90bbf74a35be6b40b8eedf2785e42
        874d
        "
    );

    #[test]
    fn chacha20_keystream() {
        let mut cipher = ChaCha20::new(&Key::from(KEY), &Nonce::from(IV));

        // The test vectors omit the first 64-bytes of the keystream
        let mut prefix = [0u8; 64];
        cipher.apply_keystream(&mut prefix);

        let mut buf = [0u8; 114];
        cipher.apply_keystream(&mut buf);
        assert_eq!(&buf[..], &KEYSTREAM[..]);
    }

    #[test]
    fn chacha20_encryption() {
        let mut cipher = ChaCha20::new(&Key::from(KEY), &Nonce::from(IV));
        let mut buf = PLAINTEXT.clone();

        // The test vectors omit the first 64-bytes of the keystream
        let mut prefix = [0u8; 64];
        cipher.apply_keystream(&mut prefix);

        cipher.apply_keystream(&mut buf);
        assert_eq!(&buf[..], &CIPHERTEXT[..]);
    }
}

#[rustfmt::skip]
mod xchacha20 {
    use chacha20::{Key, XChaCha20, XNonce};
    use cipher::{KeyIvInit, StreamCipher};
    use hex_literal::hex;

    cipher::stream_cipher_seek_test!(xchacha20_seek, XChaCha20);

    //
    // XChaCha20 test vectors from:
    // <https://tools.ietf.org/id/draft-arciszewski-xchacha-03.html#rfc.appendix.A.3.2>
    //

    const KEY: [u8; 32] = hex!("
        808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f
    ");

    const IV: [u8; 24] = hex!("
        404142434445464748494a4b4c4d4e4f5051525354555658
    ");

    const PLAINTEXT: [u8; 304] = hex!("
        5468652064686f6c65202870726f6e6f756e6365642022646f6c652229206973
        20616c736f206b6e6f776e2061732074686520417369617469632077696c6420
        646f672c2072656420646f672c20616e642077686973746c696e6720646f672e
        2049742069732061626f7574207468652073697a65206f662061204765726d61
        6e20736865706865726420627574206c6f6f6b73206d6f7265206c696b652061
        206c6f6e672d6c656767656420666f782e205468697320686967686c7920656c
        757369766520616e6420736b696c6c6564206a756d70657220697320636c6173
        736966696564207769746820776f6c7665732c20636f796f7465732c206a6163
        6b616c732c20616e6420666f78657320696e20746865207461786f6e6f6d6963
        2066616d696c792043616e696461652e
    ");

    const KEYSTREAM: [u8; 304] = hex!("
        29624b4b1b140ace53740e405b2168540fd7d630c1f536fecd722fc3cddba7f4
        cca98cf9e47e5e64d115450f9b125b54449ff76141ca620a1f9cfcab2a1a8a25
        5e766a5266b878846120ea64ad99aa479471e63befcbd37cd1c22a221fe46221
        5cf32c74895bf505863ccddd48f62916dc6521f1ec50a5ae08903aa259d9bf60
        7cd8026fba548604f1b6072d91bc91243a5b845f7fd171b02edc5a0a84cf28dd
        241146bc376e3f48df5e7fee1d11048c190a3d3deb0feb64b42d9c6fdeee290f
        a0e6ae2c26c0249ea8c181f7e2ffd100cbe5fd3c4f8271d62b15330cb8fdcf00
        b3df507ca8c924f7017b7e712d15a2eb5c50484451e54e1b4b995bd8fdd94597
        bb94d7af0b2c04df10ba0890899ed9293a0f55b8bafa999264035f1d4fbe7fe0
        aafa109a62372027e50e10cdfecca127
    ");

    const CIPHERTEXT: [u8; 304] = hex!("
        7d0a2e6b7f7c65a236542630294e063b7ab9b555a5d5149aa21e4ae1e4fbce87
        ecc8e08a8b5e350abe622b2ffa617b202cfad72032a3037e76ffdcdc4376ee05
        3a190d7e46ca1de04144850381b9cb29f051915386b8a710b8ac4d027b8b050f
        7cba5854e028d564e453b8a968824173fc16488b8970cac828f11ae53cabd201
        12f87107df24ee6183d2274fe4c8b1485534ef2c5fbc1ec24bfc3663efaa08bc
        047d29d25043532db8391a8a3d776bf4372a6955827ccb0cdd4af403a7ce4c63
        d595c75a43e045f0cce1f29c8b93bd65afc5974922f214a40b7c402cdb91ae73
        c0b63615cdad0480680f16515a7ace9d39236464328a37743ffc28f4ddb324f4
        d0f5bbdc270c65b1749a6efff1fbaa09536175ccd29fb9e6057b307320d31683
        8a9c71f70b5b5907a66f7ea49aadc409
    ");

    #[test]
    fn xchacha20_keystream() {
        let mut cipher = XChaCha20::new(&Key::from(KEY), &XNonce::from(IV));

        // The test vectors omit the first 64-bytes of the keystream
        let mut prefix = [0u8; 64];
        cipher.apply_keystream(&mut prefix);

        let mut buf = [0u8; 304];
        cipher.apply_keystream(&mut buf);
        assert_eq!(&buf[..], &KEYSTREAM[..]);
    }

    #[test]
    fn xchacha20_encryption() {
        let mut cipher = XChaCha20::new(&Key::from(KEY), &XNonce::from(IV));
        let mut buf = PLAINTEXT.clone();

        // The test vectors omit the first 64-bytes of the keystream
        let mut prefix = [0u8; 64];
        cipher.apply_keystream(&mut prefix);

        cipher.apply_keystream(&mut buf);
        assert_eq!(&buf[..], &CIPHERTEXT[..]);
    }
}

// Legacy "djb" version of ChaCha20 (64-bit nonce)
#[cfg(feature = "legacy")]
#[rustfmt::skip]
mod legacy {
    use chacha20::{ChaCha20Legacy, Key, LegacyNonce};
    use cipher::{NewCipher, StreamCipher, StreamCipherSeek};
    use hex_literal::hex;

    cipher::stream_cipher_test!(chacha20_legacy_core, ChaCha20Legacy, "chacha20-legacy");
    cipher::stream_cipher_seek_test!(chacha20_legacy_seek, ChaCha20Legacy);

    const KEY_LONG: [u8; 32] = hex!("
        0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20
    ");

    const IV_LONG: [u8; 8] = hex!("0301040105090206");

    const EXPECTED_LONG: [u8; 256] = hex!("
        deeb6b9d06dff3e091bf3ad4f4d492b6dd98246f69691802e466e03bad235787
        0f1c6c010b6c2e650c4bf58d2d35c72ab639437069a384e03100078cc1d735a0
        db4e8f474ee6291460fd9197c77ed87b4c64e0d9ac685bd1c56cce021f3819cd
        13f49c9a3053603602582a060e59c2fbee90ab0bf7bb102d819ced03969d3bae
        71034fe598246583336aa744d8168e5dfff5c6d10270f125a4130e719717e783
        c0858b6f7964437173ea1d7556c158bc7a99e74a34d93da6bf72ac9736a215ac
        aefd4ec031f3f13f099e3d811d83a2cf1d544a68d2752409cc6be852b0511a2e
        32f69aa0be91b30981584a1c56ce7546cca24d8cfdfca525d6b15eea83b6b686
    ");

    #[test]
    #[ignore]
    fn chacha20_offsets() {
        for idx in 0..256 {
            for middle in idx..256 {
                for last in middle..256 {
                    let mut cipher =
                        ChaCha20Legacy::new(&Key::from(KEY_LONG), &LegacyNonce::from(IV_LONG));
                    let mut buf = [0; 256];

                    cipher.seek(idx as u64);
                    cipher.apply_keystream(&mut buf[idx..middle]);
                    cipher.apply_keystream(&mut buf[middle..last]);

                    for k in idx..last {
                        assert_eq!(buf[k], EXPECTED_LONG[k])
                    }
                }
            }
        }
    }
}
