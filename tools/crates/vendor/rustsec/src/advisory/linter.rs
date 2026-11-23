//! Advisory linter: ensure advisories are well-formed according to the
//! currently valid set of fields.
//!
//! This is run in CI at the time advisories are submitted.

use std::borrow::Cow;
use std::str::FromStr;
use std::{fmt, path::Path};

use platforms::Arch;

use super::{Advisory, Category, parts};
use crate::advisory::Informational;
use crate::advisory::license::License;
use crate::fs;

/// Lint information about a particular advisory
#[derive(Debug)]
pub struct Linter {
    /// Advisory being linted
    advisory: Advisory,

    /// Errors detected during linting
    errors: Vec<Error>,
}

impl Linter {
    /// Lint the advisory TOML file located at the given path
    pub fn lint_file<P: AsRef<Path>>(path: P) -> Result<Self, crate::Error> {
        let path = path.as_ref();

        match path.extension().and_then(|ext| ext.to_str()) {
            Some("md") => (),
            other => fail!(
                crate::error::ErrorKind::Parse,
                "invalid advisory file extension: {}",
                other.unwrap_or("(missing)")
            ),
        }

        let advisory_data = fs::read_to_string(path).map_err(|e| {
            crate::Error::with_source(
                crate::error::ErrorKind::Io,
                format!("couldn't open {}", path.display()),
                e,
            )
        })?;

        Self::lint_string(&advisory_data, Some(Advisory::is_draft(path)))
    }

    /// Lint the given advisory data
    pub fn lint_string(s: &str, draft: Option<bool>) -> Result<Self, crate::Error> {
        // Ensure the advisory parses according to the normal parser first
        let advisory = s.parse::<Advisory>()?;

        // Get advisory "front matter" (TOML formatted)
        let advisory_parts = parts::Parts::parse(s)?;
        let front_matter = advisory_parts
            .front_matter
            .parse::<toml::Table>()
            .map_err(crate::Error::from_toml)?;

        let mut linter = Self {
            advisory,
            errors: vec![],
        };

        // Lint against user mentions to avoid spamming
        if advisory_parts.markdown.contains("@") {
            let front_matter_lines = advisory_parts.front_matter.lines().count();
            for (i, line) in advisory_parts.markdown.lines().enumerate() {
                let mut found = None;
                for (pi, part) in line.split_inclusive("@").enumerate() {
                    if pi == 0 {
                        continue;
                    }

                    if !part.starts_with(|s: char| s.is_ascii_alphanumeric()) {
                        continue;
                    }

                    let Some(username) = part.split_whitespace().next() else {
                        continue;
                    };

                    if username.contains(['.', '@', '`']) {
                        continue;
                    }

                    found = Some(username.trim_end_matches([')']));
                    break;
                }

                let Some(user) = found else {
                    continue;
                };

                linter.errors.push(Error {
                    kind: ErrorKind::Malformed,
                    section: Some("markdown"),
                    message: Some(
                        format!(
                            "bare @{user} in advisory on line {} (use explicit link to avoid spam)",
                            front_matter_lines + 4 + i
                        )
                        .into(),
                    ),
                });
            }
        }

        linter.lint_advisory(&front_matter, draft);
        Ok(linter)
    }

    /// Get the parsed advisory
    pub fn advisory(&self) -> &Advisory {
        &self.advisory
    }

    /// Get the errors that occurred during linting
    pub fn errors(&self) -> &[Error] {
        self.errors.as_slice()
    }

    /// Lint the provided TOML value as the toplevel table of an advisory
    fn lint_advisory(&mut self, advisory: &toml::Table, draft: Option<bool>) {
        for (key, value) in advisory {
            match key.as_str() {
                "advisory" => self.lint_metadata(value, draft),
                "versions" => self.lint_versions(value),
                "affected" => self.lint_affected(value),
                _ => self.errors.push(Error {
                    kind: ErrorKind::key(key),
                    section: None,
                    message: None,
                }),
            }
        }
    }

