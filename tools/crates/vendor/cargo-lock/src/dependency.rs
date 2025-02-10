//! Package dependencies

#[cfg(feature = "dependency-tree")]
pub mod graph;
#[cfg(feature = "dependency-tree")]
pub mod tree;

#[cfg(feature = "dependency-tree")]
pub use self::tree::Tree;

use crate::package::{Name, Package, SourceId};
use semver::Version;
use serde::{Deserialize, Serialize};
use std::fmt;

/// Package dependencies
#[derive(Clone, Debug, Deserialize, Eq, Hash, PartialEq, PartialOrd, Ord, Serialize)]
pub struct Dependency {
    /// Name of the dependency
    pub name: Name,

    /// Version of the dependency
    pub version: Version,

    /// Source identifier for the dependency
    pub source: Option<SourceId>,
}

impl Dependency {
    /// Does the given [`Package`] exactly match this `Dependency`?
    pub fn matches(&self, package: &Package) -> bool {
        self.name == package.name && self.version == package.version
    }
}

impl fmt::Display for Dependency {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} {}", &self.name, &self.version)?;

        if let Some(source) = &self.source {
            write!(f, " ({source})")?;
        }

        Ok(())
    }
}

impl From<&Package> for Dependency {
    /// Get the [`Dependency`] requirement for this `[[package]]`
    fn from(pkg: &Package) -> Dependency {
        Self {
            name: pkg.name.clone(),
            version: pkg.version.clone(),
            source: pkg
                .source
                .clone()
                .map(|x| x.normalize_git_source_for_dependency()),
        }
    }
}
