//! SARIF (Static Analysis Results Interchange Format) output support
//!
//! This module provides functionality to convert cargo-audit reports to SARIF format,
//! which can be uploaded to GitHub Security tab and other security analysis platforms.
//!
//! SARIF is an OASIS Standard that defines a common format for static analysis tools
//! to report their findings. This implementation follows SARIF 2.1.0 specification
//! and is compatible with GitHub's code scanning requirements.

use std::collections::{HashMap, HashSet};

use rustsec::{Report, Vulnerability, Warning, WarningKind, advisory};
use serde::{Serialize, Serializer, ser::SerializeStruct};

/// SARIF log root object
#[derive(Debug)]
pub struct SarifLog {
    /// Array of analysis runs
    runs: Vec<Run>,
}

impl SarifLog {
    /// Convert a cargo-audit report to SARIF format
    pub fn from_report(report: &Report, cargo_lock_path: &str) -> Self {
        Self {
            runs: vec![Run::from_report(report, cargo_lock_path)],
        }
    }
}

impl Serialize for SarifLog {
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut state = Serializer::serialize_struct(serializer, "SarifLog", 3)?;
        state.serialize_field("$schema", "https://json.schemastore.org/sarif-2.1.0.json")?;
        state.serialize_field("version", "2.1.0")?;
        state.serialize_field("runs", &self.runs)?;
        state.end()
    }
}

/// A run represents a single invocation of an analysis tool
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Run {
    /// Tool information for this run
    tool: Tool,
    /// Array of results (findings) from the analysis
    results: Vec<SarifResult>,
}

impl Run {
    fn from_report(report: &Report, cargo_lock_path: &str) -> Self {
        let mut rules = Vec::new();
        let mut seen_rules = HashSet::new();
        let mut results = Vec::new();

        for vuln in &report.vulnerabilities.list {
            let rule_id = vuln.advisory.id.to_string();

            if seen_rules.insert(rule_id.clone()) {
                rules.push(ReportingDescriptor::from_advisory(&vuln.advisory, true));
            }

            results.push(SarifResult::from_vulnerability(vuln, cargo_lock_path));
        }

        for (warning_kind, warnings) in &report.warnings {
            for warning in warnings {
                let rule_id = if let Some(advisory) = &warning.advisory {
                    advisory.id.to_string()
                } else {
                    format!("{warning_kind:?}").to_lowercase()
                };

                if seen_rules.insert(rule_id) {
                    rules.push(match &warning.advisory {
                        Some(advisory) => ReportingDescriptor::from_advisory(advisory, false),
                        None => ReportingDescriptor::from_warning_kind(*warning_kind),
                    });
                }

                results.push(SarifResult::from_warning(warning, cargo_lock_path));
            }
        }

        Self {
            tool: Tool {
                driver: ToolComponent { rules },
            },
            results,
        }
    }
}

/// Tool information
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct Tool {
    /// The analysis tool that was run
    driver: ToolComponent,
}

/// Tool component (driver) information
#[derive(Debug)]
struct ToolComponent {
    /// Rules defined by this tool
    rules: Vec<ReportingDescriptor>,
}

impl Serialize for ToolComponent {
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut state = serializer.serialize_struct("ToolComponent", 4)?;
        state.serialize_field("name", "cargo-audit")?;
        state.serialize_field("version", env!("CARGO_PKG_VERSION"))?;
        state.serialize_field("semanticVersion", env!("CARGO_PKG_VERSION"))?;
        state.serialize_field("rules", &self.rules)?;
        state.end()
    }
}

/// Rule/reporting descriptor
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct ReportingDescriptor {
    /// Unique identifier for the rule
    id: String,
    /// Human-readable name of the rule
    name: String,
    /// Brief description of the rule
    short_description: MultiformatMessageString,
    /// Detailed description of the rule
    #[serde(skip_serializing_if = "Option::is_none")]
    full_description: Option<MultiformatMessageString>,
    /// Default severity and enablement for the rule
    default_configuration: ReportingConfiguration,
    /// Help text or URI for the rule
    #[serde(skip_serializing_if = "Option::is_none")]
    help: Option<MultiformatMessageString>,
    /// Additional properties including tags and severity scores
    properties: RuleProperties,
}

