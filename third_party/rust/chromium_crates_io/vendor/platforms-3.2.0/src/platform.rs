//! Rust platforms

mod platforms;

#[cfg(feature = "std")]
mod req;
mod tier;

pub use self::tier::Tier;

#[cfg(feature = "std")]
pub use self::req::PlatformReq;

use self::platforms::ALL;
use crate::target::*;
use core::fmt;

/// Rust platforms supported by mainline rustc
///
/// Sourced from <https://doc.rust-lang.org/nightly/rustc/platform-support.html>
/// as well as the latest nightly version of `rustc`
#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub struct Platform {
    /// "Target triple" string uniquely identifying the platform. See:
    /// <https://github.com/rust-lang/rfcs/blob/master/text/0131-target-specification.md>
    ///
    /// These are defined in the `rustc_target` crate of the Rust compiler:
    /// <https://github.com/rust-lang/rust/blob/master/src/librustc_target/spec/mod.rs>
    pub target_triple: &'static str,

    /// Target architecture `cfg` attribute (i.e. `cfg(target_arch)`)
    pub target_arch: Arch,

    /// Target OS `cfg` attribute (i.e. `cfg(target_os)`).
    pub target_os: OS,

    /// Target environment `cfg` attribute (i.e. `cfg(target_env)`).
    /// Only used when needed for disambiguation, e.g. on many GNU platforms
    /// this value will be `None`.
    pub target_env: Env,

    /// Target pointer width `cfg` attribute, in bits (i.e. `cfg(target_pointer_width)`).
    /// Typically 64 on modern platforms, 32 on older platforms, 16 on some microcontrollers.
    pub target_pointer_width: PointerWidth,

    /// Target [endianness](https://en.wikipedia.org/wiki/Endianness) `cfg` attribute (i.e. `cfg(target_endian)`).
    /// Set to "little" on the vast majority of modern platforms.
    pub target_endian: Endian,

    /// Tier of this platform:
    ///
    /// - `Tier::One`: guaranteed to work
    /// - `Tier::Two`: guaranteed to build
    /// - `Tier::Three`: unofficially supported with no guarantees
    pub tier: Tier,
}

impl Platform {
    /// All valid Rust platforms usable from the mainline compiler.
    ///
    /// Note that this list will evolve over time, and platforms will be both added and removed.
    pub const ALL: &'static [Platform] = ALL;

    /// Find a Rust platform by its "target triple", e.g. `i686-apple-darwin`
    pub fn find(target_triple: &str) -> Option<&'static Platform> {
        Self::ALL
            .iter()
            .find(|platform| platform.target_triple == target_triple)
    }
}

impl fmt::Display for Platform {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.target_triple)
    }
}

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::Platform;
    use std::collections::HashSet;

    /// Ensure there are no duplicate target triples in the platforms list
    #[test]
    fn no_dupes_test() {
        let mut target_triples = HashSet::new();

        for platform in Platform::ALL {
            assert!(
                target_triples.insert(platform.target_triple),
                "duplicate target triple: {}",
                platform.target_triple
            );
        }
    }

    use std::collections::HashMap;

    use super::*;

    /// `platforms` v2.0 used to provide various constants passed as `cfg` values,
    /// and attempted to detect the target triple based on that.
    /// This test is meant to check whether such detection can be accurate.
    ///
    /// Turns out that as of v3.0 this is infeasible,
    /// even though the list of supported cfg values was expanded.
    ///
    /// I have also verified that no possible expansion of the supported cfg fields
    /// will lets uniquely identify the platform based on cfg values using a shell script:
    /// `rustc --print=target-list | parallel 'rustc --print=cfg --target={} > ./{}'; fdupes`
    #[test]
    #[ignore]
    fn test_detection_feasibility() {
        let mut all_platforms = HashMap::new();
        for p in ALL {
            if let Some(other_p) = all_platforms.insert(
                (
                    p.target_arch,
                    p.target_os,
                    p.target_env,
                    p.target_endian,
                    p.target_pointer_width,
                ),
                p.target_triple,
            ) {
                panic!("{} and {} have identical properties, and cannot be distinguished based on properties alone", p.target_triple, other_p);
            }
        }
    }
}
