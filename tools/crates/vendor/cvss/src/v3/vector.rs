#[cfg(feature = "serde")]
use alloc::string::{String, ToString};
use alloc::{borrow::ToOwned, vec::Vec};
use core::fmt;
use core::str::FromStr;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize, de, ser};

#[cfg(feature = "std")]
use crate::Severity;

use crate::{
    Error, PREFIX,
    v3::{
        Metric, Score,
        metric::{
            MetricType,
            base::{
                AttackComplexity, AttackVector, Availability, Confidentiality, Integrity,
                PrivilegesRequired, Scope, UserInteraction,
            },
            environmental::{
                AvailabilityRequirement, ConfidentialityRequirement, IntegrityRequirement,
                Modified, ModifiedScope,
            },
            temporal::{ExploitCodeMaturity, RemediationLevel, ReportConfidence},
        },
    },
};

/// A CVSS 3.x vector, including Base, Temporal, and Environmental metrics.
///
/// Described in CVSS v3.1 Specification: Section 2:
/// <https://www.first.org/cvss/specification-document#t6>
///
/// > The Base metric group represents the intrinsic characteristics of a
/// > vulnerability that are constant over time and across user environments.
/// > It is composed of two sets of metrics: the Exploitability metrics and
/// > the Impact metrics.
/// >
/// > The Exploitability metrics reflect the ease and technical means by which
/// > the vulnerability can be exploited. That is, they represent characteristics
/// > of *the thing that is vulnerable*, which we refer to formally as the
/// > *vulnerable component*. The Impact metrics reflect the direct consequence
/// > of a successful exploit, and represent the consequence to the
/// > *thing that suffers the impact*, which we refer to formally as the
/// > *impacted component*.
/// >
/// > While the vulnerable component is typically a software application,
/// > module, driver, etc. (or possibly a hardware device), the impacted
/// > component could be a software application, a hardware device or a network
/// > resource. This potential for measuring the impact of a vulnerability other
/// > than the vulnerable component, was a key feature introduced with
/// > CVSS v3.0. This property is captured by the Scope metric.
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct Vector {
    /// Minor component of the version
    pub minor_version: usize,

    /// Attack Vector (AV)
    pub av: Option<AttackVector>,

    /// Attack Complexity (AC)
    pub ac: Option<AttackComplexity>,

    /// Privileges Required (PR)
    pub pr: Option<PrivilegesRequired>,

    /// User Interaction (UI)
    pub ui: Option<UserInteraction>,

    /// Scope (S)
    pub s: Option<Scope>,

    /// Confidentiality Impact (C)
    pub c: Option<Confidentiality>,

    /// Integrity Impact (I)
    pub i: Option<Integrity>,

    /// Availability Impact (A)
    pub a: Option<Availability>,

    /// Exploit Code Maturity (E)
    pub e: Option<ExploitCodeMaturity>,

    /// Remediation Level (RL)
    pub rl: Option<RemediationLevel>,

    /// Report Confidence (RC)
    pub rc: Option<ReportConfidence>,

    /// Modified Attack Vector (MAV)
    pub mav: Option<Modified<AttackVector>>,

    /// Confidentiality Requirements (CR)
    pub cr: Option<ConfidentialityRequirement>,

    /// Integrity Requirements (IR)
    pub ir: Option<IntegrityRequirement>,

    /// Availability Requirements (AR)
    pub ar: Option<AvailabilityRequirement>,

    /// Modified Attack Complexity (MAC)
    pub mac: Option<Modified<AttackComplexity>>,

    /// Modified Privileges Required (MPR)
    pub mpr: Option<Modified<PrivilegesRequired>>,

    /// Modified User Interaction (MUI)
    pub mui: Option<Modified<UserInteraction>>,

    /// Modified Scope (MS)
    pub ms: Option<ModifiedScope>,

    /// Modified Confidentiality (MC)
    pub mc: Option<Modified<Confidentiality>>,

    /// Modified Integrity (MI)
    pub mi: Option<Modified<Integrity>>,

    /// Modified Availability (MA)
    pub ma: Option<Modified<Availability>>,
}