    /// Lint the `[advisory]` metadata section
    fn lint_metadata(&mut self, metadata: &toml::Value, draft: Option<bool>) {
        let mut year = None;

        if let Some(table) = metadata.as_table() {
            for (key, value) in table {
                match key.as_str() {
                    "id" => {
                        if let Some(draft) = draft
                            && self.advisory.metadata.id.is_placeholder() != draft
                        {
                            self.errors.push(Error {
                                kind: ErrorKind::value("id", value.to_string()),
                                section: Some("advisory"),
                                message: Some(
                                    "advisory ID draft status does not match file name".into(),
                                ),
                            });
                        }
                        if self.advisory.metadata.id.is_other() {
                            self.errors.push(Error {
                                kind: ErrorKind::value("id", value.to_string()),
                                section: Some("advisory"),
                                message: Some("unknown advisory ID type".into()),
                            });
                        } else if let Some(y1) = self.advisory.metadata.id.year() {
                            // Exclude CVE IDs, since the year from CVE ID may not match the report date
                            if !self.advisory.metadata.id.is_cve() {
                                if let Some(y2) = year {
                                    if y1 != y2 {
                                        self.errors.push(Error {
                                            kind: ErrorKind::value("id", value.to_string()),
                                            section: Some("advisory"),
                                            message: Some(
                                                "year in advisory ID does not match date".into(),
                                            ),
                                        });
                                    }
                                } else {
                                    year = Some(y1);
                                }
                            }
                        }
                    }
                    "categories" => {
                        for category in &self.advisory.metadata.categories {
                            if let Category::Other(other) = category {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("category", other.to_string()),
                                    section: Some("advisory"),
                                    message: Some(format!("unknown category {other:?}").into()),
                                });
                            }
                        }
                    }
                    "collection" => self.errors.push(Error {
                        kind: ErrorKind::Malformed,
                        section: Some("advisory"),
                        message: Some(
                            "collection shouldn't be explicit; inferred by location".into(),
                        ),
                    }),
                    "informational" => {
                        let informational = self
                            .advisory
                            .metadata
                            .informational
                            .as_ref()
                            .expect("parsed informational");

                        if let Informational::Other(key) = informational {
                            self.errors.push(Error {
                                kind: ErrorKind::value("informational", informational.as_str()),
                                section: Some("advisory"),
                                message: Some(
                                    format!("unknown informational advisory type {key:?}").into(),
                                ),
                            });
                        }
                    }
                    "url" => {
                        let Some(url) = value.as_str() else {
                            continue;
                        };

                        if !url.starts_with("https://") {
                            self.errors.push(Error {
                                kind: ErrorKind::value("url", value.to_string()),
                                section: Some("advisory"),
                                message: Some("URL must start with https://".into()),
                            });
                        }

                        // The GHSA URL can be derived from the alias, so it is redundant here
                        for alias in &self.advisory.metadata.aliases {
                            if alias.is_ghsa() && url.contains(alias.as_str()) {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("url", value.to_string()),
                                    section: Some("advisory"),
                                    message: Some(
                                        format!("URL refers to GHSA alias {alias}; refer to an issue, PR or commit instead").into(),
                                    ),
                                });
                            }
                        }
                    }
                    "references" => {
                        let Some(references) = value.as_array() else {
                            continue;
                        };

                        // GHSA URLs can be derived from the alias, so they are redundant here
                        for reference in references {
                            let Some(url) = reference.as_str() else {
                                continue;
                            };

                            for alias in &self.advisory.metadata.aliases {
                                if alias.is_ghsa() && url.contains(alias.as_str()) {
                                    self.errors.push(Error {
                                        kind: ErrorKind::value("references", reference.to_string()),
                                        section: Some("advisory"),
                                        message: Some(
                                            format!("reference to GHSA alias {alias} is redundant")
                                                .into(),
                                        ),
                                    });
                                }
                            }
                        }
                    }
                    "date" => {
                        let y1 = self.advisory.metadata.date.year();

                        if let Some(y2) = year {
                            if y1 != y2 {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("date", value.to_string()),
                                    section: Some("advisory"),
                                    message: Some("year in advisory ID does not match date".into()),
                                });
                            }
                        } else {
                            year = Some(y1);
                        }
                    }
                    "yanked" => {
                        if self.advisory.metadata.withdrawn.is_none() {
                            self.errors.push(Error {
                                kind: ErrorKind::Malformed,
                                section: Some("metadata"),
                                message: Some(
                                    "field `yanked` is deprecated, use `withdrawn` field instead"
                                        .into(),
                                ),
                            });
                        }
                    }
                    "license" => {
                        if let Some(l) = value.as_str() {
                            // We don't want to accept any license, only explicitly accepted ones
                            let unknown_license =
                                matches!(License::from_str(l).unwrap(), License::Other(_));
                            if unknown_license {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("license", l.to_string()),
                                    section: Some("advisory"),
                                    message: Some(format!("unknown license {l:?}").into()),
                                });
                            }
                        }
                    }
                    "aliases" | "cvss" | "keywords" | "package" | "related" | "title"
                    | "withdrawn" | "description" | "expect-deleted" => (),
                    _ => self.errors.push(Error {
                        kind: ErrorKind::key(key),
                        section: Some("advisory"),
                        message: None,
                    }),
                }
            }
        } else {
            self.errors.push(Error {
                kind: ErrorKind::Malformed,
                section: Some("advisory"),
                message: Some("expected table".into()),
            });
        }
    }

    /// Lint the `[versions]` section of an advisory
    fn lint_versions(&mut self, versions: &toml::Value) {
        if let Some(table) = versions.as_table() {
            for (key, _) in table {
                match key.as_str() {
                    "patched" | "unaffected" => (),
                    _ => self.errors.push(Error {
                        kind: ErrorKind::key(key),
                        section: Some("versions"),
                        message: None,
                    }),
                }
            }
        }
    }

    /// Lint the `[affected]` section of an advisory
    fn lint_affected(&mut self, affected: &toml::Value) {
        if let Some(table) = affected.as_table() {
            for (key, _) in table {
                match key.as_str() {
                    "functions" => {
                        for function in self.advisory.affected.as_ref().unwrap().functions.keys() {
                            // Rust identifiers do not allow '-' character but crate names do,
                            // thus "crate-name" would be addressed as "crate_name" in function path
                            let crate_name =
                                self.advisory.metadata.package.as_str().replace('-', "_");
                            if function.segments()[0].as_str() != crate_name {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("functions", function.to_string()),
                                    section: Some("affected"),
                                    message: Some(
                                        "function path must start with crate name".into(),
                                    ),
                                });
                            }
                        }
                    }
                    "arch" => {
                        for arch in &self.advisory.affected.as_ref().unwrap().arch {
                            if arch.is_empty() || !Arch::from_str(arch).is_ok() {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("arch", arch.to_string()),
                                    section: Some("affected"),
                                    message: Some(format!("unknown architecture {arch:?}").into()),
                                });
                            }
                        }
                    }
                    "os" => {
                        for os in &self.advisory.affected.as_ref().unwrap().os {
                            if os.is_empty() {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("os", os.to_string()),
                                    section: Some("affected"),
                                    message: Some("unknown OS".into()),
                                });
                            }
                        }
                    }
                    _ => self.errors.push(Error {
                        kind: ErrorKind::key(key),
                        section: Some("affected"),
                        message: None,
                    }),
                }
            }
        }
    }
}

