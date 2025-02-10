//! Lockfile integration test

use std::str::FromStr;

// TODO(tarcieri): add more example `Cargo.lock` files which cover more scenarios

use cargo_lock::{
    package::{GitReference, SourceKind},
    Lockfile, MetadataKey, ResolveVersion, Version,
};

/// Path to a V1 `Cargo.lock` file.
const V1_LOCKFILE_PATH: &str = "tests/examples/Cargo.lock.v1";

/// Path to a V2 `Cargo.lock` file.
const V2_LOCKFILE_PATH: &str = "tests/examples/Cargo.lock.v2";

/// Path to a V3 `Cargo.lock` file.
const V3_LOCKFILE_PATH: &str = "tests/examples/Cargo.lock.v3";

/// Path to a V4 `Cargo.lock` file.
const V4_LOCKFILE_PATH: &str = "tests/examples/Cargo.lock.v4";

/// Load example V1 `Cargo.lock` file (from the Cargo project itself)
#[test]
fn load_example_v1_lockfile() {
    let lockfile = Lockfile::load(V1_LOCKFILE_PATH).unwrap();

    assert_eq!(lockfile.version, ResolveVersion::V1);
    assert_eq!(lockfile.packages.len(), 141);
    assert_eq!(lockfile.metadata.len(), 136);

    let package = &lockfile.packages[0];
    assert_eq!(package.name.as_ref(), "adler32");
    assert_eq!(package.version, Version::parse("1.0.4").unwrap());

    let metadata_key: MetadataKey =
        "checksum adler32 1.0.4 (registry+https://github.com/rust-lang/crates.io-index)"
            .parse()
            .unwrap();

    let metadata_value = &lockfile.metadata[&metadata_key];
    assert_eq!(
        metadata_value.as_ref(),
        "5d2e7343e7fc9de883d1b0341e0b13970f764c14101234857d2ddafa1cb1cac2"
    );
}

/// Load example V2 `Cargo.lock` file
#[test]
fn load_example_v2_lockfile() {
    let lockfile = Lockfile::load(V2_LOCKFILE_PATH).unwrap();
    assert_eq!(lockfile.version, ResolveVersion::V2);
    assert_eq!(lockfile.packages.len(), 25);
    assert_eq!(lockfile.metadata.len(), 0);
}

/// Load example V3 `Cargo.lock` file
#[test]
fn load_example_v3_lockfile() {
    let lockfile = Lockfile::load(V3_LOCKFILE_PATH).unwrap();
    assert_eq!(lockfile.version, ResolveVersion::V3);
    assert_eq!(lockfile.packages.len(), 25);
    assert_eq!(lockfile.metadata.len(), 0);
}

/// Ensure V3 lockfiles encode their version correctly.
#[test]
fn serialize_v3() {
    let lockfile = Lockfile::load(V3_LOCKFILE_PATH).unwrap();
    let reserialized = lockfile.to_string();
    let lockfile2 = reserialized.parse::<Lockfile>().unwrap();
    assert_eq!(lockfile2.version, ResolveVersion::V3);
    assert_eq!(lockfile2.packages, lockfile.packages);
}

/// Load example V4 `Cargo.lock` file
#[test]
fn load_example_v4_lockfile() {
    let lockfile = Lockfile::load(V4_LOCKFILE_PATH).unwrap();
    assert_eq!(lockfile.version, ResolveVersion::V4);
    assert_eq!(lockfile.packages.len(), 25);
    assert_eq!(lockfile.metadata.len(), 0);

    let source_kind = lockfile
        .packages
        .iter()
        .find(|pkg| pkg.name.as_str() == "url")
        .and_then(|pkg| pkg.source.as_ref())
        .map(|id| id.kind())
        .unwrap();
    assert_eq!(
        source_kind,
        &SourceKind::Git(GitReference::Tag("a-_+#$)z".into()))
    );

    let source_kind = lockfile
        .packages
        .iter()
        .find(|pkg| pkg.name.as_str() == "toml")
        .and_then(|pkg| pkg.source.as_ref())
        .map(|id| id.kind())
        .unwrap();
    assert_eq!(
        source_kind,
        &SourceKind::Git(GitReference::Branch("a-_+#$)z".into()))
    );
}

/// Ensure V4 lockfiles encode their version correctly.
#[test]
fn serialize_v4() {
    let lockfile = Lockfile::load(V4_LOCKFILE_PATH).unwrap();
    let reserialized = lockfile.to_string();
    let lockfile2 = reserialized.parse::<Lockfile>().unwrap();
    assert_eq!(lockfile2.version, ResolveVersion::V4);
    assert_eq!(lockfile2.packages, lockfile.packages);
}