// Helper macro to build the array of (MetricType, Option<&dyn fmt::Debug>)
macro_rules! metrics_array {
    ($s:expr, $( ($metric_ty:expr, $field:ident) ),+ $(,)?) => {
        [
            $(
                ($metric_ty, $s.$field.as_ref().map(|m| m as &dyn fmt::Debug)),
            )+
        ]
    };
}

impl Vector {
    /// Alias for `base_score()`.
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn score(&self) -> Score {
        self.base_score()
    }

    /// Calculate Base CVSS score: overall value for determining the severity
    /// of a vulnerability, generally referred to as the "CVSS score".
    ///
    /// Described in CVSS v3.1 Specification: Section 2:
    /// <https://www.first.org/cvss/specification-document#t6>
    ///
    /// > When the Base metrics are assigned values by an analyst, the Base
    /// > equation computes a score ranging from 0.0 to 10.0.
    /// >
    /// > Specifically, the Base equation is derived from two sub equations:
    /// > the Exploitability sub-score equation, and the Impact sub-score
    /// > equation. The Exploitability sub-score equation is derived from the
    /// > Base Exploitability metrics, while the Impact sub-score equation is
    /// > derived from the Base Impact metrics.
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn base_score(&self) -> Score {
        let exploitability = self.exploitability().value();
        let iss = self.impact().value();

        let iss_scoped = if !self.is_scope_changed() {
            6.42 * iss
        } else {
            (7.52 * (iss - 0.029)) - (3.25 * (iss - 0.02).powf(15.0))
        };

        let score = if iss_scoped <= 0.0 {
            0.0
        } else if !self.is_scope_changed() {
            (iss_scoped + exploitability).min(10.0)
        } else {
            (1.08 * (iss_scoped + exploitability)).min(10.0)
        };

        Score::new(score).roundup_for_version(self.minor_version)
    }

    /// Calculate Base Exploitability score: sub-score for measuring
    /// ease of exploitation.
    ///
    /// Described in CVSS v3.1 Specification: Section 2:
    /// <https://www.first.org/cvss/specification-document#t6>
    ///
    /// > The Exploitability metrics reflect the ease and technical means by which
    /// > the vulnerability can be exploited. That is, they represent characteristics
    /// > of *the thing that is vulnerable*, which we refer to formally as the
    /// > *vulnerable component*.
    pub fn exploitability(&self) -> Score {
        let av_score = self.av.map(|av| av.score()).unwrap_or(0.0);
        let ac_score = self.ac.map(|ac| ac.score()).unwrap_or(0.0);
        let ui_score = self.ui.map(|ui| ui.score()).unwrap_or(0.0);
        let pr_score = self
            .pr
            .map(|pr| pr.scoped_score(self.is_scope_changed()))
            .unwrap_or(0.0);

        (8.22 * av_score * ac_score * pr_score * ui_score).into()
    }

    /// Calculate Base Impact Score (ISS): sub-score for measuring the
    /// consequences of successful exploitation.
    ///
    /// Described in CVSS v3.1 Specification: Section 2:
    /// <https://www.first.org/cvss/specification-document#t6>
    ///
    /// > The Impact metrics reflect the direct consequence
    /// > of a successful exploit, and represent the consequence to the
    /// > *thing that suffers the impact*, which we refer to formally as the
    /// > *impacted component*.
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn impact(&self) -> Score {
        let c_score = self.c.map(|c| c.score()).unwrap_or(0.0);
        let i_score = self.i.map(|i| i.score()).unwrap_or(0.0);
        let a_score = self.a.map(|a| a.score()).unwrap_or(0.0);
        (1.0 - ((1.0 - c_score) * (1.0 - i_score) * (1.0 - a_score)).abs()).into()
    }

    /// Iterate over all defined metrics in this vector
    pub fn metrics(&self) -> impl Iterator<Item = (MetricType, &dyn fmt::Debug)> {
        metrics_array!(
            self,
            (MetricType::AV, av),
            (MetricType::AC, ac),
            (MetricType::PR, pr),
            (MetricType::UI, ui),
            (MetricType::S, s),
            (MetricType::C, c),
            (MetricType::I, i),
            (MetricType::A, a),
            // Temporal metrics
            (MetricType::E, e),
            (MetricType::RL, rl),
            (MetricType::RC, rc),
            // Environmental metrics
            (MetricType::MAV, mav),
            (MetricType::MAC, mac),
            (MetricType::MPR, mpr),
            (MetricType::MUI, mui),
            (MetricType::MS, ms),
            (MetricType::MC, mc),
            (MetricType::MI, mi),
            (MetricType::CR, cr),
            (MetricType::IR, ir),
            (MetricType::AR, ar),
            (MetricType::MA, ma),
        )
        .into_iter()
        .filter_map(|(name, metric)| metric.as_ref().map(|&m| (name, m)))
    }

