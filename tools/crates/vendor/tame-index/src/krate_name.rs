use crate::error::{Error, InvalidKrateName};

#[cfg(test)]
/// Create a `KrateName` from a string literal.
#[macro_export]
macro_rules! kn {
    ($kn:literal) => {
        $crate::KrateName($kn)
    };
}

/// Used to wrap user-provided strings so that bad crate names are required to
/// be handled separately from things more outside the user control such as I/O
/// errors
#[derive(Copy, Clone)]
pub struct KrateName<'name>(pub(crate) &'name str);

impl<'name> KrateName<'name> {
    /// Ensures the specified string is a valid crates.io crate name, according
    /// to the (current) crates.io name restrictions
    ///
    /// 1. Non-empty
    /// 2. May not start with a digit
    /// 3. Maximum of 64 characters in length
    /// 4. Must be ASCII alphanumeric, `-`, or `_`
    /// 5. May not be a reserved name
    ///     * A Rust keyword
    ///     * Name of a Cargo output artifact
    ///     * Name of a std library crate (or `test`)
    ///     * A reserved Windows name (such as `nul`)
    #[inline]
    pub fn crates_io(name: &'name str) -> Result<Self, Error> {
        Self::validated(name, Some(64))
    }

    /// Ensures the specified string is a valid crate name according to [cargo](https://github.com/rust-lang/cargo/blob/00b8da63269420610758464c02fc46584e373dd3/src/cargo/ops/cargo_new.rs#L167-L264)
    ///
    /// 1. Non-empty
    /// 2. May not start with a digit
    /// 3. Must be ASCII alphanumeric, `-`, or `_`
    /// 4. May not be a reserved name
    ///     * A Rust keyword
    ///     * Name of a Cargo output artifact
    ///     * Name of a std library crate (or `test`)
    ///     * A reserved Windows name (such as `nul`)
    #[inline]
    pub fn cargo(name: &'name str) -> Result<Self, Error> {
        Self::validated(name, None)
    }

    fn validated(name: &'name str, max_len: Option<usize>) -> Result<Self, Error> {
        if name.is_empty() {
            return Err(InvalidKrateName::InvalidLength(0).into());
        }

        let mut chars = name.chars().enumerate();

        while let Some((i, c)) = chars.next() {
            if i == 0 && c != '_' && !c.is_ascii_alphabetic() {
                return Err(InvalidKrateName::InvalidCharacter {
                    invalid: c,
                    index: i,
                }
                .into());
            }

            if max_len == Some(i) {
                return Err(InvalidKrateName::InvalidLength(i + 1 + chars.count()).into());
            }

            if c != '-' && c != '_' && !c.is_ascii_alphanumeric() {
                return Err(InvalidKrateName::InvalidCharacter {
                    invalid: c,
                    index: i,
                }
                .into());
            }
        }

        // This is a single table, binary sorted so that we can more easily just
        // check matches and move on
        //
        // 1. Rustlang keywords, see https://doc.rust-lang.org/reference/keywords.html
        // 2. Windows reserved, see https://github.com/rust-lang/cargo/blob/b40be8bdcf2eff9ed81702594d44bf96c27973a6/src/cargo/util/restricted_names.rs#L26-L32
        // 3. Cargo artifacts, see https://github.com/rust-lang/cargo/blob/b40be8bdcf2eff9ed81702594d44bf96c27973a6/src/cargo/util/restricted_names.rs#L35-L37
        // 4. Rustlang std, see https://github.com/rust-lang/cargo/blob/b40be8bdcf2eff9ed81702594d44bf96c27973a6/src/cargo/ops/cargo_new.rs#L225-L239
        use crate::error::ReservedNameKind::{Artifact, Keyword, Standard, Windows};
        const DISALLOWED: &[(&str, crate::error::ReservedNameKind)] = &[
            ("Self", Keyword),
            ("abstract", Keyword),
            ("alloc", Standard),
            ("as", Keyword),
            ("async", Keyword),
            ("aux", Windows),
            ("await", Keyword),
            ("become", Keyword),
            ("box", Keyword),
            ("break", Keyword),
            ("build", Artifact),
            ("com1", Windows),
            ("com2", Windows),
            ("com3", Windows),
            ("com4", Windows),
            ("com5", Windows),
            ("com6", Windows),
            ("com7", Windows),
            ("com8", Windows),
            ("com9", Windows),
            ("con", Windows),
            ("const", Keyword),
            ("continue", Keyword),
            ("core", Standard),
            ("crate", Keyword),
            ("deps", Artifact),
            ("do", Keyword),
            ("dyn", Keyword),
            ("else", Keyword),
            ("enum", Keyword),
            ("examples", Artifact),
            ("extern", Keyword),
            ("false", Keyword),
            ("final", Keyword),
            ("fn", Keyword),
            ("for", Keyword),
            ("if", Keyword),
            ("impl", Keyword),
            ("in", Keyword),
            ("incremental", Artifact),
            ("let", Keyword),
            ("loop", Keyword),
            ("lpt1", Windows),
            ("lpt2", Windows),
            ("lpt3", Windows),
            ("lpt4", Windows),
            ("lpt5", Windows),
            ("lpt6", Windows),
            ("lpt7", Windows),
            ("lpt8", Windows),
            ("lpt9", Windows),
            ("macro", Keyword),
            ("match", Keyword),
            ("mod", Keyword),
            ("move", Keyword),
            ("mut", Keyword),
            ("nul", Windows),
            ("override", Keyword),
            ("priv", Keyword),
            ("prn", Windows),
            ("proc-macro", Standard),
            ("proc_macro", Standard),
            ("pub", Keyword),
            ("ref", Keyword),
            ("return", Keyword),
            ("self", Keyword),
            ("static", Keyword),
            ("std", Standard),
            ("struct", Keyword),
            ("super", Keyword),
            ("test", Standard),
            ("trait", Keyword),
            ("true", Keyword),
            ("try", Keyword),
            ("type", Keyword),
            ("typeof", Keyword),
            ("unsafe", Keyword),
            ("unsized", Keyword),
            ("use", Keyword),
            ("virtual", Keyword),
            ("where", Keyword),
            ("while", Keyword),
            ("yield", Keyword),
        ];

        if let Ok(i) = DISALLOWED.binary_search_by_key(&name, |(k, _v)| k) {
            let (reserved, kind) = DISALLOWED[i];
            Err(InvalidKrateName::ReservedName { reserved, kind }.into())
        } else {
            Ok(Self(name))
        }
    }
}

