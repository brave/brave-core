//! RustSec Vulnerability Categories

use crate::error::Error;
use serde::{Deserialize, Serialize, de, ser};
use std::{fmt, str::FromStr};

/// RustSec Vulnerability Categories
///
/// The RustSec project maintains its own categorization system for
/// vulnerabilities according to our [criteria for acceptable advisories][1].
///
/// This type represents the present list of allowable vulnerability types for
/// which we allow advisories to be filed.
///
/// [1]: https://github.com/RustSec/advisory-db/blob/main/CONTRIBUTING.md#criteria
#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum Category {
    /// Execution of arbitrary code allowing an attacker to gain partial or
    /// total control of an impacted computer system.
    CodeExecution,

    /// Cryptography Failure (e.g. confidentiality breakage, integrity
    /// breakage, key leakage)
    CryptoFailure,

    /// Vulnerabilities an attacker can leverage to cause crashes or excess
    /// resource consumption such that software ceases to function normally,
    /// notably panics in code that is advertised as "panic-free" (particularly
    /// in format parsers for untrusted data)
    DenialOfService,

    /// Disclosure of local files (a.k.a. "directory traversal")
    FileDisclosure,

    /// Mishandled escaping allowing an attacker to execute code or perform
    /// otherwise unexpected operations, e.g. shell escaping, SQL injection, XSS.
    FormatInjection,

    /// Memory unsafety vulnerabilities allowing an attacker to write to
    /// unintended locations in memory.
    MemoryCorruption,

    /// Read-only memory safety vulnerabilities which unintentionally expose data.
    MemoryExposure,

    /// Attacks which bypass authentication and/or authorization systems,
    /// allowing the attacker to obtain unintended privileges.
    PrivilegeEscalation,

    /// Thread safety bug, e.g. data races arising from unsafe code that
    /// misapplies and/or misuses `Send`/`Sync`.
    ThreadSafety,

    /// Other types of categories: left open-ended to add more of them in the future.
    Other(String),
}

impl Category {
    /// Get the short "kebab case" identifier for a category
    pub fn name(&self) -> &str {
        match self {
            Category::CodeExecution => "code-execution",
            Category::CryptoFailure => "crypto-failure",
            Category::DenialOfService => "denial-of-service",
            Category::FileDisclosure => "file-disclosure",
            Category::FormatInjection => "format-injection",
            Category::MemoryCorruption => "memory-corruption",
            Category::MemoryExposure => "memory-exposure",
            Category::PrivilegeEscalation => "privilege-escalation",
            Category::ThreadSafety => "thread-safety",
            Category::Other(other) => other,
        }
    }
}

impl fmt::Display for Category {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.name())
    }
}

impl FromStr for Category {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        Ok(match s {
            "code-execution" => Category::CodeExecution,
            "crypto-failure" => Category::CryptoFailure,
            "denial-of-service" => Category::DenialOfService,
            "file-disclosure" => Category::FileDisclosure,
            "format-injection" => Category::FormatInjection,
            "memory-corruption" => Category::MemoryCorruption,
            "memory-exposure" => Category::MemoryExposure,
            "privilege-escalation" => Category::PrivilegeEscalation,
            "thread-safety" => Category::ThreadSafety,
            other => Category::Other(other.to_owned()),
        })
    }
}

impl<'de> Deserialize<'de> for Category {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        use de::Error;
        let string = String::deserialize(deserializer)?;
        string.parse().map_err(D::Error::custom)
    }
}

impl Serialize for Category {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}