    /// Calculate Base CVSS `Severity` according to the
    /// Qualitative Severity Rating Scale (i.e. Low / Medium / High / Critical)
    ///
    /// Described in CVSS v3.1 Specification: Section 5:
    /// <https://www.first.org/cvss/specification-document#t17>
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn severity(&self) -> Severity {
        self.base_score().severity()
    }

    /// Has the scope changed?
    pub(crate) fn is_scope_changed(&self) -> bool {
        self.s.map(|s| s.is_changed()).unwrap_or(false)
    }

    /// Calculate the CVSS 3.x Environmental Score for this vector.
    ///
    /// Described in CVSS v3.0 Specification: Section 8.2:
    /// <https://www.first.org/cvss/v3-0/specification-document#8-3-Environmental>
    ///
    /// Described in CVSS v3.1 Specification: Section 7.3:
    /// <https://www.first.org/cvss/v3-1/specification-document#7-3-Environmental-Metrics-Equations>
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn environmental_score(&self) -> Score {
        let modified_impact = self.modified_impact();
        let modified_exploitability = self.modified_exploitability();
        let e_score = self.e.unwrap_or(ExploitCodeMaturity::NotDefined).score();
        let rl_score = self.rl.unwrap_or(RemediationLevel::NotDefined).score();
        let rc_score = self.rc.unwrap_or(ReportConfidence::NotDefined).score();

        let environmental_score = if modified_impact <= 0.0 {
            0.0
        } else if !self.is_modified_scope_changed() {
            Score::new((modified_impact + modified_exploitability).min(10.0))
                .roundup_for_version(self.minor_version)
                .value()
                * e_score
                * rl_score
                * rc_score
        } else {
            Score::new((1.08 * (modified_impact + modified_exploitability)).min(10.0))
                .roundup_for_version(self.minor_version)
                .value()
                * e_score
                * rl_score
                * rc_score
        };
        Score::new(environmental_score).roundup_for_version(self.minor_version)
    }

    /// Calculate the modified impact sub-score (MISS) for environmental score
    /// calculations.
    #[cfg(feature = "std")]
    pub(crate) fn modified_impact_sub_score(&self) -> f64 {
        let cr_score = self
            .cr
            .unwrap_or(ConfidentialityRequirement::NotDefined)
            .score();
        let ir_score = self.ir.unwrap_or(IntegrityRequirement::NotDefined).score();
        let ar_score = self
            .ar
            .unwrap_or(AvailabilityRequirement::NotDefined)
            .score();
        let c_score = self
            .mc
            .unwrap_or(Modified::NotDefined)
            .modified_score(self.c);
        let i_score = self
            .mi
            .unwrap_or(Modified::NotDefined)
            .modified_score(self.i);
        let a_score = self
            .ma
            .unwrap_or(Modified::NotDefined)
            .modified_score(self.a);

        let miss = 1.0
            - (1.0 - cr_score * c_score) * (1.0 - ir_score * i_score) * (1.0 - ar_score * a_score);
        miss.min(0.915)
    }

    /// Calculate the CVSS 3.x Modified Impact sub-score (MISS) for
    /// environmental score calculations.
    #[cfg(feature = "std")]
    pub(crate) fn modified_impact(&self) -> f64 {
        let miss = self.modified_impact_sub_score();
        if self.is_modified_scope_changed() {
            if self.minor_version == 0 {
                7.52 * (miss - 0.029) - 3.25 * (miss - 0.02).powf(15.0)
            } else {
                7.52 * (miss - 0.029) - 3.25 * (miss * 0.9731 - 0.02).powf(13.0)
            }
        } else {
            6.42 * miss
        }
    }