/// Ensure we can serialize a V2 lockfile as a V1 lockfile
#[test]
fn serialize_v2_to_v1() {
    let mut lockfile = Lockfile::load(V2_LOCKFILE_PATH).unwrap();
    lockfile.version = ResolveVersion::V1;

    let reserialized = lockfile.to_string();
    let lockfile2 = reserialized.parse::<Lockfile>().unwrap();
    assert_eq!(lockfile2.version, ResolveVersion::V1);
    assert_eq!(lockfile2.packages, lockfile.packages);
}

/// Ensure we can serialize a V1 lockfile as a V2 lockfile
#[test]
fn serialize_v1_to_v2() {
    let mut lockfile = Lockfile::load(V1_LOCKFILE_PATH).unwrap();
    lockfile.version = ResolveVersion::V2;

    let reserialized = lockfile.to_string();
    let lockfile2 = reserialized.parse::<Lockfile>().unwrap();
    assert_eq!(lockfile.packages, lockfile2.packages);
}

/// Test that encoded V1 lockfiles match what Cargo would normally write.
#[test]
fn serde_matches_v1() {
    let lockfile = Lockfile::load(V1_LOCKFILE_PATH).unwrap();
    let reserialized = lockfile.to_string();
    let file_content = std::fs::read_to_string(V1_LOCKFILE_PATH).unwrap();

    assert_eq!(reserialized, file_content);
}

/// Test that encoded V2 lockfiles match what Cargo would normally write.
#[test]
fn serde_matches_v2() {
    let lockfile = Lockfile::load(V2_LOCKFILE_PATH).unwrap();
    let reserialized = lockfile.to_string();
    let file_content = std::fs::read_to_string(V2_LOCKFILE_PATH).unwrap();

    assert_eq!(reserialized, file_content);
}

/// Test that encoded V3 lockfiles match what Cargo would normally write.
#[test]
fn serde_matches_v3() {
    let lockfile = Lockfile::load(V3_LOCKFILE_PATH).unwrap();
    let reserialized = lockfile.to_string();
    let file_content = std::fs::read_to_string(V3_LOCKFILE_PATH).unwrap();

    assert_eq!(reserialized, file_content);
}

/// Dependency tree tests
#[cfg(feature = "dependency-tree")]
mod tree {
    use super::{Lockfile, V1_LOCKFILE_PATH, V2_LOCKFILE_PATH};

    /// Compute a dependency graph from a non-trivial example V1 `Cargo.lock`
    #[test]
    fn compute_from_v1_example_lockfile() {
        let tree = Lockfile::load(V1_LOCKFILE_PATH)
            .unwrap()
            .dependency_tree()
            .unwrap();

        assert_eq!(tree.nodes().len(), 141);
    }

    /// Compute a dependency graph from a non-trivial example V2 `Cargo.lock`
    #[test]
    fn compute_from_v2_example_lockfile() {
        let tree = Lockfile::load(V2_LOCKFILE_PATH)
            .unwrap()
            .dependency_tree()
            .unwrap();

        assert_eq!(tree.nodes().len(), 25);
    }
}

/// Test that a lockfile with ambiguous registries have registries encoded for each package which has multiple
#[test]
fn encoding_multi_registry() {
    let lockfile = cargo_lock::Lockfile::from_str(
        r#"# This file is automatically @generated by Cargo.
# It is not intended for manual editing.
version = 3 

[[package]]
name = "bytes"
version = "0.6.0"
source = "registry+https://github.com/rust-lang/alternate-index"
checksum = "e0dcbc35f504eb6fc275a6d20e4ebcda18cf50d40ba6fabff8c711fa16cb3b16"

[[package]]
name = "bytes"
version = "1.2.1"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "ec8a7b6a70fde80372154c65702f00a0f56f3e1c36abbc6c440484be248856db"

[[package]]
name = "bytestring"
version = "1.1.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "86b6a75fd3048808ef06af5cd79712be8111960adaf89d90250974b38fc3928a"
dependencies = [ 
 "bytes 1.2.1",
 "external 1.0.0",
]

[[package]]
name = "example"
version = "0.1.0"
dependencies = [ 
 "bytes 0.6.0",
 "bytestring",
 "external 2.0.0",
]

[[package]]
name = "external"
version = "1.0.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "30d69d242d4c4bc978b19d6c5f254cfb61ae3679c4656f528c9992fe337e45a6"

[[package]]
name = "external"
version = "2.0.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "3b5e0d8097cb9529731750ba339ea6813275b868779461ba1d39b841641386d9"
"#,
    )
    .unwrap()
    .to_string();

    let expected = r#"# This file is automatically @generated by Cargo.
# It is not intended for manual editing.
version = 3

[[package]]
name = "bytes"
version = "0.6.0"
source = "registry+https://github.com/rust-lang/alternate-index"
checksum = "e0dcbc35f504eb6fc275a6d20e4ebcda18cf50d40ba6fabff8c711fa16cb3b16"

[[package]]
name = "bytes"
version = "1.2.1"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "ec8a7b6a70fde80372154c65702f00a0f56f3e1c36abbc6c440484be248856db"

[[package]]
name = "bytestring"
version = "1.1.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "86b6a75fd3048808ef06af5cd79712be8111960adaf89d90250974b38fc3928a"
dependencies = [
 "bytes 1.2.1 (registry+https://github.com/rust-lang/crates.io-index)",
 "external 1.0.0",
]

[[package]]
name = "example"
version = "0.1.0"
dependencies = [
 "bytes 0.6.0 (registry+https://github.com/rust-lang/alternate-index)",
 "bytestring",
 "external 2.0.0",
]

[[package]]
name = "external"
version = "1.0.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "30d69d242d4c4bc978b19d6c5f254cfb61ae3679c4656f528c9992fe337e45a6"

[[package]]
name = "external"
version = "2.0.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "3b5e0d8097cb9529731750ba339ea6813275b868779461ba1d39b841641386d9"
"#;

    assert_eq!(expected, lockfile);
}

