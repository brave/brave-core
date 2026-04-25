//! Advisory information (i.e. the `[advisory]` section)

use super::{
    category::Category, date::Date, id::Id, informational::Informational, keyword::Keyword,
};
use crate::advisory::license::License;
use crate::{SourceId, collection::Collection, package};
use cvss::Cvss;
use serde::{Deserialize, Serialize};
use url::Url;

/// The `[advisory]` section of a RustSec security advisory
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct Metadata {
    /// Security advisory ID (e.g. RUSTSEC-YYYY-NNNN)
    pub id: Id,

    /// Name of affected crate
    pub package: package::Name,

    /// One-liner description of a vulnerability
    #[serde(default)]
    pub title: String,

    /// Extended description of a vulnerability
    #[serde(default)]
    pub description: String,

    /// Date the underlying issue was reported
    pub date: Date,

    /// Advisory IDs in other databases which point to the same advisory
    #[serde(default)]
    pub aliases: Vec<Id>,

    /// Advisory IDs which are related to this advisory.
    /// (use `aliases` for the same vulnerability syndicated to other databases)
    #[serde(default)]
    pub related: Vec<Id>,

    /// Collection this advisory belongs to. This isn't intended to be
    /// explicitly specified in the advisory, but rather is auto-populated
    /// based on the location
    pub collection: Option<Collection>,

    /// RustSec vulnerability categories: one of a fixed list of vulnerability
    /// categorizations accepted by the project.
    #[serde(default)]
    pub categories: Vec<Category>,

    /// Freeform keywords which succinctly describe this vulnerability (e.g. "ssl", "rce", "xss")
    #[serde(default)]
    pub keywords: Vec<Keyword>,

    /// CVSS v3.1 Base Metrics vector string containing severity information.
    ///
    /// Example:
    ///
    /// ```text
    /// CVSS:3.1/AV:N/AC:L/PR:N/UI:R/S:C/C:L/I:L/A:N
    /// ```
    pub cvss: Option<Cvss>,

    /// Informational advisories can be used to warn users about issues
    /// affecting a particular crate without failing the build.
    pub informational: Option<Informational>,

    /// Additional reference URLs with more information related to this advisory
    #[serde(default)]
    pub references: Vec<Url>,

    /// Source URL where the vulnerable package is located/published.
    ///
    /// Defaults to crates.io, i.e. `registry+https://github.com/rust-lang/crates.io-index`
    pub source: Option<SourceId>,

    /// URL with an announcement (e.g. blog post, PR, disclosure issue, CVE)
    pub url: Option<Url>,

    /// Was this advisory (i.e. itself, regardless of the crate) withdrawn?
    /// If yes, when?
    ///
    /// This can be used to soft-delete advisories which were filed in error.
    #[serde(default)]
    pub withdrawn: Option<Date>,

    /// License under which the advisory content is available
    #[serde(default)]
    pub license: License,
}
