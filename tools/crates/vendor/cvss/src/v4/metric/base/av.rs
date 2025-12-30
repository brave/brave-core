//! Attack Vector (AV)

use crate::{
    Error, Result,
    v4::metric::{Metric, MetricType},
};
use alloc::borrow::ToOwned;
use core::{fmt, str::FromStr};

/// Attack Vector (AV) - CVSS v4.0 Base Metric Group
///
/// Described in CVSS v4.0 Specification: Section 2.1.1
///
/// > This metric reflects the context by which vulnerability exploitation is
/// > possible. This metric value (and consequently the resulting severity) will
/// > be larger the more remote (logically, and physically) an attacker can be
/// > in order to exploit the vulnerable system. The assumption is that the
/// > number of potential attackers for a vulnerability that could be exploited
/// > from across a network is larger than the number of potential attackers
/// > that could exploit a vulnerability requiring physical access to a device,
/// > and therefore warrants a greater severity.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub enum AttackVector {
    /// Physical (P)
    ///
    /// > The attack requires the attacker to physically touch or manipulate the
    /// > vulnerable system. Physical interaction may be brief (e.g., evil maid
    /// > attack1) or persistent. An example of such an attack is a cold boot
    /// > attack in which an attacker gains access to disk encryption keys after
    /// > physically accessing the target system. Other examples include
    /// > peripheral attacks via FireWire/USB Direct Memory Access (DMA).
    Physical,

    /// Local (L)
    ///
    /// > The vulnerable system is not bound to the network stack and the
    /// > attacker’s path is via read/write/execute capabilities. Either: the
    /// > attacker exploits the vulnerability by accessing the target system
    /// > locally (e.g., keyboard, console), or through terminal emulation
    /// > (e.g., SSH); or the attacker relies on User Interaction by another
    /// > person to perform actions required to exploit the vulnerability (e.g.,
    /// > using social engineering techniques to trick a legitimate user into
    /// > opening a malicious document).
    Local,

    /// Adjacent (A)
    ///
    /// > The vulnerable system is bound to a protocol stack, but the attack is
    /// > limited at the protocol level to a logically adjacent topology. This
    /// > can mean an attack must be launched from the same shared proximity
    /// > (e.g., Bluetooth, NFC, or IEEE 802.11) or logical network (e.g., local
    /// > IP subnet), or from within a secure or otherwise limited
    /// > administrative domain (e.g., MPLS, secure VPN within an administrative
    /// > network zone). One example of an Adjacent attack would be an ARP
    /// > (IPv4) or neighbor discovery (IPv6) flood leading to a denial of
    /// > service on the local LAN segment (e.g., CVE-2013-6014).
    Adjacent,

    /// Network (N)
    ///
    /// > The vulnerable system is bound to the network stack and the set of
    /// > possible attackers extends beyond the other options listed below, up
    /// > to and including the entire Internet. Such a vulnerability is often
    /// > termed “remotely exploitable” and can be thought of as an attack being
    /// > exploitable at the protocol level one or more network hops away (e.g.,
    /// > across one or more routers). An example of a network attack is an
    /// > attacker causing a denial of service (DoS) by sending a specially
    /// > crafted TCP packet across a wide area network (e.g., CVE-2004-0230).
    Network,
}

impl Default for AttackVector {
    fn default() -> Self {
        Self::Network
    }
}

impl Metric for AttackVector {
    const TYPE: MetricType = MetricType::AV;

    fn as_str(self) -> &'static str {
        match self {
            AttackVector::Network => "N",
            AttackVector::Adjacent => "A",
            AttackVector::Local => "L",
            AttackVector::Physical => "P",
        }
    }
}

impl fmt::Display for AttackVector {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", Self::name(), self.as_str())
    }
}

impl FromStr for AttackVector {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "N" => Ok(AttackVector::Network),
            "A" => Ok(AttackVector::Adjacent),
            "L" => Ok(AttackVector::Local),
            "P" => Ok(AttackVector::Physical),
            _ => Err(Error::InvalidMetricV4 {
                metric_type: Self::TYPE,
                value: s.to_owned(),
            }),
        }
    }
}

#[cfg(feature = "std")]
pub(crate) mod merge {
    use super::*;
    use crate::{
        Error,
        v4::{
            MetricType,
            metric::{MetricLevel, environmental::ModifiedAttackVector},
        },
    };
    use alloc::borrow::ToOwned;
    use core::str::FromStr;

    /// Result of the merging of the base and modified metrics.
    ///
    /// Used in scoring.
    #[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
    pub(crate) enum MergedAttackVector {
        Physical,
        Local,
        Adjacent,
        Network,
    }

    impl Default for MergedAttackVector {
        fn default() -> Self {
            Self::Network
        }
    }

    impl FromStr for MergedAttackVector {
        type Err = Error;

        fn from_str(s: &str) -> Result<Self> {
            match s {
                "P" => Ok(MergedAttackVector::Physical),
                "L" => Ok(MergedAttackVector::Local),
                "A" => Ok(MergedAttackVector::Adjacent),
                "N" => Ok(MergedAttackVector::Network),
                _ => Err(Error::InvalidMetricV4 {
                    metric_type: MetricType::AV,
                    value: s.to_owned(),
                }),
            }
        }
    }

    impl MetricLevel for MergedAttackVector {
        fn level(self) -> f64 {
            // AV_levels = {"N": 0.0, "A": 0.1, "L": 0.2, "P": 0.3}
            match self {
                Self::Physical => 0.3,
                Self::Local => 0.2,
                Self::Adjacent => 0.1,
                Self::Network => 0.0,
            }
        }
    }

    impl AttackVector {
        pub(crate) fn merge(self, value: Option<ModifiedAttackVector>) -> MergedAttackVector {
            match value {
                Some(ModifiedAttackVector::NotDefined) | None => match self {
                    Self::Network => MergedAttackVector::Network,
                    Self::Adjacent => MergedAttackVector::Adjacent,
                    Self::Local => MergedAttackVector::Local,
                    Self::Physical => MergedAttackVector::Physical,
                },
                Some(ModifiedAttackVector::Network) => MergedAttackVector::Network,
                Some(ModifiedAttackVector::Adjacent) => MergedAttackVector::Adjacent,
                Some(ModifiedAttackVector::Local) => MergedAttackVector::Local,
                Some(ModifiedAttackVector::Physical) => MergedAttackVector::Physical,
            }
        }
    }
}
