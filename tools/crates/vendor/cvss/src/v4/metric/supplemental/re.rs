//! Vulnerability Response Effort (RE)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Vulnerability Response Effort (RE) - CVSS v4.0 Supplemental Metric Group
///
/// Described in CVSS v4.0 Specification: Section 5.6
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum VulnerabilityResponseEffort {
    /// Not Defined (X)
    ///
    /// > The metric has not been evaluated.
    NotDefined,

    /// Low (L)
    ///
    /// > The effort required to respond to a vulnerability is low/trivial.
    /// > Examples include: communication on better documentation, configuration
    /// > workarounds, or guidance from the vendor that does **not** require an
    /// > immediate update, upgrade, or replacement by the consuming entity,
    /// > such as firewall filter configuration.
    Low,
    /// Moderate (M)
    ///
    /// > The actions required to respond to a vulnerability require some effort
    /// > on behalf of the consumer and could cause minimal service impact to
    /// > implement. Examples include: simple remote update, disabling of a
    /// > subsystem, or a low-touch software upgrade such as a driver update.
    Moderate,
    /// High (H)
    ///
    /// > The actions required to respond to a vulnerability are significant
    /// > and/or difficult, and may possibly lead to an extended, scheduled
    /// > service impact. This would need to be considered for scheduling
    /// > purposes including honoring any embargo on deployment of the selected
    /// > response. Alternatively, response to the vulnerability in the field is
    /// > not possible remotely. The only resolution to the vulnerability
    /// > involves physical replacement (e.g. units deployed would have to be
    /// > recalled for a depot level repair or replacement). Examples include: a
    /// > highly privileged driver update, microcode or UEFI BIOS updates, or
    /// > software upgrades requiring careful analysis and understanding of any
    /// > potential infrastructure impact before implementation. A UEFI BIOS
    /// > update that impacts Trusted Platform Module (TPM) attestation without
    /// > impacting disk encryption software such as Bit locker is a good recent
    /// > example. Irreparable failures such as non-bootable flash subsystems,
    /// > failed disks or solid-state drives (SSD), bad memory modules, network
    /// > devices, or other non-recoverable under warranty hardware, should also
    /// > be scored as having a High effort.
    High,
}

impl Default for VulnerabilityResponseEffort {
    fn default() -> Self {
        Self::NotDefined
    }
}

impl Metric for VulnerabilityResponseEffort {
    const TYPE: MetricType = MetricType::RE;

    fn as_str(self) -> &'static str {
        match self {
            VulnerabilityResponseEffort::NotDefined => "X",
            VulnerabilityResponseEffort::Low => "L",
            VulnerabilityResponseEffort::Moderate => "M",
            VulnerabilityResponseEffort::High => "H",
        }
    }
}

impl fmt::Display for VulnerabilityResponseEffort {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for VulnerabilityResponseEffort {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "X" => Ok(VulnerabilityResponseEffort::NotDefined),
            "L" => Ok(VulnerabilityResponseEffort::Low),
            "M" => Ok(VulnerabilityResponseEffort::Moderate),
            "H" => Ok(VulnerabilityResponseEffort::High),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}
