//! Detection tests against the known SHA-1 collisions.
//!
//! The published crate does not contain the `shattered` PDFs. Those tests
//! skip themselves if the files are absent.

use hex_literal::hex;
use sha1dc::Hasher;

#[test]
fn shambles_1() {
    collision_test(
        "sha-mbles-1.bin",
        hex!("8ac60ba76f1999a1ab70223f225aefdc78d4ddc0"),
        hex!("4f3d9be4a472c4dae83c6314aa6c36a064c1fd14"),
        None,
        false,
        false,
    );
}

#[test]
fn shambles_2() {
    collision_test(
        "sha-mbles-2.bin",
        hex!("8ac60ba76f1999a1ab70223f225aefdc78d4ddc0"),
        hex!("9ed5d77a4f48be1dbf3e9e15650733eb850897f2"),
        None,
        false,
        false,
    );
}

#[test]
fn shattered_1() {
    collision_test(
        "shattered-1.pdf",
        hex!("38762cf7f55934b34d179ae6a4c80cadccbb7f0a"),
        hex!("16e96b70000dd1e7c85b8368ee197754400e58ec"),
        Some(hex!("d3a1d09969c3b57113fd17b23e01dd3de74a99bb")),
        false,
        true,
    );
}

#[test]
fn shattered_2() {
    collision_test(
        "shattered-2.pdf",
        hex!("38762cf7f55934b34d179ae6a4c80cadccbb7f0a"),
        hex!("e1761773e6a35916d99f891b77663e6405313587"),
        Some(hex!("92246b0b718f4c704d37bb025717cbc66babf102")),
        false,
        true,
    );
}

#[test]
fn reduced_sha_collision() {
    collision_test(
        "sha1_reducedsha_coll.bin",
        hex!("a56374e1cf4c3746499bc7c0acb39498ad2ee185"),
        hex!("dd39885a2a5d8f59030b451e00cb45da9f9d3828"),
        Some(hex!("dd39885a2a5d8f59030b451e00cb45da9f9d3828")),
        true,
        false,
    );
}

/// The `shattered` PDFs collide under plain SHA-1. They must not collide when
/// the hasher mitigates.
#[test]
fn mitigation_breaks_the_collision() {
    let (Some(one), Some(two)) = (read("shattered-1.pdf"), read("shattered-2.pdf")) else {
        eprintln!("SKIPPING TEST, data not available");
        return;
    };

    // The two files have the same SHA-1 digest. With mitigation they must not.
    let plain = |data: &[u8]| sha1dc::digest(data).unwrap_err().digest();

    assert_eq!(plain(&one), plain(&two), "known SHA-1 collision");
    let mitigated = |data: &[u8]| sha1dc::mitigate::digest(data).unwrap_err().digest();
    assert_ne!(mitigated(&one), mitigated(&two));
}

/// Detection applies to one message, so the hasher must work again after it.
#[test]
fn reset_clears_detected_collision() {
    let Some(data) = read("sha-mbles-1.bin") else {
        return;
    };

    let mut hasher = Hasher::new();
    hasher.update(&data);
    assert!(hasher.finalize_reset().is_err());
    assert!(!hasher.collision_detected());

    hasher.update(b"abc");
    assert_eq!(
        hasher.finalize().unwrap(),
        hex!("a9993e364706816aba3e25717850c26c9cd0d89d")
    );
}

fn read(name: &str) -> Option<Vec<u8>> {
    let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("tests/data")
        .join(name);
    std::fs::read(path).ok()
}

fn collision_test(
    name: &str,
    hash: [u8; 20],
    mitigated_hash: [u8; 20],
    reduced_rounds_mitigated: Option<[u8; 20]>,
    reduced_rounds: bool,
    allow_skip: bool,
) {
    let Some(input) = read(name) else {
        assert!(allow_skip, "missing test data: {name}");
        eprintln!("SKIPPING TEST, data not available");
        return;
    };

    // Detection without mitigation. Reports the collision and returns plain
    // SHA-1.
    let mut hasher = Hasher::builder()
        .internal_reduced_round_collisions(reduced_rounds)
        .build();
    hasher.update(&input);
    assert_eq!(hasher.finalize().unwrap_err().digest(), hash);

    // The same, with the UBC optimization off. The result must not change.
    let mut hasher = Hasher::builder()
        .internal_use_ubc(false)
        .internal_reduced_round_collisions(reduced_rounds)
        .build();
    hasher.update(&input);
    assert_eq!(hasher.finalize().unwrap_err().digest(), hash);

    // With mitigation. Reports the collision and returns a digest that is
    // different from plain SHA-1.
    let mut hasher = Hasher::builder()
        .internal_reduced_round_collisions(reduced_rounds)
        .build_mitigating();
    hasher.update(&input);
    assert_eq!(hasher.finalize().unwrap_err().digest(), mitigated_hash);

    // The mitigating one-shot agrees, but only for a collision that the
    // default configuration detects. That excludes the reduced-round one.
    if !reduced_rounds {
        assert_eq!(
            sha1dc::mitigate::digest(&input).unwrap_err().digest(),
            mitigated_hash
        );
    }

    if let Some(expected) = reduced_rounds_mitigated {
        let mut hasher = Hasher::builder()
            .internal_reduced_round_collisions(true)
            .build_mitigating();
        hasher.update(&input);
        assert_eq!(hasher.finalize().unwrap_err().digest(), expected);
    }

    // A split of the input across `update` calls must not change detection.
    for chunk in [1, 17, 64, 65, 1000] {
        let mut hasher = Hasher::builder()
            .internal_reduced_round_collisions(reduced_rounds)
            .build_mitigating();
        for part in input.chunks(chunk) {
            hasher.update(part);
        }
        assert_eq!(
            hasher.finalize().unwrap_err().digest(),
            mitigated_hash,
            "chunk size {chunk}"
        );
    }
}