    #[cfg(feature = "std")]
    pub(crate) fn modified_exploitability(&self) -> f64 {
        let av_score = self
            .mav
            .unwrap_or(Modified::NotDefined)
            .modified_score(self.av);
        let ac_score = self
            .mac
            .unwrap_or(Modified::NotDefined)
            .modified_score(self.ac);
        let pr_score = self
            .mpr
            .unwrap_or(Modified::NotDefined)
            .scoped_score(self.is_modified_scope_changed(), self.pr);
        let ui_score = self
            .mui
            .unwrap_or(Modified::NotDefined)
            .modified_score(self.ui);

        8.22 * av_score * ac_score * pr_score * ui_score
    }

    #[cfg(feature = "std")]
    pub(crate) fn is_modified_scope_changed(&self) -> bool {
        let Some(modified_scope) = self.ms else {
            return self.is_scope_changed();
        };

        match modified_scope {
            ModifiedScope::Modified(scope) => scope == Scope::Changed,
            ModifiedScope::NotDefined => self.is_scope_changed(),
        }
    }
}

macro_rules! write_metrics {
    ($f:expr, $($metric:expr),+) => {
        $(
            if let Some(metric) = $metric {
                write!($f, "/{}", metric)?;
            }
        )+
    };
}

impl fmt::Display for Vector {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:3.{}", PREFIX, self.minor_version)?;
        write_metrics!(
            f, self.av, self.ac, self.pr, self.ui, self.s, self.c, self.i, self.a,
            // Temporal
            self.e, self.rl, self.rc, // Requirements (standard order)
            self.cr, self.ir, self.ar, // Modified base metrics
            self.mav, self.mac, self.mpr, self.mui, self.ms, self.mc, self.mi, self.ma
        );
        Ok(())
    }
}

impl FromStr for Vector {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self, Error> {
        let component_vec = s
            .split('/')
            .map(|component| {
                let mut parts = component.split(':');

                let id = parts.next().ok_or_else(|| Error::InvalidComponent {
                    component: component.to_owned(),
                })?;

                let value = parts.next().ok_or_else(|| Error::InvalidComponent {
                    component: component.to_owned(),
                })?;

                if parts.next().is_some() {
                    return Err(Error::InvalidComponent {
                        component: component.to_owned(),
                    });
                }

                Ok((id, value))
            })
            .collect::<Result<Vec<_>, Error>>()?;

        let mut components = component_vec.iter();
        let &(id, version_string) = components.next().ok_or_else(|| Error::InvalidPrefix {
            prefix: s.to_owned(),
        })?;

        if id != PREFIX {
            return Err(Error::InvalidPrefix {
                prefix: id.to_owned(),
            });
        }

        let mut metrics = Self {
            minor_version: match version_string {
                "3.0" => 0,
                "3.1" => 1,
                _ => {
                    return Err(Error::UnsupportedVersion {
                        version: version_string.to_owned(),
                    });
                }
            },
            ..Default::default()
        };

        fn get_value<T: FromStr<Err = Error>>(
            metric_type: MetricType,
            current_val: Option<T>,
            new_val: &str,
        ) -> Result<Option<T>, Error> {
            let parsed = T::from_str(new_val)?;
            if current_val.is_some() {
                return Err(Error::DuplicateMetricV3 { metric_type });
            }
            Ok(Some(parsed))
        }