impl ReportingDescriptor {
    /// Create a ReportingDescriptor from an advisory
    fn from_advisory(metadata: &advisory::Metadata, is_vulnerability: bool) -> Self {
        let tags = if is_vulnerability {
            &[Tag::Security, Tag::Vulnerability]
        } else {
            &[Tag::Security, Tag::Warning]
        };

        let security_severity = metadata
            .cvss
            .as_ref()
            .map(|cvss| format!("{:.1}", cvss.score()));

        ReportingDescriptor {
            id: metadata.id.to_string(),
            name: metadata.id.to_string(),
            short_description: MultiformatMessageString {
                text: metadata.title.clone(),
                markdown: None,
            },
            full_description: if metadata.description.is_empty() {
                None
            } else {
                Some(MultiformatMessageString {
                    text: metadata.description.clone(),
                    markdown: None,
                })
            },
            default_configuration: ReportingConfiguration {
                level: match is_vulnerability {
                    true => ReportingLevel::Error,
                    false => ReportingLevel::Warning,
                },
            },
            help: metadata.url.as_ref().map(|url| MultiformatMessageString {
                text: format!("For more information, see: {url}"),
                markdown: Some(format!(
                    "For more information, see: [{}]({url})",
                    metadata.id
                )),
            }),
            properties: RuleProperties {
                tags,
                precision: Precision::VeryHigh,
                problem_severity: if !is_vulnerability {
                    Some(ProblemSeverity::Warning)
                } else {
                    None
                },
                security_severity,
            },
        }
    }

    /// Create a ReportingDescriptor from a warning kind
    fn from_warning_kind(kind: WarningKind) -> Self {
        let (name, description) = match kind {
            WarningKind::Unmaintained => (
                "unmaintained",
                "Package is unmaintained and may have unaddressed security vulnerabilities",
            ),
            WarningKind::Unsound => (
                "unsound",
                "Package has known soundness issues that may lead to memory safety problems",
            ),
            WarningKind::Yanked => (
                "yanked",
                "Package version has been yanked from the registry",
            ),
            _ => ("unknown", "Unknown warning type"),
        };

        ReportingDescriptor {
            id: name.to_string(),
            name: name.to_string(),
            short_description: MultiformatMessageString {
                text: description.to_string(),
                markdown: None,
            },
            full_description: None,
            default_configuration: ReportingConfiguration {
                level: ReportingLevel::Warning,
            },
            help: None,
            properties: RuleProperties {
                tags: &[Tag::Security, Tag::Warning],
                precision: Precision::High,
                problem_severity: Some(ProblemSeverity::Warning),
                security_severity: None,
            },
        }
    }
}

/// Rule properties
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct RuleProperties {
    /// Tags associated with the rule
    #[serde(skip_serializing_if = "<[Tag]>::is_empty")]
    tags: &'static [Tag],
    /// Precision of the rule (e.g., "very-high", "high")
    precision: Precision,
    /// Problem severity for non-security issues
    #[serde(skip_serializing_if = "Option::is_none")]
    #[serde(rename = "problem.severity")]
    problem_severity: Option<ProblemSeverity>,
    /// CVSS score as a string (0.0-10.0)
    #[serde(skip_serializing_if = "Option::is_none")]
    #[serde(rename = "security-severity")]
    security_severity: Option<String>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "lowercase")]
enum ProblemSeverity {
    Warning,
}