/// The simplest way to create a crate name, this just ensures that the crate name
/// is non-empty, and ASCII alphanumeric, `-`, or, `-`, the minimum requirements
/// for this crate
impl<'name> TryFrom<&'name str> for KrateName<'name> {
    type Error = Error;
    #[inline]
    fn try_from(s: &'name str) -> Result<Self, Self::Error> {
        if s.is_empty() {
            Err(InvalidKrateName::InvalidLength(0).into())
        } else if let Some((index, invalid)) = s
            .chars()
            .enumerate()
            .find(|(_i, c)| *c != '-' && *c != '_' && !c.is_ascii_alphanumeric())
        {
            Err(InvalidKrateName::InvalidCharacter { invalid, index }.into())
        } else {
            Ok(Self(s))
        }
    }
}

impl KrateName<'_> {
    /// Writes the crate's prefix to the specified string
    ///
    /// Cargo uses a simple prefix in the registry index so that crate's can be
    /// partitioned, particularly on disk without running up against potential OS
    /// specific issues when hundreds of thousands of files are located with a single
    /// directory
    ///
    /// The separator should be [`std::path::MAIN_SEPARATOR`] in disk cases and
    /// '/' when used for urls
    pub fn prefix(&self, acc: &mut String, sep: char) {
        let name = self.0;
        match name.len() {
            0 => unreachable!(),
            1 => acc.push('1'),
            2 => acc.push('2'),
            3 => {
                acc.push('3');
                acc.push(sep);
                acc.push_str(&name[..1]);
            }
            _ => {
                acc.push_str(&name[..2]);
                acc.push(sep);
                acc.push_str(&name[2..4]);
            }
        }
    }

    /// Gets the relative path to a crate
    ///
    /// This will be of the form [`Self::prefix`] + `<sep>` + `<name>`
    ///
    /// If not specified, the separator is [`std::path::MAIN_SEPARATOR`]
    ///
    /// ```
    /// let crate_name: tame_index::KrateName = "tame-index".try_into().unwrap();
    /// assert_eq!(crate_name.relative_path(Some('/')), "ta/me/tame-index");
    /// ```
    pub fn relative_path(&self, sep: Option<char>) -> String {
        let name = self.0;
        // Preallocate with the maximum possible width of a crate prefix `aa/bb/`
        let mut rel_path = String::with_capacity(name.len() + 6);
        let sep = sep.unwrap_or(std::path::MAIN_SEPARATOR);

        self.prefix(&mut rel_path, sep);
        rel_path.push(sep);
        rel_path.push_str(name);

        // A valid krate name is ASCII only, we don't need to worry about
        // lowercasing utf-8
        rel_path.make_ascii_lowercase();

        rel_path
    }
}

