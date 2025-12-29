//! OSV advisories.
//!
//! It implements the parts of the [OSV schema](https://ossf.github.io/osv-schema) required for
//! RustSec.

use super::ranges_for_advisory;
use crate::advisory::Versions;
use crate::{
    Advisory,
    advisory::{Affected, Category, Id, Informational, affected::FunctionPath},
    repository::git::{GitModificationTimes, GitPath},
};
use cvss::Cvss;
use serde::{Deserialize, Deserializer, Serialize};
use std::str::FromStr;
use url::Url;

const ECOSYSTEM: &str = "crates.io";

/// Security advisory in the format defined by <https://github.com/google/osv>
#[derive(Debug, Clone, Serialize, Deserialize)]
#[cfg_attr(docsrs, doc(cfg(feature = "osv-export")))]
pub struct OsvAdvisory {
    #[serde(skip_serializing_if = "Option::is_none")]
    schema_version: Option<semver::Version>,
    id: Id,
    modified: String,  // maybe add an rfc3339 newtype?
    published: String, // maybe add an rfc3339 newtype?
    #[serde(skip_serializing_if = "Option::is_none")]
    withdrawn: Option<String>, // maybe add an rfc3339 newtype?
    #[serde(default)]
    aliases: Vec<Id>,
    #[serde(default)]
    related: Vec<Id>,
    summary: String,
    details: String,
    #[serde(default)]
    severity: Vec<OsvSeverity>,
    #[serde(default)]
    affected: Vec<OsvAffected>,
    #[serde(default)]
    references: Vec<OsvReference>,
    #[serde(default)]
    database_specific: MainOsvDatabaseSpecific,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OsvPackage {
    /// Set to a constant identifying crates.io
    pub(crate) ecosystem: String,
    /// Crate name
    pub(crate) name: String,
    /// https://github.com/package-url/purl-spec derived from the other two
    #[serde(default)]
    purl: Option<String>,
}

impl From<&cargo_lock::Name> for OsvPackage {
    fn from(package: &cargo_lock::Name) -> Self {
        OsvPackage {
            ecosystem: ECOSYSTEM.to_string(),
            name: package.to_string(),
            purl: Some("pkg:cargo/".to_string() + package.as_str()),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[allow(non_camel_case_types)]
#[serde(tag = "type", content = "score")]
pub enum OsvSeverity {
    CVSS_V3(cvss::v3::Base),
    CVSS_V4(cvss::v4::Vector),
}

impl TryFrom<Cvss> for OsvSeverity {
    type Error = &'static str;

    fn try_from(cvss: Cvss) -> Result<Self, Self::Error> {
        match cvss {
            Cvss::CvssV30(base) => Ok(OsvSeverity::CVSS_V3(base)),
            Cvss::CvssV31(base) => Ok(OsvSeverity::CVSS_V3(base)),
            Cvss::CvssV40(vector) => Ok(OsvSeverity::CVSS_V4(vector)),
            _ => unreachable!(),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OsvAffected {
    pub(crate) package: OsvPackage,
    ecosystem_specific: Option<OsvEcosystemSpecific>,
    database_specific: OsvDatabaseSpecific,
    ranges: Option<Vec<OsvJsonRange>>,
    // FIXME deserialize with deserialize_semver_compat
    versions: Option<Vec<String>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OsvJsonRange {
    // 'type' is a reserved keyword in Rust
    #[serde(rename = "type")]
    kind: String,
    events: Vec<OsvTimelineEvent>,
    // 'repo' field is not used because we don't track or export git commit data
}

impl OsvJsonRange {
    /// Generates the timeline of the bug being introduced and fixed for the
    /// [`affected[].ranges[].events`](https://github.com/ossf/osv-schema/blob/main/schema.md#affectedrangesevents-fields) field.
    fn new(versions: &Versions) -> Self {
        let ranges = ranges_for_advisory(versions);
        assert!(!ranges.is_empty()); // zero ranges means nothing is affected, so why even have an advisory?
        let mut timeline = Vec::new();
        for range in ranges {
            match range.introduced {
                Some(ver) => timeline.push(OsvTimelineEvent::Introduced(ver)),
                None => timeline.push(OsvTimelineEvent::Introduced(
                    semver::Version::parse("0.0.0-0").unwrap(),
                )),
            }
            #[allow(clippy::single_match)]
            match range.fixed {
                Some(ver) => timeline.push(OsvTimelineEvent::Fixed(ver)),
                None => (), // "everything after 'introduced' is affected" is implicit in OSV
            }
        }

        Self {
            kind: "SEMVER".to_string(),
            events: timeline,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum OsvTimelineEvent {
    #[serde(rename = "introduced")]
    #[serde(deserialize_with = "deserialize_semver_compat")]
    Introduced(semver::Version),
    #[serde(rename = "fixed")]
    #[serde(deserialize_with = "deserialize_semver_compat")]
    Fixed(semver::Version),
    #[serde(rename = "last_affected")]
    #[serde(deserialize_with = "deserialize_semver_compat")]
    LastAffected(semver::Version),
}

fn deserialize_semver_compat<'de, D>(deserializer: D) -> Result<semver::Version, D::Error>
where
    D: Deserializer<'de>,
{
    let mut ver = String::deserialize(deserializer)?;
    match ver.matches('.').count() {
        0 => ver.push_str(".0.0"),
        1 => ver.push_str(".0"),
        _ => (),
    }
    semver::Version::from_str(&ver).map_err(serde::de::Error::custom)
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OsvReference {
    // 'type' is a reserved keyword in Rust
    #[serde(rename = "type")]
    pub kind: OsvReferenceKind,
    pub url: Url,
}

impl From<Url> for OsvReference {
    fn from(url: Url) -> Self {
        OsvReference {
            kind: guess_url_kind(&url),
            url,
        }
    }
}

#[allow(clippy::upper_case_acronyms)]
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum OsvReferenceKind {
    ADVISORY,
    #[allow(dead_code)]
    ARTICLE,
    REPORT,
    #[allow(dead_code)]
    FIX,
    PACKAGE,
    WEB,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OsvEcosystemSpecific {
    affects: Option<OsvEcosystemSpecificAffected>,
    affected_functions: Option<Vec<FunctionPath>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OsvEcosystemSpecificAffected {
    arch: Vec<platforms::target::Arch>,
    os: Vec<platforms::target::OS>,
    /// We include function names only in order to allow changing
    /// the way versions are specified without an API break
    functions: Vec<FunctionPath>,
}

impl From<Affected> for OsvEcosystemSpecificAffected {
    fn from(a: Affected) -> Self {
        OsvEcosystemSpecificAffected {
            arch: a.arch,
            os: a.os,
            functions: a.functions.into_keys().collect(),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OsvDatabaseSpecific {
    #[serde(default)]
    categories: Vec<Category>,
    cvss: Option<Cvss>,
    informational: Option<Informational>,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct MainOsvDatabaseSpecific {
    #[serde(default)]
    license: Option<String>,
}

impl OsvAdvisory {
    /// Advisory ID
    pub fn id(&self) -> &Id {
        &self.id
    }

    /// Publication date
    pub fn published(&self) -> &str {
        &self.published
    }

    /// Converts a single RustSec advisory to OSV format.
    /// `path` is the path to the advisory file. It must be relative to the git repository root.
    pub fn from_rustsec(
        advisory: Advisory,
        mod_times: &GitModificationTimes,
        path: GitPath<'_>,
    ) -> Self {
        let metadata = advisory.metadata;

        // Assemble the URLs to put into 'references' field
        let mut reference_urls: Vec<Url> = Vec::new();
        // link to the package on crates.io
        let package_url = "https://crates.io/crates/".to_owned() + metadata.package.as_str();
        reference_urls.push(Url::parse(&package_url).unwrap());
        // link to human-readable RustSec advisory
        let advisory_url = format!(
            "https://rustsec.org/advisories/{}.html",
            metadata.id.as_str()
        );
        reference_urls.push(Url::parse(&advisory_url).unwrap());
        // primary URL for the issue specified in the advisory
        if let Some(url) = metadata.url {
            reference_urls.push(url);
        }
        // other references
        reference_urls.extend(metadata.references);

        OsvAdvisory {
            schema_version: None,
            id: metadata.id,
            modified: mod_times
                .for_path(path)
                .format(&time::format_description::well_known::Rfc3339)
                .expect("well-known format to heap never fails"),
            published: rustsec_date_to_rfc3339(&metadata.date),
            affected: vec![OsvAffected {
                package: (&metadata.package).into(),
                ranges: Some(vec![OsvJsonRange::new(&advisory.versions)]),
                versions: Some(vec![]),
                ecosystem_specific: Some(OsvEcosystemSpecific {
                    affects: Some(advisory.affected.unwrap_or_default().into()),
                    affected_functions: None,
                }),
                database_specific: OsvDatabaseSpecific {
                    categories: metadata.categories,
                    cvss: metadata.cvss.clone(),
                    informational: metadata.informational,
                },
            }],
            withdrawn: metadata.withdrawn.map(|d| rustsec_date_to_rfc3339(&d)),
            aliases: metadata.aliases,
            related: metadata.related,
            summary: metadata.title,
            severity: match metadata.cvss {
                Some(cvss) => match cvss.try_into() {
                    Ok(sev) => vec![sev],
                    Err(_) => vec![],
                },
                None => vec![],
            },
            details: metadata.description,
            references: osv_references(reference_urls),
            database_specific: MainOsvDatabaseSpecific {
                license: Some(metadata.license.spdx().to_string()),
            },
        }
    }

    /// Try to extract RustSec alias id from OSV advisory metadata
    pub fn rustsec_refs_imported(&self) -> Vec<Id> {
        let mut refs: Vec<Id> = self
            .references
            .iter()
            .filter(|r| {
                r.url
                    .as_str()
                    .starts_with("https://rustsec.org/advisories/")
            })
            .map(|r| Id::from_str(&r.url.as_str()[31..48]).expect("Invalid rustsec url"))
            .collect();
        refs.sort();
        refs.dedup();
        refs
    }

    /// Get crates in crates.io ecosystem referenced in this advisory
    pub fn crates(&self) -> Vec<String> {
        let mut res: Vec<String> = self
            .affected
            .iter()
            .filter_map(|a| {
                if a.package.ecosystem == ECOSYSTEM {
                    Some(a.package.name.clone())
                } else {
                    None
                }
            })
            .collect();
        res.sort();
        res.dedup();
        res
    }

    /// Get aliases ids
    pub fn aliases(&self) -> &[Id] {
        self.aliases.as_slice()
    }

    /// Is this advisory withdrawn?
    pub fn withdrawn(&self) -> bool {
        self.withdrawn.is_some()
    }
}

fn osv_references(references: Vec<Url>) -> Vec<OsvReference> {
    references.into_iter().map(|u| u.into()).collect()
}

fn guess_url_kind(url: &Url) -> OsvReferenceKind {
    let str = url.as_str();
    if (str.contains("://github.com/") || str.contains("://gitlab.")) && str.contains("/issues/") {
        OsvReferenceKind::REPORT
    // the check for "/advisories/" matches both RustSec and GHSA URLs
    } else if str.contains("/advisories/") || str.contains("://cve.mitre.org/") {
        OsvReferenceKind::ADVISORY
    } else if str.contains("://crates.io/crates/") {
        OsvReferenceKind::PACKAGE
    } else {
        OsvReferenceKind::WEB
    }
}

fn rustsec_date_to_rfc3339(d: &crate::advisory::Date) -> String {
    format!("{}-{:02}-{:02}T12:00:00Z", d.year(), d.month(), d.day())
}
