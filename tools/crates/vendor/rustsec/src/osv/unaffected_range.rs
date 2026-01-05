//! This is an intermediate representation used for converting from
//! Cargo-style version selectors (`>=`, `^`, `<`, etc) to OSV ranges.
//! It is an implementation detail and is not exported outside OSV module.

use crate::{Error, ErrorKind::BadParam};
use semver::{Comparator, Op, Prerelease, Version};
use std::fmt::Display;

#[derive(Clone, PartialEq, Eq, Hash, Debug)]
pub(crate) enum Bound {
    Unbounded,
    Exclusive(Version),
    Inclusive(Version),
}

impl Bound {
    /// Returns just the version, ignoring whether the bound is inclusive or exclusive
    pub fn version(&self) -> Option<&Version> {
        match &self {
            Bound::Unbounded => None,
            Bound::Exclusive(v) => Some(v),
            Bound::Inclusive(v) => Some(v),
        }
    }

    /// The handling of `Bound::Unbounded` in this function assumes that
    /// the first bound is start of a range, and the other bound is the end of a range.
    /// **Make sure** this is the way you call it.
    /// This is also why we don't define PartialOrd.
    fn less_or_equal(&self, other: &Bound) -> bool {
        // It's defined on Bound and not UnaffectedRange
        // so that it could be used on bounds from different ranges.
        let start = self;
        let end = other;
        // This appears to be a false positive in Clippy:
        // https://github.com/rust-lang/rust-clippy/issues/7383
        #[allow(clippy::if_same_then_else)]
        if start == &Bound::Unbounded || end == &Bound::Unbounded {
            true
        } else if start.version().unwrap() < end.version().unwrap() {
            true
        } else {
            match (&start, &end) {
                (Bound::Inclusive(v_start), Bound::Inclusive(v_end)) => v_start == v_end,
                (_, _) => false,
            }
        }
    }
}

/// A range of unaffected versions, used by either `patched`
/// or `unaffected` fields in the security advisory.
/// Bounds may be inclusive or exclusive.
/// `start` is guaranteed to be less than or equal to `end`.
/// If `start == end`, both bounds must be inclusive.
#[derive(Clone, PartialEq, Eq, Hash, Debug)]
pub(crate) struct UnaffectedRange {
    start: Bound,
    end: Bound,
}

impl UnaffectedRange {
    pub fn new(start: Bound, end: Bound) -> Result<Self, Error> {
        if start.less_or_equal(&end) {
            Ok(UnaffectedRange { start, end })
        } else {
            Err(Error::new(
                BadParam,
                "Invalid range: start must be <= end; if equal, both bounds must be inclusive",
            ))
        }
    }

    pub fn start(&self) -> &Bound {
        &self.start
    }

    pub fn end(&self) -> &Bound {
        &self.end
    }

    pub fn overlaps(&self, other: &UnaffectedRange) -> bool {
        // range check for well-formed ranges is `(Start1 <= End2) && (Start2 <= End1)`
        self.start.less_or_equal(&other.end) && other.start.less_or_equal(&self.end)
    }
}

impl Display for UnaffectedRange {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match &self.start {
            Bound::Unbounded => f.write_str("[0"),
            Bound::Exclusive(v) => f.write_fmt(format_args!("({v}")),
            Bound::Inclusive(v) => f.write_fmt(format_args!("[{v}")),
        }?;
        f.write_str(", ")?;
        match &self.end {
            Bound::Unbounded => f.write_str("∞)"),
            Bound::Exclusive(v) => f.write_fmt(format_args!("{v})")),
            Bound::Inclusive(v) => f.write_fmt(format_args!("{v}]")),
        }
    }
}

/// To keep the algorithm simple, we impose several constraints:
///
/// 1. There is at most one upper and at most one lower bound in each range.
///    Stuff like `>= 1.0, >= 2.0` is nonsense and is not supported.
/// 2. If the requirement is "1.0" or "^1.0" that defines both the lower and upper bound,
///    it is the only one in its range.
///
/// If any of those constraints are unmet, an error will be returned.
impl TryFrom<&semver::VersionReq> for UnaffectedRange {
    type Error = Error;

