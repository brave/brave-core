//! The configuration file

use rustsec::{
    advisory,
    platforms::target::{Arch, OS},
    report, Error, ErrorKind, WarningKind,
};
use serde::{Deserialize, Serialize};
use std::{path::PathBuf, str::FromStr};

/// `cargo audit` configuration:
///
/// An optional TOML config file located in `~/.cargo/audit.toml` or
/// `.cargo/audit.toml`.
#[derive(Clone, Debug, Default, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct AuditConfig {
    /// Advisory-related configuration
    #[serde(default)]
    pub advisories: AdvisoryConfig,

    /// Advisory Database configuration
    #[serde(default)]
    pub database: DatabaseConfig,

    /// Output configuration
    #[serde(default)]
    pub output: OutputConfig,

    /// Target-related configuration
    #[serde(default)]
    pub target: TargetConfig,

    /// Configuration for auditing for yanked crates
    #[serde(default)]
    pub yanked: YankedConfig,
}

impl AuditConfig {
    /// Get audit report settings from the configuration
    pub fn report_settings(&self) -> report::Settings {
        let mut settings = report::Settings {
            ignore: self.advisories.ignore.clone(),
            severity: self.advisories.severity_threshold,
            target_arch: self.target.arch(),
            target_os: self.target.os(),
            ..Default::default()
        };

        if let Some(informational_warnings) = &self.advisories.informational_warnings {
            settings
                .informational_warnings
                .clone_from(informational_warnings);
        } else {
            // Alert for all informational packages by default
            settings.informational_warnings = vec![
                advisory::Informational::Unmaintained,
                advisory::Informational::Unsound,
                advisory::Informational::Notice,
            ];
        }

        // Enable warnings for all informational advisories if they are marked deny.
        // Deny only works on the output if a corresponding advisory is found but only the
        // informational categories listed in informational_warnings are reported.
        // This means that if "Unsound" is missing from informational_warnings then deny unsound
        // will not do anything.
        // To fix this always add the corresponding warning category
        let mut insert_if_not_present = |warning| {
            if !settings.informational_warnings.contains(&warning) {
                settings.informational_warnings.push(warning);
            }
        };

        for deny in &self.output.deny {
            match deny {
                DenyOption::Warnings => {
                    insert_if_not_present(advisory::Informational::Notice);
                    insert_if_not_present(advisory::Informational::Unmaintained);
                    insert_if_not_present(advisory::Informational::Unsound);
                    break;
                }
                DenyOption::Unmaintained => {
                    insert_if_not_present(advisory::Informational::Unmaintained)
                }
                DenyOption::Unsound => insert_if_not_present(advisory::Informational::Unsound),
                DenyOption::Yanked => continue,
            };
        }

        settings
    }
}

/// Advisory-related configuration.
#[derive(Clone, Debug, Default, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct AdvisoryConfig {
    /// Ignore advisories for the given IDs
    #[serde(default)]
    pub ignore: Vec<advisory::Id>,

    /// Ignore the source of this advisory, matching any package of the same name.
    #[serde(default)]
    pub ignore_source: bool,

    /// Warn for the given types of informational advisories
    pub informational_warnings: Option<Vec<advisory::Informational>>,

    /// CVSS Qualitative Severity Rating Scale threshold to alert at.
    ///
    /// Vulnerabilities with explicit CVSS info which have a severity below
    /// this threshold will be ignored.
    pub severity_threshold: Option<advisory::Severity>,
}

/// Advisory Database configuration.
///
/// The advisory database is stored in a Git repository. This section of the
/// configuration stores settings related to it.
#[derive(Clone, Debug, Default, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct DatabaseConfig {
    /// Path to the local copy of advisory database's git repo (default: ~/.cargo/advisory-db)
    pub path: Option<PathBuf>,

    /// URL to the advisory database's git repo (default: <https://github.com/RustSec/advisory-db>)
    pub url: Option<String>,

    /// Perform a `git fetch` before auditing (default: true)
    pub fetch: bool,

    /// Allow a stale advisory database? (i.e. one which hasn't been updated in 90 days)
    pub stale: bool,
}

/// Output configuration
#[derive(Clone, Debug, Default, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct OutputConfig {
    /// Disallow advisories which trigger warnings
    #[serde(default)]
    pub deny: Vec<DenyOption>,

    /// Output format to use
    #[serde(default)]
    pub format: OutputFormat,

    /// Enable quiet mode
    pub quiet: bool,

    /// Show inverse dependency trees along with advisories (default: true)
    pub show_tree: Option<bool>,
}