/// Lint errors
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Error {
    /// Kind of error
    kind: ErrorKind,

    /// Section of the advisory where the error occurred
    section: Option<&'static str>,

    /// Message about why it's invalid
    message: Option<Cow<'static, str>>,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.kind)?;

        if let Some(section) = &self.section {
            write!(f, " in [{section}]")?;
        } else {
            write!(f, " in toplevel")?;
        }

        if let Some(msg) = &self.message {
            write!(f, ": {msg}")?
        }

        Ok(())
    }
}

impl std::error::Error for Error {}

/// Lint errors
#[derive(Clone, Debug, Eq, PartialEq)]
#[non_exhaustive]
pub(crate) enum ErrorKind {
    /// Advisory is structurally malformed
    Malformed,

    /// Unknown key
    InvalidKey {
        /// Name of the key
        name: String,
    },

    /// Unknown value
    InvalidValue {
        /// Name of the key
        name: String,

        /// Invalid value
        value: String,
    },
}

impl ErrorKind {
    /// Invalid key
    fn key(name: &str) -> Self {
        Self::InvalidKey {
            name: name.to_owned(),
        }
    }

    /// Invalid value
    fn value(name: &str, value: impl Into<String>) -> Self {
        Self::InvalidValue {
            name: name.to_owned(),
            value: value.into(),
        }
    }
}

impl fmt::Display for ErrorKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Malformed => write!(f, "malformed content"),
            Self::InvalidKey { name } => write!(f, "invalid key `{name}`"),
            Self::InvalidValue { name, value } => {
                write!(f, "invalid value `{value}` for key `{name}`")
            }
        }
    }
}