        for &component in components {
            let id = component.0.to_ascii_uppercase();
            let value = component.1.to_ascii_uppercase();

            match id.parse::<MetricType>()? {
                // Base metrics
                MetricType::AV => metrics.av = get_value(MetricType::AV, metrics.av, &value)?,
                MetricType::AC => metrics.ac = get_value(MetricType::AC, metrics.ac, &value)?,
                MetricType::PR => metrics.pr = get_value(MetricType::PR, metrics.pr, &value)?,
                MetricType::UI => metrics.ui = get_value(MetricType::UI, metrics.ui, &value)?,
                MetricType::S => metrics.s = get_value(MetricType::S, metrics.s, &value)?,
                MetricType::C => metrics.c = get_value(MetricType::C, metrics.c, &value)?,
                MetricType::I => metrics.i = get_value(MetricType::I, metrics.i, &value)?,
                MetricType::A => metrics.a = get_value(MetricType::A, metrics.a, &value)?,

                // Temporal metrics
                MetricType::E => metrics.e = get_value(MetricType::E, metrics.e, &value)?,
                MetricType::RL => metrics.rl = get_value(MetricType::RL, metrics.rl, &value)?,
                MetricType::RC => metrics.rc = get_value(MetricType::RC, metrics.rc, &value)?,

                // Environmental metrics (use constructors that accept the base metric)
                MetricType::MAV => metrics.mav = get_value(MetricType::MAV, metrics.mav, &value)?,
                MetricType::MAC => metrics.mac = get_value(MetricType::MAC, metrics.mac, &value)?,
                MetricType::MPR => metrics.mpr = get_value(MetricType::MPR, metrics.mpr, &value)?,
                MetricType::MUI => metrics.mui = get_value(MetricType::MUI, metrics.mui, &value)?,
                MetricType::MS => metrics.ms = get_value(MetricType::MS, metrics.ms, &value)?,
                MetricType::MC => metrics.mc = get_value(MetricType::MC, metrics.mc, &value)?,
                MetricType::MI => metrics.mi = get_value(MetricType::MI, metrics.mi, &value)?,
                MetricType::MA => metrics.ma = get_value(MetricType::MA, metrics.ma, &value)?,
                MetricType::CR => metrics.cr = get_value(MetricType::CR, metrics.cr, &value)?,
                MetricType::IR => metrics.ir = get_value(MetricType::IR, metrics.ir, &value)?,
                MetricType::AR => metrics.ar = get_value(MetricType::AR, metrics.ar, &value)?,
            }
        }

        Ok(metrics)
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl<'de> Deserialize<'de> for Vector {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl Serialize for Vector {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}

#[cfg(all(test, feature = "std"))]
mod tests {
    use core::str::FromStr;
    use std::string::ToString;

    use crate::v3::Vector;

    #[test]
    fn parse_full_cvss3() {
        // See https://www.first.org/cvss/calculator/3-1#CVSS:3.1/AV:A/AC:H/PR:L/UI:N/S:U/C:L/I:L/A:N/E:P/RL:T/RC:R/AR:H/MAC:H/MUI:R/MS:C/MC:L/MA:N
        let vector_s = "CVSS:3.1/AV:A/AC:H/PR:L/UI:N/S:U/C:L/I:L/A:N/E:P/RL:T/RC:R/CR:X/IR:X/AR:H/MAV:X/MAC:H/MPR:X/MUI:R/MS:C/MC:L/MI:X/MA:N";
        let v: Vector = Vector::from_str(vector_s).unwrap();
        assert_eq!(vector_s, v.to_string());

        let base_score = v.base_score().value();
        assert_eq!(base_score, 3.7);

        let temporal_score = v.temporal_score().value();
        assert_eq!(temporal_score, 3.3);

        let environmental_score = v.environmental_score().value();
        assert_eq!(environmental_score, 3.5);
    }

    #[test]
    fn cvss30_vs_cvss31() {
        // See https://www.first.org/cvss/calculator/3-0#CVSS:3.0/AV:N/AC:L/PR:N/UI:R/S:C/C:H/I:H/A:H
        let v30 = "CVSS:3.0/AV:N/AC:L/PR:N/UI:R/S:C/C:H/I:H/A:H";
        // See https://www.first.org/cvss/calculator/3-1#CVSS:3.1/AV:N/AC:L/PR:N/UI:R/S:C/C:H/I:H/A:H
        let v31 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:R/S:C/C:H/I:H/A:H";

        let vec30: Vector = Vector::from_str(v30).unwrap();
        let vec31: Vector = Vector::from_str(v31).unwrap();

        let base_score_30 = vec30.base_score().value();
        let base_score_31 = vec31.base_score().value();
        assert_eq!(base_score_30, base_score_31);

        let temporal_score_30 = vec30.temporal_score().value();
        let temporal_score_31 = vec31.temporal_score().value();
        assert_eq!(temporal_score_30, temporal_score_31);

        // Environmental scores are different between CVSS v3.0 and v3.1 for
        // this vector because of the different exponent used in the calculation
        // of the Modified Impact sub-score when Scope is changed.
        let environmental_score_30 = vec30.environmental_score().value();
        let environmental_score_31 = vec31.environmental_score().value();

        assert_ne!(environmental_score_30, environmental_score_31);
        assert_eq!(environmental_score_30, 9.6);
        assert_eq!(environmental_score_31, 9.7);
    }
}