    fn try_from(input: &semver::VersionReq) -> Result<Self, Self::Error> {
        if input.comparators.len() > 2 {
            fail!(
                BadParam,
                format!("Too many comparators in version specification: {}", input)
            );
        }
        // If one of the bounds is not specified, it's unbounded,
        // e.g. ["> 0.5"] means the lower bound is 0.5 and there is no upper bound
        let mut start = Bound::Unbounded;
        let mut end = Bound::Unbounded;
        for comparator in &input.comparators {
            match comparator.op {
                // Full list of operators supported by Cargo can be found here:
                // https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html
                // One of the Cargo developers has confirmed that the list is complete:
                // https://internals.rust-lang.org/t/changing-cargo-semver-compatibility-for-pre-releases/14820/14
                // However, `semver` crate recognizes more operators than Cargo supports
                Op::Greater => {
                    if start != Bound::Unbounded {
                        fail!(
                            BadParam,
                            format!("More than one lower bound in the same range: {}", input)
                        );
                    }
                    start = Bound::Exclusive(comp_to_ver(comparator));
                }
                Op::GreaterEq => {
                    if start != Bound::Unbounded {
                        fail!(
                            BadParam,
                            format!("More than one lower bound in the same range: {}", input)
                        );
                    }
                    start = Bound::Inclusive(comp_to_ver(comparator));
                }
                Op::Less => {
                    if end != Bound::Unbounded {
                        fail!(
                            BadParam,
                            format!("More than one upper bound in the same range: {}", input)
                        );
                    }
                    end = Bound::Exclusive(comp_to_ver(comparator));
                }
                Op::LessEq => {
                    if end != Bound::Unbounded {
                        fail!(
                            BadParam,
                            format!("More than one upper bound in the same range: {}", input)
                        );
                    }
                    end = Bound::Inclusive(comp_to_ver(comparator));
                }
                Op::Exact => {
                    if input.comparators.len() != 1 {
                        fail!(
                            BadParam,
                            "Selectors that define an exact version (e.g. '=1.0') must be alone in their range"
                        );
                    }
                    start = Bound::Inclusive(comp_to_ver(comparator));
                    end = Bound::Inclusive(comp_to_ver(comparator));
                }
                Op::Caret => {
                    if input.comparators.len() != 1 {
                        fail!(
                            BadParam,
                            "Selectors that define both the upper and lower bound (e.g. '^1.0') must be alone in their range"
                        );
                    }
                    let start_version = comp_to_ver(comparator);
                    let mut end_version = if start_version.major == 0 {
                        match (comparator.minor, comparator.patch) {
                            // ^0.0.x
                            (Some(0), Some(patch)) => Version::new(0, 0, patch + 1),
                            // ^0.x and ^0.x.x
                            (Some(minor), _) => Version::new(0, minor + 1, 0),
                            // ^0
                            (None, None) => Version::new(1, 0, 0),
                            (None, Some(_)) => unreachable!(
                                "Comparator specifies patch version but not minor version"
                            ),
                        }
                    } else {
                        Version::new(&start_version.major + 1, 0, 0)
                    };
                    // -0 is the lowest possible prerelease.
                    // If we didn't append it, e.g. ^1.0.0 would match 2.0.0-alpha1
                    end_version.pre = Prerelease::new("0").unwrap();
                    start = Bound::Inclusive(start_version);
                    end = Bound::Exclusive(end_version);
                }
                Op::Tilde => {
                    if input.comparators.len() != 1 {
                        fail!(
                            BadParam,
                            "Selectors that define both the upper and lower bound (e.g. '~1.0') must be alone in their range"
                        );
                    }
                    let start_version = comp_to_ver(comparator);
                    let major = comparator.major;
                    let mut end_version = match (comparator.minor, comparator.patch) {
                        (None, None) => Version::new(major + 1, 0, 0),
                        (Some(minor), _) => Version::new(major, minor + 1, 0),
                        (None, Some(_)) => {
                            unreachable!("Comparator specifies patch version but not minor version")
                        }
                    };
                    // -0 is the lowest possible prerelease.
                    // If we didn't append it, e.g. ~1.2 would match 1.3.0-alpha1
                    end_version.pre = Prerelease::new("0").unwrap();
                    start = Bound::Inclusive(start_version);
                    end = Bound::Exclusive(end_version);
                }
                _ => {
                    // the struct is non-exhaustive, we have to do this
                    fail!(
                        BadParam,
                        "Unsupported operator in version specification: '{}'",
                        comparator
                    );
                }
            }
        }
        UnaffectedRange::new(start, end)
    }
}

