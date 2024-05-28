use bip0039::{Count, Language, Mnemonic};

#[cfg(feature = "rand")]
fn generate(language: Language, expected_word_count: Count) {
    let mnemonic = Mnemonic::generate_in(language, expected_word_count);
    let actual_word_count = mnemonic.phrase().split_whitespace().count();
    assert_eq!(actual_word_count, expected_word_count.word_count());
    assert_eq!(mnemonic.to_seed("").len(), 64);
}

#[cfg(feature = "rand")]
#[test]
fn test_generate() {
    for language in Language::all().iter().cloned() {
        generate(language, Count::Words12);
        generate(language, Count::Words15);
        generate(language, Count::Words18);
        generate(language, Count::Words21);
        generate(language, Count::Words24);
    }
}
