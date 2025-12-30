use digest::dev::{feed_rand_16mib, fixed_reset_test};
use hex_literal::hex;
use sha1_checked::{Digest, Sha1};

digest::new_test!(sha1_checked_main, "sha1", Sha1, fixed_reset_test);

#[test]
fn sha1_rand() {
    let mut h = Sha1::new();
    feed_rand_16mib(&mut h);
    assert_eq!(
        &h.finalize()[..],
        hex!("7e565a25a8b123e9881addbcedcd927b23377a78"),
    );
}

#[test]
fn sha1_collision_rand() {
    let mut h = Sha1::new();
    feed_rand_16mib(&mut h);
    assert_eq!(
        &h.finalize()[..],
        hex!("7e565a25a8b123e9881addbcedcd927b23377a78"),
    );
}

#[cfg(feature = "std")]
#[test]
fn shambles_1() {
    collision_test(
        "./data/sha-mbles-1.bin",
        hex!("8ac60ba76f1999a1ab70223f225aefdc78d4ddc0"),
        hex!("4f3d9be4a472c4dae83c6314aa6c36a064c1fd14"),
        None,
        false,
        false,
    );
}

#[cfg(feature = "std")]
#[test]
fn shambles_2() {
    collision_test(
        "./data/sha-mbles-2.bin",
        hex!("8ac60ba76f1999a1ab70223f225aefdc78d4ddc0"),
        hex!("9ed5d77a4f48be1dbf3e9e15650733eb850897f2"),
        None,
        false,
        false,
    );
}

#[cfg(feature = "std")]
#[test]
fn shattered_1() {
    collision_test(
        "./data/shattered-1.pdf",
        hex!("38762cf7f55934b34d179ae6a4c80cadccbb7f0a"),
        hex!("16e96b70000dd1e7c85b8368ee197754400e58ec"),
        Some(hex!("d3a1d09969c3b57113fd17b23e01dd3de74a99bb")),
        false,
        true,
    );
}

#[cfg(feature = "std")]
#[test]
fn shattered_2() {
    collision_test(
        "./data/shattered-2.pdf",
        hex!("38762cf7f55934b34d179ae6a4c80cadccbb7f0a"),
        hex!("e1761773e6a35916d99f891b77663e6405313587"),
        Some(hex!("92246b0b718f4c704d37bb025717cbc66babf102")),
        false,
        true,
    );
}

#[cfg(feature = "std")]
#[test]
fn reducedsha_coll() {
    collision_test(
        "./data/sha1_reducedsha_coll.bin",
        hex!("a56374e1cf4c3746499bc7c0acb39498ad2ee185"),
        hex!("dd39885a2a5d8f59030b451e00cb45da9f9d3828"),
        Some(hex!("dd39885a2a5d8f59030b451e00cb45da9f9d3828")),
        true,
        false,
    );
}

#[cfg(feature = "std")]
fn collision_test(
    input_path: &str,
    hash: [u8; 20],
    mitigated_hash: [u8; 20],
    reduced_rounds_mitigated: Option<[u8; 20]>,
    reduced_rounds: bool,
    allow_skip: bool,
) {
    let p = std::env::current_dir()
        .unwrap()
        .join("tests")
        .join(input_path);

    if !p.exists() && allow_skip {
        eprintln!("SKIPPING TEST, data not available");
        return;
    }

    let input = std::fs::read(p).unwrap();
    let has_collision = true;

    // No detection.
    let mut ctx = Sha1::builder().detect_collision(false).build();
    ctx.update(&input);
    let d = ctx.try_finalize();
    assert!(!d.has_collision());
    assert_eq!(&d.hash()[..], hash,);

    // No mitigation.
    let mut ctx = Sha1::builder()
        .safe_hash(false)
        .reduced_round_collision(reduced_rounds)
        .build();
    ctx.update(&input);

    let d = ctx.try_finalize();
    assert_eq!(d.has_collision(), has_collision);
    assert_eq!(&d.hash()[..], hash);

    // No mitigation, no optimization.
    let mut ctx = Sha1::builder()
        .safe_hash(false)
        .use_ubc(false)
        .reduced_round_collision(reduced_rounds)
        .build();
    ctx.update(&input);
    let d = ctx.try_finalize();
    assert_eq!(d.has_collision(), has_collision);
    assert_eq!(&d.hash()[..], hash);

    // With mitigation.
    let mut ctx = Sha1::builder()
        .reduced_round_collision(reduced_rounds)
        .build();
    ctx.update(&input);
    let d = ctx.try_finalize();
    assert_eq!(d.has_collision(), has_collision);
    assert_eq!(&d.hash()[..], mitigated_hash);

    if let Some(rr) = reduced_rounds_mitigated {
        let mut ctx = Sha1::builder().reduced_round_collision(true).build();
        ctx.update(&input);
        let d = ctx.try_finalize();
        assert_eq!(d.has_collision(), has_collision);
        assert_eq!(&d.hash()[..], rr);
    }
}