/// Strips comparison operators from a Comparator and turns it into a Version.
/// Would have been better implemented by `into` but these are foreign types
fn comp_to_ver(c: &Comparator) -> Version {
    Version {
        major: c.major,
        minor: c.minor.unwrap_or(0),
        patch: c.patch.unwrap_or(0),
        pre: c.pre.clone(),
        build: Default::default(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use semver::VersionReq;

    #[test]
    fn both_unbounded() {
        let range1 = UnaffectedRange {
            start: Bound::Unbounded,
            end: Bound::Unbounded,
        };
        let range2 = UnaffectedRange {
            start: Bound::Unbounded,
            end: Bound::Unbounded,
        };
        assert!(range1.overlaps(&range2));
        assert!(range2.overlaps(&range1));
    }

    #[test]
    fn barely_not_overlapping() {
        let range1 = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.2.3").unwrap()),
            end: Bound::Unbounded,
        };
        let range2 = UnaffectedRange {
            start: Bound::Unbounded,
            end: Bound::Exclusive(Version::parse("1.2.3").unwrap()),
        };
        assert!(!range1.overlaps(&range2));
        assert!(!range2.overlaps(&range1));
    }

    #[test]
    fn barely_overlapping() {
        let range1 = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.2.3").unwrap()),
            end: Bound::Unbounded,
        };
        let range2 = UnaffectedRange {
            start: Bound::Unbounded,
            end: Bound::Inclusive(Version::parse("1.2.3").unwrap()),
        };
        assert!(range1.overlaps(&range2));
        assert!(range2.overlaps(&range1));
    }

    #[test]
    fn clearly_not_overlapping() {
        let range1 = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.1.0").unwrap()),
            end: Bound::Inclusive(Version::parse("0.3.0").unwrap()),
        };
        let range2 = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.1.0").unwrap()),
            end: Bound::Inclusive(Version::parse("1.3.0").unwrap()),
        };
        assert!(!range1.overlaps(&range2));
        assert!(!range2.overlaps(&range1));
    }

    #[test]
    fn clearly_overlapping() {
        let range1 = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.1.0").unwrap()),
            end: Bound::Inclusive(Version::parse("1.1.0").unwrap()),
        };
        let range2 = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.2.0").unwrap()),
            end: Bound::Inclusive(Version::parse("1.3.0").unwrap()),
        };
        assert!(range1.overlaps(&range2));
        assert!(range2.overlaps(&range1));
    }

    #[test]
    fn exact_requirement_10() {
        let input = VersionReq::parse("=1.0").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.0.0").unwrap()),
            end: Bound::Inclusive(Version::parse("1.0.0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    // Test data for caret requirements is taken from the Cargo spec
    // https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html#caret-requirements
    // but adjusted to correctly handle pre-releases under semver precedence rules:
    // https://semver.org/#spec-item-11

    #[test]
    fn caret_requirement_123() {
        let input = VersionReq::parse("^1.2.3").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.2.3").unwrap()),
            end: Bound::Exclusive(Version::parse("2.0.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn caret_requirement_12() {
        let input = VersionReq::parse("^1.2").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.2.0").unwrap()),
            end: Bound::Exclusive(Version::parse("2.0.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn caret_requirement_1() {
        let input = VersionReq::parse("^1").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.0.0").unwrap()),
            end: Bound::Exclusive(Version::parse("2.0.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn caret_requirement_023() {
        let input = VersionReq::parse("^0.2.3").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.2.3").unwrap()),
            end: Bound::Exclusive(Version::parse("0.3.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn caret_requirement_02() {
        let input = VersionReq::parse("^0.2").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.2.0").unwrap()),
            end: Bound::Exclusive(Version::parse("0.3.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn caret_requirement_003() {
        let input = VersionReq::parse("^0.0.3").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.0.3").unwrap()),
            end: Bound::Exclusive(Version::parse("0.0.4-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn caret_requirement_00() {
        let input = VersionReq::parse("^0.0").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.0.0").unwrap()),
            end: Bound::Exclusive(Version::parse("0.1.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn caret_requirement_0() {
        let input = VersionReq::parse("^0").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("0.0.0").unwrap()),
            end: Bound::Exclusive(Version::parse("1.0.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    // Test data for tilde requirements is taken from the Cargo spec
    // https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html#tilde-requirements
    // but adjusted to correctly handle pre-releases under semver precedence rules:
    // https://semver.org/#spec-item-11

    #[test]
    fn tilde_requirement_123() {
        let input = VersionReq::parse("~1.2.3").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.2.3").unwrap()),
            end: Bound::Exclusive(Version::parse("1.3.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn tilde_requirement_12() {
        let input = VersionReq::parse("~1.2").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.2.0").unwrap()),
            end: Bound::Exclusive(Version::parse("1.3.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }

    #[test]
    fn tilde_requirement_1() {
        let input = VersionReq::parse("~1").unwrap();
        let expected = UnaffectedRange {
            start: Bound::Inclusive(Version::parse("1.0.0").unwrap()),
            end: Bound::Exclusive(Version::parse("2.0.0-0").unwrap()),
        };
        let result: UnaffectedRange = (&input).try_into().unwrap();
        assert_eq!(expected, result);
    }
}
