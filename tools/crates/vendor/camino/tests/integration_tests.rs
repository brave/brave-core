// Copyright (c) The camino Contributors
// SPDX-License-Identifier: MIT OR Apache-2.0

use camino::{Utf8Path, Utf8PathBuf};
use std::{
    collections::hash_map::DefaultHasher,
    hash::{Hash, Hasher},
    path::Path,
};

static PATH_CORPUS: &[&str] = &[
    "",
    "foo",
    "foo/bar",
    "foo//bar",
    "foo/bar/baz",
    "foo/bar/./baz",
    "foo/bar/../baz",
    "../foo/bar/./../baz",
    "/foo",
    "/foo/bar",
    "/",
    "///",
    // ---
    // Windows-only paths
    // ---
    #[cfg(windows)]
    "foo\\bar",
    #[cfg(windows)]
    "\\foo\\bar",
    #[cfg(windows)]
    "C:\\foo",
    #[cfg(windows)]
    "C:foo\\bar",
    #[cfg(windows)]
    "C:\\foo\\..\\.\\bar",
    #[cfg(windows)]
    "\\\\server\\foo\\bar",
    #[cfg(windows)]
    "\\\\.\\C:\\foo\\bar.txt",
];

#[test]
fn test_borrow_eq_ord() {
    // Utf8PathBuf implements Borrow<Utf8Path> so equality and ordering comparisons should
    // match.
    for (idx, &path1) in PATH_CORPUS.iter().enumerate() {
        for &path2 in &PATH_CORPUS[idx..] {
            let borrowed1 = Utf8Path::new(path1);
            let borrowed2 = Utf8Path::new(path2);
            let owned1 = Utf8PathBuf::from(path1);
            let owned2 = Utf8PathBuf::from(path2);

            assert_eq!(
                borrowed1 == borrowed2,
                owned1 == owned2,
                "Eq impls match: {} == {}",
                borrowed1,
                borrowed2
            );
            assert_eq!(
                borrowed1.cmp(borrowed2),
                owned1.cmp(&owned2),
                "Ord impls match: {} and {}",
                borrowed1,
                borrowed2
            );

            // Also check against std paths.
            let std1 = Path::new(path1);
            let std2 = Path::new(path2);
            assert_eq!(
                borrowed1, std1,
                "Eq between Path and Utf8Path: {}",
                borrowed1
            );
            assert_eq!(
                borrowed1 == borrowed2,
                std1 == std2,
                "Eq impl matches Path: {} == {}",
                borrowed1,
                borrowed2
            );
            assert_eq!(
                borrowed1.cmp(borrowed2),
                std1.cmp(std2),
                "Ord impl matches Path: {} and {}",
                borrowed1,
                borrowed2
            );
        }
    }
}

#[test]
fn test_borrow_hash() {
    // Utf8PathBuf implements Borrow<Utf8Path> so hash comparisons should match.
    fn hash_output(x: impl Hash) -> u64 {
        let mut hasher = DefaultHasher::new();
        x.hash(&mut hasher);
        hasher.finish()
    }

    for &path in PATH_CORPUS {
        let borrowed = Utf8Path::new(path);
        let owned = Utf8PathBuf::from(path);

        assert_eq!(
            hash_output(owned),
            hash_output(borrowed),
            "consistent Hash: {}",
            borrowed
        );
    }
}

// Utf8Path's Eq compares components, so textually-distinct paths can compare
// equal (e.g. "foo//bar" == "foo/bar"). The Hash/Eq contract requires equal
// values to hash equally -- this test verifies that using the path corpus.
#[test]
fn test_eq_implies_equal_hash() {
    // This is similar to hash_one in the standard library, except our MSRV is
    // currently below 1.71 which introduced it.
    fn hash_one(x: impl Hash) -> u64 {
        let mut hasher = DefaultHasher::new();
        x.hash(&mut hasher);
        hasher.finish()
    }

    // Count equal pairs that have distinct string representations. If the
    // corpus ever loses all such pairs, this test would silently pass, so we
    // assert below that we exercised at least one.
    let mut nontrivial_pairs = 0;

    for (idx, &path1) in PATH_CORPUS.iter().enumerate() {
        for &path2 in &PATH_CORPUS[idx..] {
            let p1 = Utf8Path::new(path1);
            let p2 = Utf8Path::new(path2);

            // The converse does not hold, so we only assert equal => equal
            // hash.
            if p1 == p2 {
                if path1 != path2 {
                    nontrivial_pairs += 1;
                }
                assert_eq!(
                    hash_one(p1),
                    hash_one(p2),
                    "equal paths must hash equally: {p1:?} vs {p2:?}",
                );
            }
        }
    }

    assert!(
        nontrivial_pairs > 0,
        "PATH_CORPUS must contain at least one equal-but-textually-distinct pair \
         (e.g. \"foo/bar\" / \"foo//bar\") for this test to be meaningful",
    );
}
