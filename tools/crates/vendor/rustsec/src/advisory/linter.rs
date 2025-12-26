//! Advisory linter: ensure advisories are well-formed according to the
//! currently valid set of fields.
//!
//! This is run in CI at the time advisories are submitted.

use super::{Advisory, Category, parts};
use crate::advisory::license::License;
use crate::fs;
use std::str::FromStr;
use std::{fmt, path::Path};

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
                crate::ErrorKind::Parse,
                "invalid advisory file extension: {}",
                other.unwrap_or("(missing)")
            ),
        }

        let advisory_data = fs::read_to_string(path).map_err(|e| {
            crate::Error::with_source(
                crate::ErrorKind::Io,
                format!("couldn't open {}", path.display()),
                e,
            )
        })?;

        Self::lint_string(&advisory_data)
    }

    /// Lint the given advisory data
    pub fn lint_string(s: &str) -> Result<Self, crate::Error> {
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

        linter.lint_advisory(&front_matter);
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
    fn lint_advisory(&mut self, advisory: &toml::Table) {
        for (key, value) in advisory {
            match key.as_str() {
                "advisory" => self.lint_metadata(value),
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
    fn lint_metadata(&mut self, metadata: &toml::Value) {
        let mut year = None;

        if let Some(table) = metadata.as_table() {
            for (key, value) in table {
                match key.as_str() {
                    "id" => {
                        if self.advisory.metadata.id.is_other() {
                            self.errors.push(Error {
                                kind: ErrorKind::value("id", value.to_string()),
                                section: Some("advisory"),
                                message: Some("unknown advisory ID type"),
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
                                                "year in advisory ID does not match date",
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
                                    message: Some("unknown category"),
                                });
                            }
                        }
                    }
                    "collection" => self.errors.push(Error {
                        kind: ErrorKind::Malformed,
                        section: Some("advisory"),
                        message: Some("collection shouldn't be explicit; inferred by location"),
                    }),
                    "informational" => {
                        let informational = self
                            .advisory
                            .metadata
                            .informational
                            .as_ref()
                            .expect("parsed informational");

                        if informational.is_other() {
                            self.errors.push(Error {
                                kind: ErrorKind::value("informational", informational.as_str()),
                                section: Some("advisory"),
                                message: Some("unknown informational advisory type"),
                            });
                        }
                    }
                    "url" => {
                        if let Some(url) = value.as_str() {
                            if !url.starts_with("https://") {
                                self.errors.push(Error {
                                    kind: ErrorKind::value("url", value.to_string()),
                                    section: Some("advisory"),
                                    message: Some("URL must start with https://"),
                                });
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
                                    message: Some("year in advisory ID does not match date"),
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
                                    "Field `yanked` is deprecated, use `withdrawn` field instead",
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
                                    message: Some("Unknown license"),
                                });
                            }
                        }
                    }
                    "aliases" | "cvss" | "keywords" | "package" | "references" | "related"
                    | "title" | "withdrawn" | "description" => (),
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
                message: Some("expected table"),
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
                                    message: Some("function path must start with crate name"),
                                });
                            }
                        }
                    }
                    "arch" | "os" => (),
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
    message: Option<&'static str>,
}

impl Error {
    /// Get the kind of error
    pub fn kind(&self) -> &ErrorKind {
        &self.kind
    }

    /// Get the section of the advisory where the error occurred
    pub fn section(&self) -> Option<&str> {
        self.section.as_ref().map(AsRef::as_ref)
    }

    /// Get an optional message about the lint failure
    pub fn message(&self) -> Option<&str> {
        self.message.as_ref().map(AsRef::as_ref)
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", &self.kind)?;

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

/// Lint errors
#[derive(Clone, Debug, Eq, PartialEq)]
#[non_exhaustive]
pub enum ErrorKind {
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
    pub fn key(name: &str) -> Self {
        ErrorKind::InvalidKey {
            name: name.to_owned(),
        }
    }

    /// Invalid value
    pub fn value(name: &str, value: impl Into<String>) -> Self {
        ErrorKind::InvalidValue {
            name: name.to_owned(),
            value: value.into(),
        }
    }
}

impl fmt::Display for ErrorKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ErrorKind::Malformed => write!(f, "malformed content"),
            ErrorKind::InvalidKey { name } => write!(f, "invalid key `{name}`"),
            ErrorKind::InvalidValue { name, value } => {
                write!(f, "invalid value `{value}` for key `{name}`")
            }
        }
    }
}
