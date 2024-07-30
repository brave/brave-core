//! Platform requirements

use crate::error::Error;
use crate::platform::Platform;
use std::{fmt, str::FromStr, string::String};

#[cfg(feature = "serde")]
use serde::{de, ser, Deserialize, Serialize};

/// Platform requirements: glob-like expressions for matching Rust platforms
/// as identified by a "target triple", e.g. `i686-apple-darwin`.
///
/// For a list of all valid platforms, "target triples", see:
///
/// <https://doc.rust-lang.org/nightly/rustc/platform-support.html>
///
/// Platforms can be grouped with simple globbing rules:
///
/// - Start with wildcard: `*-gnu`
/// - End with wildcard: `x86_64-*`
/// - Start and end with wildcard: `*windows*`
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct PlatformReq(String);

/// Wildcard character used for globbing
pub const WILDCARD: char = '*';

impl PlatformReq {
    /// Borrow this platform requirement as a string slice
    pub fn as_str(&self) -> &str {
        self.0.as_ref()
    }

    /// Does this platform requirement match the given platform string?
    ///
    /// This matcher accepts a platform "target triple" string ala
    /// `x86_64-unknown-linux-gnu` and matches it against this
    /// `Platform`, using simple glob like rules.
    pub fn matches(&self, platform: &Platform) -> bool {
        let self_len = self.as_str().len();

        // Universal matcher
        if self.0.len() == 1 && self.0.chars().next().unwrap() == WILDCARD {
            return true;
        }

        let mut chars = self.as_str().chars();
        let starts_with_wildcard = chars.next().unwrap() == WILDCARD;
        let ends_with_wildcard = chars.last() == Some(WILDCARD);

        if starts_with_wildcard {
            if ends_with_wildcard {
                // Contains expression: `*windows*`
                platform
                    .target_triple
                    .contains(&self.0[1..self_len.checked_sub(1).unwrap()])
            } else {
                // Suffix expression: `*-gnu`
                platform.target_triple.ends_with(&self.0[1..])
            }
        } else if ends_with_wildcard {
            // Prefix expression: `x86_64-*`
            platform
                .target_triple
                .starts_with(&self.0[..self_len.checked_sub(1).unwrap()])
        } else {
            // No wildcards: direct comparison
            self.as_str() == platform.target_triple
        }
    }

    /// Expand glob expressions into a list of all known matching platforms
    pub fn matching_platforms(&self) -> impl Iterator<Item = &Platform> {
        matching_platforms(self, Platform::ALL)
    }
}

// Split into its own function for unit testing
#[inline]
fn matching_platforms<'a>(
    req: &'a PlatformReq,
    platforms: &'a [Platform],
) -> impl Iterator<Item = &'a Platform> {
    platforms
        .iter()
        .filter(move |&platform| req.matches(platform))
}

impl FromStr for PlatformReq {
    type Err = Error;

    /// Create a new platform requirement. Platforms support glob-like
    /// wildcards on the beginning and end, e.g. `*windows*`.
    ///
    /// Must match at least one known Rust platform "target triple"
    /// (e.g. `x86_64-unknown-linux-gnu`) to be considered valid.
    fn from_str(req_str: &str) -> Result<PlatformReq, Error> {
        let platform_req = PlatformReq(req_str.into());

        if platform_req.0.is_empty() || platform_req.matching_platforms().next().is_none() {
            Err(Error)
        } else {
            Ok(platform_req)
        }
    }
}

impl fmt::Display for PlatformReq {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl Serialize for PlatformReq {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.serialize_str(self.as_str())
    }
}

#[cfg(feature = "serde")]
impl<'de> Deserialize<'de> for PlatformReq {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        use de::Error;
        String::deserialize(deserializer)?
            .parse()
            .map_err(D::Error::custom)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{str::FromStr, vec::Vec};

