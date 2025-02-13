#![forbid(unsafe_code)]

//! Obtains the dependency list from a compiled Rust binary by parsing its panic messages.
//! Recovers both crate names and versions.
//!
//! ## Caveats
//!  * If the crate never panics, it will not show up.
//!    The Rust compiler is very good at removing unreachable panics,
//!    so we can only discover at around a half of all dependencies.
//!  * C code such as `openssl-src` never shows up, because it can't panic.
//!  * Only crates installed from a registry are discovered. Crates from local workspace or git don't show up.
//!
//! # Alternatives
//! [`cargo auditable`](https://crates.io/crates/cargo-auditable) embeds the **complete** dependency information
//! into binaries, which can then be recovered using [`auditable-info`](https://crates.io/crates/auditable-info).
//! It should be used instead of `quitters` whenever possible, unless you're specifically after panics.

use std::collections::BTreeSet;

use once_cell::sync::OnceCell;
use regex::bytes::Regex;
use semver::Version;

// This regex works suprisingly well. We can even split the crate name and version reliably
// because crate names publishable on crates.io cannot contain the `.` character,
// which *must* appear in the version string.
// Versions like "1" are not valid in Cargo, or under the semver spec.
const REGEX_STRING: &str =
    "(?-u)cargo/registry/src/[^/]+/(?P<crate>[0-9A-Za-z_-]+)-(?P<version>[0-9]+\\.[0-9]+\\.[0-9]+[0-9A-Za-z+.-]*)/";

// Compiled regular expressions use interior mutability and may cause contention
// in heavily multi-threaded workloads. This should not be an issue here
// because we only use `.captures_iter()`, which acquires the mutable state
// only once per invocation and for a short amount of time:
// https://github.com/rust-lang/regex/blob/0d0023e412f7ead27b0809f5d2f95690d0f0eaef/PERFORMANCE.md#using-a-regex-from-multiple-threads
// This could be refactored into cloning in case it *does* end up being a bottleneck in practice,
// which would sacrifice ergonomics.
static REGEX_UNIX: OnceCell<Regex> = OnceCell::new();
static REGEX_WINDOWS: OnceCell<Regex> = OnceCell::new();

/// Obtains the dependency list from a compiled Rust binary by parsing its panic messages.
///
/// ## Caveats
///  * If the crate never panics, it will not show up.
///    The Rust compiler is very good at removing unreachable panics,
///    so we can only discover at around a half of all dependencies.
///  * C code such as `openssl-src` never shows up, because it can't panic.
///  * Only crates installed from a registry are discovered. Crates from local workspace or git don't show up.
///
/// ## Usage
/// ```rust,ignore
/// let file = std::fs::read("target/release/my-program")?;
/// let versions = quitters::versions(&file);
/// for (krate, version) in versions.iter() {
///    println!("{krate} v{version}")
/// }
/// ```
pub fn versions(data: &[u8]) -> BTreeSet<(&str, Version)> {
    // You might think that just making two functions, versions_unix and versions_windows
    // and then calling the appropriate function for your platform would be faster,
    // since \ paths cannot be used on Unix. I briefly thought so!
    // However, cross-compilation from Windows to Unix would put \ paths into a Unix binary.
    // So that optimization would miss cross-compiled binaries.
    // It only gets you a 20% reduction in runtime because the I/O dominates anyway.
    //
    // A significant optimization to tackle the I/O problem would be only ever reading things
    // into the CPU cache as opposed to loading the entire file to memory.
    // Basically streaming the data. This requires special handling of the start and end,
    // so either needs a state-machine-based parser like nom or capping the possible match length.
    // The latter is doable but only makes sense if it turns out that the current approach is too slow.
    let re = REGEX_UNIX.get_or_init(|| Regex::new(REGEX_STRING).unwrap());
    let versions = versions_for_regex(data, re);
    if !versions.is_empty() {
        versions
    } else {
        // Sadly the single-pass RegexSet only lets you check for presence of matches,
        // and doesn't let you find out where they are.
        // And using a composite regex like `unix_regex|windows_regex` is as slow as two passes,
        // so we'll just use two passes. That's what Regex crate documentation recommends, too.
        let re = REGEX_WINDOWS.get_or_init(|| {
            let windows_regex = REGEX_STRING.replace('/', "\\\\");
            Regex::new(&windows_regex).unwrap()
        });
        versions_for_regex(data, re)
    }
}

fn versions_for_regex<'a>(data: &'a [u8], re: &Regex) -> BTreeSet<(&'a str, Version)> {
    let mut versions = BTreeSet::new();
    for c in re.captures_iter(data) {
        if let Some(parsed) = parse_capture(c) {
            versions.insert(parsed);
        }
    }
    versions
}

/// Extracts crate and version from a single regex match
fn parse_capture(c: regex::bytes::Captures) -> Option<(&str, Version)> {
    Some((
        std::str::from_utf8(c.name("crate").unwrap().as_bytes()).ok()?,
        Version::parse(std::str::from_utf8(c.name("version").unwrap().as_bytes()).ok()?).ok()?,
    ))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn two_crates_one_line() {
        let data = b"\x7FELF/cargo/registry/src/github.com-1ecc6299db9ec823/xz2-0.1.6/src/stream.rsunknown return code: lzma data errorNoCheckProgramMemFormatOptionszstd returned null pointer when creating new context/cargo/registry/src/github.com-1ecc6299db9ec823/zstd-safe-5.0.2+zstd.1.5.2/src/lib.rsbad error message from zstdGiven position outside of the buffer bounds.";
        assert_eq!(versions(data).len(), 2);
    }

    #[test]
    fn complex_versions() {
        for version_suffix in [
            "",
            "+foobar",
            "+Fo0bar",
            "+zstd.1.5.2",
            "-rc",
            "-alpha.1",
            "-alpha.1+zstd.1.5.2",
        ] {
            let string = format!("new context/cargo/registry/src/github.com-1ecc6299db9ec823/zstd-safe-5.0.2{}/src/lib.rsbad error message from zstdGiven position outside of the buffer bounds.", version_suffix);
            let expected_version = format!("5.0.2{}", version_suffix);
            assert!(versions(string.as_bytes())
                .contains(&("zstd-safe", Version::parse(&expected_version).unwrap())));
        }
    }

    #[test]
    fn windows_matching() {
        let data = br"C:\Users\runneradmin\.cargo\registry\src\github.com-1ecc6299db9ec823\rustc-demangle-0.1.21\src\legacy.rs";
        assert!(versions(data).contains(&("rustc-demangle", Version::parse("0.1.21").unwrap())))
    }
}
