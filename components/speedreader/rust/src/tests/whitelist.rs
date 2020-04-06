extern crate speedreader;

use std::fs;

#[test]
fn serialization_works() {
    let mut whitelist = speedreader::whitelist::Whitelist::default();
    whitelist.add_configuration(speedreader::SpeedReaderConfig {
        domain: "example.com".to_owned(),
        url_rules: vec![
            "||example.com/news/*".to_owned(),
        ],
        declarative_rewrite: None,
    });
    let serialized = whitelist.serialize();
    assert!(serialized.is_ok());
    let serialized = serialized.unwrap();
    assert!(!serialized.is_empty());

    let new_list = speedreader::whitelist::Whitelist::deserialize(&serialized);
    assert!(new_list.is_ok());
    let new_list = new_list.unwrap();
    assert!(new_list.get_configuration("example.com").is_some());
}

#[test]
fn deserialize_stable_format() {
    let serialized = fs::read("./tests/SpeedReaderConfig.dat").unwrap();
    let maybe_whitelist = speedreader::whitelist::Whitelist::deserialize(&serialized);
    assert!(maybe_whitelist.is_ok());
    let whitelist = maybe_whitelist.unwrap();
    assert!(whitelist.get_configuration("cnn.com").is_some());
}

#[test]
fn deserialize_uncompressed() {
    let serialized = fs::read("./tests/SpeedReaderConfig.json").unwrap();
    let maybe_whitelist = speedreader::whitelist::Whitelist::deserialize(&serialized);
    assert!(maybe_whitelist.is_ok(), "Deserialization errored: {:?}", maybe_whitelist.err());
    let whitelist = maybe_whitelist.unwrap();
    assert!(whitelist.get_configuration("cnn.com").is_some());
}
