use hkdf::Hkdf;
use sha2::Sha256;

// If you already have a strong key to work from (uniformly-distributed and
// long enough), you can save a tiny amount of time by skipping the extract
// step. In this case, you pass a Pseudo-Random Key (PRK) into the `from_prk`
// constructor, then use the resulting `Hkdf` object as usual.

fn main() {
    // data from RFC 5869, section A.1, Test Case 1
    let prk =
        hex::decode("077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5").unwrap();
    let info = hex::decode("f0f1f2f3f4f5f6f7f8f9").unwrap();

    let hk = Hkdf::<Sha256>::from_prk(&prk).expect("PRK should be large enough");
    let mut okm = [0u8; 42];
    hk.expand(&info, &mut okm)
        .expect("42 is a valid length for Sha256 to output");

    println!("Vector 1 OKM is {}", hex::encode(&okm[..]));
    println!("Matched with https://tools.ietf.org/html/rfc5869#appendix-A.1");

    let expected =
        "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865";
    assert_eq!(hex::encode(&okm[..]), expected);
}