/// Reporting configuration for a rule
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct ReportingConfiguration {
    /// Default level for the rule ("error", "warning", "note")
    level: ReportingLevel,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
enum ReportingLevel {
    Error,
    Warning,
}

/// Message with optional markdown
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct MultiformatMessageString {
    /// Plain text message
    text: String,
    /// Optional markdown-formatted message
    #[serde(skip_serializing_if = "Option::is_none")]
    markdown: Option<String>,
}

/// A result (finding/alert)
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct SarifResult {
    /// ID of the rule that was violated
    rule_id: String,
    /// Message describing the result
    message: Message,
    /// Severity level of the result
    level: ResultLevel,
    /// Locations where the issue was detected
    locations: Vec<Location>,
    /// Fingerprints for result matching
    partial_fingerprints: HashMap<String, String>,
}

impl SarifResult {
    /// Create a Result from a vulnerability
    fn from_vulnerability(vuln: &Vulnerability, cargo_lock_path: &str) -> Self {
        let fingerprint = format!(
            "{}:{}:{}",
            vuln.advisory.id, vuln.package.name, vuln.package.version
        );

        SarifResult {
            rule_id: vuln.advisory.id.to_string(),
            message: Message {
                text: format!(
                    "{} {} is vulnerable to {} ({})",
                    vuln.package.name, vuln.package.version, vuln.advisory.id, vuln.advisory.title
                ),
            },
            level: ResultLevel::Error,
            locations: vec![Location::new(cargo_lock_path)],
            partial_fingerprints: {
                let mut fingerprints = HashMap::new();
                // Use a custom fingerprint key instead of primaryLocationLineHash
                // to avoid conflicts with GitHub's calculated fingerprints
                fingerprints.insert("cargo-audit/advisory-fingerprint".to_string(), fingerprint);
                fingerprints
            },
        }
    }

    /// Create a Result from a warning
    fn from_warning(warning: &Warning, cargo_lock_path: &str) -> Self {
        let rule_id = if let Some(advisory) = &warning.advisory {
            advisory.id.to_string()
        } else {
            format!("{:?}", warning.kind).to_lowercase()
        };

        let message_text = if let Some(advisory) = &warning.advisory {
            format!(
                "{} {} has a {} warning: {}",
                warning.package.name,
                warning.package.version,
                warning.kind.as_str(),
                advisory.title
            )
        } else {
            format!(
                "{} {} has a {} warning",
                warning.package.name,
                warning.package.version,
                warning.kind.as_str()
            )
        };

        let fingerprint = format!(
            "{rule_id}:{}:{}",
            warning.package.name, warning.package.version
        );

        SarifResult {
            rule_id,
            message: Message { text: message_text },
            level: ResultLevel::Warning,
            locations: vec![Location::new(cargo_lock_path)],
            partial_fingerprints: {
                let mut fingerprints = HashMap::new();
                // Use a custom fingerprint key instead of primaryLocationLineHash
                // to avoid conflicts with GitHub's calculated fingerprints
                fingerprints.insert("cargo-audit/advisory-fingerprint".to_string(), fingerprint);
                fingerprints
            },
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
enum ResultLevel {
    Error,
    Warning,
}

/// Simple message
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct Message {
    /// The message text
    text: String,
}

/// Location of a finding
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct Location {
    /// Physical location of the finding
    physical_location: PhysicalLocation,
}

impl Location {
    fn new(cargo_lock_path: &str) -> Self {
        Self {
            physical_location: PhysicalLocation {
                artifact_location: ArtifactLocation {
                    uri: cargo_lock_path.to_string(),
                },
                region: Region { start_line: 1 },
            },
        }
    }
}

/// Physical location in a file
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct PhysicalLocation {
    /// The artifact (file) containing the issue
    artifact_location: ArtifactLocation,
    /// Region within the artifact
    region: Region,
}

/// Artifact (file) location
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct ArtifactLocation {
    /// URI of the artifact
    uri: String,
}

/// Region within a file
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct Region {
    /// Starting line number (1-based)
    start_line: u32,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
enum Tag {
    Security,
    Vulnerability,
    Warning,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "kebab-case")]
enum Precision {
    High,
    VeryHigh,
}
