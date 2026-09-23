//! Security advisories in the RustSec database

pub mod affected;
mod category;
mod date;
mod id;
mod informational;
mod keyword;
mod license;
pub mod linter;
mod metadata;
mod parts;
pub(crate) mod versions;

pub use self::{
    affected::Affected,
    category::Category,
    date::Date,
    id::{Id, IdKind},
    informational::Informational,
    keyword::Keyword,
    license::License,
    linter::Linter,
    metadata::Metadata,
    parts::Parts,
    versions::Versions,
};
pub use cvss::Severity;

use crate::{
    error::{Error, ErrorKind},
    fs,
};
use serde::{Deserialize, Serialize};
use std::{ffi::OsStr, path::Path, str::FromStr};

/// RustSec Security Advisories
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct Advisory {
    /// The `[advisory]` section of a RustSec advisory
    #[serde(rename = "advisory")]
    pub metadata: Metadata,

    /// The (optional) `[affected]` section of a RustSec advisory
    pub affected: Option<Affected>,

    /// Versions related to this advisory which are patched or unaffected.
    pub versions: Versions,
}

impl Advisory {
    /// Load an advisory from a `RUSTSEC-20XX-NNNN.md` file
    pub fn load_file(path: impl AsRef<Path>) -> Result<Self, Error> {
        let path = path.as_ref();

        let advisory_data = fs::read_to_string(path).map_err(|e| {
            Error::with_source(
                ErrorKind::Io,
                format!("couldn't open {}", path.display()),
                e,
            )
        })?;

        advisory_data.parse().map_err(|e| {
            Error::with_source(
                ErrorKind::Parse,
                format!("error parsing {}", path.display()),
                e,
            )
        })
    }

    /// Get advisory ID
    pub fn id(&self) -> &Id {
        &self.metadata.id
    }

    /// Get advisory title
    pub fn title(&self) -> &str {
        self.metadata.title.as_ref()
    }

    /// Get advisory description
    pub fn description(&self) -> &str {
        self.metadata.description.as_ref()
    }

    /// Get the date the underlying issue was reported on
    pub fn date(&self) -> &Date {
        &self.metadata.date
    }

    /// Get the severity of this advisory if it has a CVSS v3 associated
    pub fn severity(&self) -> Option<Severity> {
        self.metadata.cvss.as_ref().map(|cvss| cvss.severity())
    }

    /// Whether the advisory has been withdrawn, i.e. soft-deleted
    pub fn withdrawn(&self) -> bool {
        self.metadata.withdrawn.is_some()
    }

    /// Whether the given `path` represents a draft advisory
    pub fn is_draft(path: &Path) -> bool {
        matches!(
            path.file_name().and_then(OsStr::to_str),
            Some(name) if name.starts_with("RUSTSEC-0000-0000."),
        )
    }
}

impl FromStr for Advisory {
    type Err = Error;

    fn from_str(advisory_data: &str) -> Result<Self, Error> {
        let parts = Parts::parse(advisory_data)?;

        // V4 advisories omit the leading `[advisory]` TOML table
        let front_matter = if parts.front_matter.starts_with("[advisory]") {
            parts.front_matter.to_owned()
        } else {
            String::from("[advisory]\n") + parts.front_matter
        };

        let mut advisory: Self = toml::from_str(&front_matter).map_err(Error::from_toml)?;

        if !advisory.metadata.title.is_empty() {
            fail!(
                ErrorKind::Parse,
                "invalid `title` attribute in advisory TOML"
            );
        }

        if !advisory.metadata.description.is_empty() {
            fail!(
                ErrorKind::Parse,
                "invalid `description` attribute in advisory TOML"
            );
        }

        #[allow(clippy::assigning_clones)]
        {
            advisory.metadata.title = parts.title.to_owned();
            advisory.metadata.description = parts.description.to_owned();
        }

        Ok(advisory)
    }
}
