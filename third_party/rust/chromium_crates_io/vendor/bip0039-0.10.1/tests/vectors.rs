use bip0039::{Language, Mnemonic};
use serde::Deserialize;
use unicode_normalization::UnicodeNormalization;

#[derive(Debug, Deserialize)]
#[serde(rename_all = "lowercase")]
struct Case {
    entropy: String,
    mnemonic: String,
    passphrase: String,
    seed: String,
    #[allow(dead_code)]
    bip32_xprv: String,
}

#[test]
fn test_all_vectors() {
    // https://github.com/bip32JP/bip32JP.github.io/blob/master/test_EN_BIP39.json
    // The passphrase "TREZOR" is used for all vectors.
    let en_cases = serde_json::from_str::<Vec<Case>>(include_str!("./test_EN_BIP39.json")).unwrap();
    for Case {
        entropy,
        mnemonic,
        passphrase,
        seed,
        ..
    } in en_cases
    {
        test_mnemonic(Language::English, &passphrase, &entropy, &mnemonic, &seed);
    }

    // https://github.com/bip32JP/bip32JP.github.io/blob/master/test_JP_BIP39.json
    // Japanese wordlist test with heavily normalized symbols as passphrase
    let jp_cases = serde_json::from_str::<Vec<Case>>(include_str!("./test_JP_BIP39.json")).unwrap();
    for Case {
        entropy,
        mnemonic,
        passphrase,
        seed,
        ..
    } in jp_cases
    {
        test_mnemonic(Language::Japanese, &passphrase, &entropy, &mnemonic, &seed);
    }
}

fn test_mnemonic(
    lang: Language,
    passphrase: &str,
    entropy_hex: &str,
    expected_phrase: &str,
    expected_seed_hex: &str,
) {
    let entropy = hex::decode(entropy_hex).unwrap();
    let mnemonic = Mnemonic::from_entropy_in(lang, entropy).unwrap();
    assert_eq!(mnemonic.phrase(), expected_phrase.nfkd().to_string());
    assert!(Mnemonic::from_phrase_in(lang, expected_phrase).is_ok());

    let seed = mnemonic.to_seed(passphrase);
    assert_eq!(hex::encode(&seed[..]), expected_seed_hex);
}