use std::fmt;

impl fmt::Display for KrateName<'_> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.0)
    }
}

impl fmt::Debug for KrateName<'_> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.0)
    }
}

#[cfg(test)]
mod test {
    use super::KrateName;
    use crate::error::{Error, InvalidKrateName, ReservedNameKind};

    /// Validates that all ways to create a krate name validate the basics of
    /// not empty and allowed characters
    #[test]
    fn rejects_simple() {
        assert!(matches!(
            TryInto::<KrateName<'_>>::try_into("").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::InvalidLength(0))
        ));
        assert!(matches!(
            KrateName::crates_io("").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::InvalidLength(0))
        ));
        assert!(matches!(
            TryInto::<KrateName<'_>>::try_into("no.pe").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::InvalidCharacter {
                index: 2,
                invalid: '.',
            })
        ));
        assert!(matches!(
            KrateName::crates_io("no.pe").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::InvalidCharacter {
                index: 2,
                invalid: '.',
            })
        ));
    }

    /// Validates that crate names can't start with digit
    #[test]
    fn rejects_leading_digit() {
        assert!(matches!(
            KrateName::crates_io("3nop").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::InvalidCharacter {
                index: 0,
                invalid: '3',
            })
        ));
    }

    /// Validates the crate name doesn't exceed the crates.io limit
    #[test]
    fn rejects_too_long() {
        assert!(matches!(
            KrateName::crates_io(
                "aaaaaaaabbbbbbbbccccccccddddddddaaaaaaaabbbbbbbbccccccccddddddddxxxxxxx"
            )
            .unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::InvalidLength(71))
        ));

        assert!(
            KrateName::cargo(
                "aaaaaaaabbbbbbbbccccccccddddddddaaaaaaaabbbbbbbbccccccccddddddddxxxxxxx"
            )
            .is_ok()
        );
    }

    /// Validates the crate name can't be a reserved name
    #[test]
    fn rejects_reserved() {
        assert!(matches!(
            KrateName::cargo("nul").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::ReservedName {
                reserved: "nul",
                kind: ReservedNameKind::Windows
            })
        ));
        assert!(matches!(
            KrateName::cargo("deps").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::ReservedName {
                reserved: "deps",
                kind: ReservedNameKind::Artifact
            })
        ));
        assert!(matches!(
            KrateName::cargo("Self").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::ReservedName {
                reserved: "Self",
                kind: ReservedNameKind::Keyword
            })
        ));
        assert!(matches!(
            KrateName::cargo("yield").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::ReservedName {
                reserved: "yield",
                kind: ReservedNameKind::Keyword
            })
        ));
        assert!(matches!(
            KrateName::cargo("proc-macro").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::ReservedName {
                reserved: "proc-macro",
                kind: ReservedNameKind::Standard
            })
        ));
        assert!(matches!(
            KrateName::cargo("proc_macro").unwrap_err(),
            Error::InvalidKrateName(InvalidKrateName::ReservedName {
                reserved: "proc_macro",
                kind: ReservedNameKind::Standard
            })
        ));
    }

    #[inline]
    fn rp(n: &str) -> String {
        KrateName(n).relative_path(Some('/'))
    }

    /// Validates we get the correct relative path to crate
    #[test]
    fn relative_path() {
        assert_eq!(rp("a"), "1/a");
        assert_eq!(rp("ab"), "2/ab");
        assert_eq!(rp("abc"), "3/a/abc");
        assert_eq!(rp("AbCd"), "ab/cd/abcd");
        assert_eq!(rp("normal"), "no/rm/normal");
        assert_eq!(rp("_boop-"), "_b/oo/_boop-");
        assert_eq!(rp("Inflector"), "in/fl/inflector");
    }
}