    use crate::platform::platforms::*;
    const TEST_PLATFORM_LIST: &[Platform] = &[
        AARCH64_PC_WINDOWS_MSVC,
        AARCH64_UNKNOWN_LINUX_MUSL,
        ARMV7_UNKNOWN_LINUX_MUSLEABI,
        ARMV7_UNKNOWN_LINUX_MUSLEABIHF,
        SPARC_UNKNOWN_LINUX_GNU,
        SPARC64_UNKNOWN_LINUX_GNU,
        SPARC64_UNKNOWN_NETBSD,
        SPARC64_UNKNOWN_OPENBSD,
        SPARCV9_SUN_SOLARIS,
        AARCH64_UWP_WINDOWS_MSVC,
        I586_PC_WINDOWS_MSVC,
        I686_PC_WINDOWS_GNU,
        I686_PC_WINDOWS_MSVC,
        I686_UWP_WINDOWS_GNU,
        I686_UWP_WINDOWS_MSVC,
        MIPS64_UNKNOWN_LINUX_GNUABI64,
        MIPS64_UNKNOWN_LINUX_MUSLABI64,
        THUMBV7A_PC_WINDOWS_MSVC,
        THUMBV7A_UWP_WINDOWS_MSVC,
        RISCV64GC_UNKNOWN_LINUX_MUSL,
        X86_64_PC_WINDOWS_GNU,
    ];

    #[test]
    fn prefix_glob_test() {
        let req = PlatformReq::from_str("sparc*").unwrap();

        assert_eq!(
            matching_platforms(&req, TEST_PLATFORM_LIST)
                .map(|p| p.target_triple)
                .collect::<Vec<_>>(),
            [
                "sparc-unknown-linux-gnu",
                "sparc64-unknown-linux-gnu",
                "sparc64-unknown-netbsd",
                "sparc64-unknown-openbsd",
                "sparcv9-sun-solaris"
            ]
        );
    }

    #[test]
    fn suffix_glob_test() {
        let req = PlatformReq::from_str("*-musl").unwrap();

        assert_eq!(
            matching_platforms(&req, TEST_PLATFORM_LIST)
                .map(|p| p.target_triple)
                .collect::<Vec<_>>(),
            ["aarch64-unknown-linux-musl", "riscv64gc-unknown-linux-musl"]
        );
    }

    #[test]
    fn contains_glob_test() {
        let req = PlatformReq::from_str("*windows*").unwrap();

        assert_eq!(
            matching_platforms(&req, TEST_PLATFORM_LIST)
                .map(|p| p.target_triple)
                .collect::<Vec<_>>(),
            [
                "aarch64-pc-windows-msvc",
                "aarch64-uwp-windows-msvc",
                "i586-pc-windows-msvc",
                "i686-pc-windows-gnu",
                "i686-pc-windows-msvc",
                "i686-uwp-windows-gnu",
                "i686-uwp-windows-msvc",
                "thumbv7a-pc-windows-msvc",
                "thumbv7a-uwp-windows-msvc",
                "x86_64-pc-windows-gnu",
            ]
        );
    }

    #[test]
    fn direct_match_test() {
        let req = PlatformReq::from_str("x86_64-unknown-dragonfly").unwrap();

        assert_eq!(
            req.matching_platforms()
                .map(|p| p.target_triple)
                .collect::<Vec<_>>(),
            ["x86_64-unknown-dragonfly"]
        );
    }

    #[test]
    fn wildcard_test() {
        let req = PlatformReq::from_str("*").unwrap();
        assert_eq!(req.matching_platforms().count(), Platform::ALL.len())
    }

    // How to handle this is debatable...
    #[test]
    fn double_wildcard_test() {
        let req = PlatformReq::from_str("**").unwrap();
        assert_eq!(req.matching_platforms().count(), Platform::ALL.len())
    }

    #[test]
    fn invalid_req_tests() {
        assert!(PlatformReq::from_str("").is_err());
        assert!(PlatformReq::from_str(" ").is_err());
        assert!(PlatformReq::from_str("derp").is_err());
        assert!(PlatformReq::from_str("***").is_err());
    }
}