/// Test that a lockfile with consistent registries are able to skip being encoded
#[test]
fn encoding_unified_registry() {
    let lockfile = r#"# This file is automatically @generated by Cargo.
# It is not intended for manual editing.
version = 3

[[package]]
name = "bytes"
version = "0.6.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "e0dcbc35f504eb6fc275a6d20e4ebcda18cf50d40ba6fabff8c711fa16cb3b16"

[[package]]
name = "bytes"
version = "1.2.1"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "ec8a7b6a70fde80372154c65702f00a0f56f3e1c36abbc6c440484be248856db"

[[package]]
name = "bytestring"
version = "1.1.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "86b6a75fd3048808ef06af5cd79712be8111960adaf89d90250974b38fc3928a"
dependencies = [
 "bytes 1.2.1",
]

[[package]]
name = "example"
version = "0.1.0"
dependencies = [
 "bytes 0.6.0",
 "bytestring",
]
"#;

    assert_eq!(
        lockfile,
        cargo_lock::Lockfile::from_str(lockfile)
            .unwrap()
            .to_string(),
    );
}

/// Test that a lockfile with git sources are correctly encoded
#[test]
fn encoding_registry_and_git() {
    let lockfile = r#"# This file is automatically @generated by Cargo.
# It is not intended for manual editing.
version = 3

[[package]]
name = "tower-buffer"
version = "0.3.0"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "c4887dc2a65d464c8b9b66e0e4d51c2fd6cf5b3373afc72805b0a60bce00446a"
dependencies = [
 "tracing 0.1.35",
]

[[package]]
name = "tracing"
version = "0.1.35"
source = "registry+https://github.com/rust-lang/crates.io-index"
checksum = "a400e31aa60b9d44a52a8ee0343b5b18566b03a8321e0d321f695cf56e940160"

[[package]]
name = "tracing"
version = "0.2.0"
source = "git+https://github.com/tokio-rs/tracing.git?rev=1e09e50e8d15580b5929adbade9c782a6833e4a0#1e09e50e8d15580b5929adbade9c782a6833e4a0"

[[package]]
name = "example"
version = "0.1.0"
dependencies = [
 "tower-buffer",
 "tracing 0.2.0",
]
"#;

    assert_eq!(
        lockfile,
        cargo_lock::Lockfile::from_str(lockfile)
            .unwrap()
            .to_string(),
    );
}

#[cfg(feature = "dependency-tree")]
#[test]
fn hash_fragment_dep() {
    let lockfile_str = r#"# This file is automatically @generated by Cargo.
# It is not intended for manual editing.
version = 3

[[package]]
name = "parent"
version = "0.1.0"
source = "git+https://github.com/nope/parent?rev=xxxx#xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
dependencies = [
 "child 0.1.0 (git+https://github.com/nope/child?rev=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)",
 "child 0.1.0 (git+https://github.com/nope/child?rev=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb)",
]

[[package]]
name = "child"
version = "0.1.0"
source = "git+https://github.com/nope/child?rev=aaaa#aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"

[[package]]
name = "child"
version = "0.1.0"
source = "git+https://github.com/nope/child?rev=bbbb#bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
"#;
    let lockfile = cargo_lock::Lockfile::from_str(lockfile_str).unwrap();
    assert_eq!(lockfile_str, lockfile.to_string(),);
    // This will fail to resolve if child source URLs aren't normalized when deriving
    // dependencies from packges.
    let _tree = cargo_lock::dependency::tree::Tree::new(&lockfile).unwrap();
}
