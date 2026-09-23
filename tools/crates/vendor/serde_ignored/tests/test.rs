#![allow(
    clippy::derive_partial_eq_without_eq,
    clippy::from_iter_instead_of_collect,
    clippy::zero_sized_map_values
)]

use serde::Deserialize;
use serde_derive::Deserialize;
use std::collections::{BTreeMap as Map, BTreeSet as Set};
use std::iter::FromIterator;

fn assert_ignored<'de, T>(json: &'de str, expected: &[&str]) -> T
where
    T: Deserialize<'de>,
{
    let de = &mut serde_json::Deserializer::from_str(json);

    let mut unused = Set::new();

    let value: T = serde_ignored::deserialize(de, |path| {
        unused.insert(path.to_string());
    })
    .unwrap();

    let expected = Set::from_iter(expected.iter().copied().map(str::to_owned));
    assert_eq!(unused, expected);

    value
}

#[derive(Debug, Deserialize)]
struct V {
    #[allow(dead_code)]
    used: (),
}

#[test]
fn test_readme() {
    #[derive(Debug, PartialEq, Deserialize)]
    struct Package {
        name: String,
        dependencies: Map<String, Dependency>,
    }

    #[derive(Debug, PartialEq, Deserialize)]
    struct Dependency {
        version: String,
    }

    let json = r#"{
        "name": "demo",
        "dependencies": {
            "serde": {
                "version": "1.0",
                "typo1": ""
            }
        },
        "typo2": {
            "inner": ""
        },
        "typo3": {}
    }"#;

    let ignored = &["dependencies.serde.typo1", "typo2", "typo3"];
    let p: Package = assert_ignored(json, ignored);

    let expected = Package {
        name: "demo".to_owned(),
        dependencies: {
            let mut map = Map::new();
            map.insert(
                "serde".to_owned(),
                Dependency {
                    version: "1.0".to_owned(),
                },
            );
            map
        },
    };
    assert_eq!(p, expected);
}

#[test]
fn test_int_key() {
    #[derive(Debug, Deserialize)]
    struct Test {
        #[allow(dead_code)]
        a: Map<usize, V>,
    }

    let json = r#"{
        "a": {
            "2": {
                "used": null,
                "unused": null
            }
        }
    }"#;

    let ignored = &["a.2.unused"];
    assert_ignored::<Test>(json, ignored);
}

#[test]
fn test_newtype_key() {
    type Test = Map<Key, V>;

    #[derive(Debug, Eq, PartialEq, Ord, PartialOrd, Deserialize)]
    struct Key(&'static str);

    let json = r#"{
        "k": {
            "used": null,
            "unused": null
        }
    }"#;

    let ignored = &["k.unused"];
    assert_ignored::<Test>(json, ignored);
}

#[test]
fn test_unit_variant_key() {
    type Test = Map<Key, V>;

    #[derive(Debug, Eq, PartialEq, Ord, PartialOrd, Deserialize)]
    enum Key {
        First,
        Second,
    }

    let json = r#"{
        "First": {
            "used": null,
            "unused": null
        }
    }"#;

    let ignored = &["First.unused"];
    assert_ignored::<Test>(json, ignored);
}
