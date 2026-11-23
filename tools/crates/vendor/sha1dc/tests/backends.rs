//! The scalar implementation and the CPU's SHA-1 instructions must agree.
//!
//! On a machine with the instructions, the scalar path never runs. But every
//! target without a hardware backend uses it. This test selects it and
//! compares the two paths against *each other*. That is stronger than two runs
//! of the suite against the same constants, because a difference between the
//! paths appears directly.

use sha1dc::{Collision, Digest, Hasher};

fn scalar() -> Hasher {
    Hasher::builder().internal_scalar_backend().build()
}

/// Hashes with both backends and returns the result that they agree on.
#[track_caller]
fn both(data: &[u8]) -> Result<Digest, Collision> {
    let mut auto = Hasher::new();
    auto.update(data);
    let auto = auto.finalize();

    let mut soft = scalar();
    soft.update(data);
    let soft = soft.finalize();

    match (&auto, &soft) {
        (Ok(a), Ok(s)) => assert_eq!(a, s, "backends disagree on a clean digest"),
        (Err(a), Err(s)) => assert_eq!(a, s, "backends disagree on a collision"),
        _ => panic!("one backend flagged a collision and the other did not"),
    }
    auto
}

/// Every length over several blocks. This covers the padding and the block
/// loop.
#[test]
fn backends_agree_on_every_length() {
    for n in 0..=600 {
        let data: Vec<u8> = (0..n).map(|i| ((i * 31 + n * 7) % 256) as u8).collect();
        both(&data).expect("no collision");
    }
}

/// A real collision runs `ensure_states` and the DV loop. The two backends
/// differ there.
#[test]
fn backends_agree_on_the_known_collisions() {
    for name in [
        "sha-mbles-1.bin",
        "sha-mbles-2.bin",
        "sha1_reducedsha_coll.bin",
        "shattered-1.pdf",
        "shattered-2.pdf",
    ] {
        let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("tests/data")
            .join(name);
        let Ok(data) = std::fs::read(path) else {
            continue; // the shattered PDFs are excluded from the published crate
        };
        let result = both(&data);
        if name != "sha1_reducedsha_coll.bin" {
            assert!(result.is_err(), "{name} should be flagged");
        }
    }
}

/// The other builder options must also apply to the scalar path.
#[test]
fn backends_agree_with_detection_options() {
    let Ok(data) = std::fs::read(
        std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/data/sha-mbles-1.bin"),
    ) else {
        return;
    };

    for use_ubc in [true, false] {
        // Both hasher types, because they use different paths on detection.
        let mitigating = |scalar: bool| {
            let mut b = Hasher::builder().internal_use_ubc(use_ubc);
            if scalar {
                b = b.internal_scalar_backend();
            }
            let mut h = b.build_mitigating();
            h.update(&data);
            h.finalize()
        };
        assert_eq!(
            mitigating(false).unwrap_err(),
            mitigating(true).unwrap_err(),
            "mitigating, ubc={use_ubc}"
        );

        let plain = |scalar: bool| {
            let mut b = Hasher::builder().internal_use_ubc(use_ubc);
            if scalar {
                b = b.internal_scalar_backend();
            }
            let mut h = b.build();
            h.update(&data);
            h.finalize()
        };
        assert_eq!(
            plain(false).unwrap_err(),
            plain(true).unwrap_err(),
            "plain, ubc={use_ubc}"
        );
    }
}
