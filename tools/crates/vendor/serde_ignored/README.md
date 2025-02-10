# Serde ignored

[<img alt="github" src="https://img.shields.io/badge/github-dtolnay/serde--ignored-8da0cb?style=for-the-badge&labelColor=555555&logo=github" height="20">](https://github.com/dtolnay/serde-ignored)
[<img alt="crates.io" src="https://img.shields.io/crates/v/serde_ignored.svg?style=for-the-badge&color=fc8d62&logo=rust" height="20">](https://crates.io/crates/serde_ignored)
[<img alt="docs.rs" src="https://img.shields.io/badge/docs.rs-serde__ignored-66c2a5?style=for-the-badge&labelColor=555555&logo=docs.rs" height="20">](https://docs.rs/serde_ignored)
[<img alt="build status" src="https://img.shields.io/github/actions/workflow/status/dtolnay/serde-ignored/ci.yml?branch=master&style=for-the-badge" height="20">](https://github.com/dtolnay/serde-ignored/actions?query=branch%3Amaster)

Find out about keys that are ignored when deserializing data. This crate
provides a wrapper that works with any existing Serde `Deserializer` and invokes
a callback on every ignored field.

You can use this to warn users about extraneous keys in a config file, for
example.

Note that if you want unrecognized fields to be an error, consider using the
`#[serde(deny_unknown_fields)]` [attribute] instead.

[attribute]: https://serde.rs/attributes.html

```toml
[dependencies]
serde = "1.0"
serde_ignored = "0.1"
```

```rust
use serde::Deserialize;
use std::collections::{BTreeSet as Set, BTreeMap as Map};

#[derive(Debug, PartialEq, Deserialize)]
struct Package {
    name: String,
    dependencies: Map<String, Dependency>,
}

#[derive(Debug, PartialEq, Deserialize)]
struct Dependency {
    version: String,
}

fn main() {
    let j = r#"{
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

    // Some Deserializer.
    let jd = &mut serde_json::Deserializer::from_str(j);

    // We will build a set of paths to the unused elements.
    let mut unused = Set::new();

    let p: Package = serde_ignored::deserialize(jd, |path| {
        unused.insert(path.to_string());
    }).unwrap();

    // Deserialized as normal.
    println!("{:?}", p);

    // There were three ignored keys.
    let mut expected = Set::new();
    expected.insert("dependencies.serde.typo1".to_owned());
    expected.insert("typo2".to_owned());
    expected.insert("typo3".to_owned());
    assert_eq!(unused, expected);
}
```

<br>

#### License

<sup>
Licensed under either of <a href="LICENSE-APACHE">Apache License, Version
2.0</a> or <a href="LICENSE-MIT">MIT license</a> at your option.
</sup>

<br>

<sub>
Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in this crate by you, as defined in the Apache-2.0 license, shall
be dual licensed as above, without any additional terms or conditions.
</sub>