impl OutputConfig {
    /// Is quiet mode enabled?
    pub fn is_quiet(&self) -> bool {
        self.quiet || self.format == OutputFormat::Json
    }
}

/// Warning kinds
#[derive(Copy, Clone, Debug, Deserialize, Eq, Hash, PartialEq, PartialOrd, Serialize, Ord)]
pub enum DenyOption {
    /// Deny all warnings
    #[serde(rename = "warnings")]
    Warnings,

    /// Deny unmaintained dependency warnings
    #[serde(rename = "unmaintained")]
    Unmaintained,

    /// Deny unsound dependency warnings
    #[serde(rename = "unsound")]
    Unsound,

    /// Deny yanked dependency warnings
    #[serde(rename = "yanked")]
    Yanked,
}

impl DenyOption {
    /// Get all of the possible warnings to be denied
    pub fn all() -> Vec<Self> {
        vec![
            DenyOption::Warnings,
            DenyOption::Unmaintained,
            DenyOption::Unsound,
            DenyOption::Yanked,
        ]
    }
    /// Get the warning::Kind that corresponds to self, if applicable
    pub fn get_warning_kind(self) -> &'static [WarningKind] {
        match self {
            DenyOption::Warnings => &[
                WarningKind::Unmaintained,
                WarningKind::Unsound,
                WarningKind::Yanked,
            ],
            DenyOption::Unmaintained => &[WarningKind::Unmaintained],
            DenyOption::Unsound => &[WarningKind::Unsound],
            DenyOption::Yanked => &[WarningKind::Yanked],
        }
    }
}

impl FromStr for DenyOption {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        match s {
            "warnings" => Ok(DenyOption::Warnings),
            "unmaintained" => Ok(DenyOption::Unmaintained),
            "unsound" => Ok(DenyOption::Unsound),
            "yanked" => Ok(DenyOption::Yanked),
            other => Err(Error::new(
                ErrorKind::Parse,
                &format!("invalid deny option: {}", other),
            )),
        }
    }
}

/// Output format
#[derive(Default, Copy, Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub enum OutputFormat {
    /// Display JSON
    #[serde(rename = "json")]
    Json,

    /// Display human-readable output to the terminal
    #[serde(rename = "terminal")]
    #[default]
    Terminal,
}

/// Helper enum for configuring filter values
///
/// This enum exists for backwards compatibility reasons.
/// In `cargo-audit` versions `<= 0.20.0` target's config
/// options `arch` and `os` were deserialized from a single
/// string. But following next minor release those values can be
/// configured as a list. Serde's untagged enum provides
/// dispatch that is backwards compatible.
#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(untagged)]
pub enum FilterList<T> {
    /// Legacy, single filter value
    Single(T),
    /// List of filters
    Many(Vec<T>),
}

/// Target configuration
#[derive(Clone, Debug, Default, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct TargetConfig {
    /// Target architecture to find vulnerabilities for
    pub arch: Option<FilterList<Arch>>,

    /// Target OS to find vulnerabilities for
    pub os: Option<FilterList<OS>>,
}

impl TargetConfig {
    /// Returns list of configured target architectures, cloning if needed
    pub fn arch(&self) -> Vec<Arch> {
        match &self.arch {
            Some(FilterList::Single(single)) => vec![*single],
            Some(FilterList::Many(many)) => many.clone(),
            None => vec![],
        }
    }

    /// Returns list of configured target operating systems, cloning if needed
    pub fn os(&self) -> Vec<OS> {
        match &self.os {
            Some(FilterList::Single(single)) => vec![*single],
            Some(FilterList::Many(many)) => many.clone(),
            None => vec![],
        }
    }
}

/// Configuration for auditing for yanked crates
#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct YankedConfig {
    /// Is auditing for yanked crates enabled?
    #[serde(default = "default_true")]
    pub enabled: bool,

    /// Should the crates.io index be updated before checking for yanked crates?
    #[serde(default = "default_true")]
    pub update_index: bool,
}

impl Default for YankedConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            update_index: true,
        }
    }
}

/// Helper function for returning a default of `true`
fn default_true() -> bool {
    true
}
