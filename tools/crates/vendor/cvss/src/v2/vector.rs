//! CVSS v2.0 vector and score calculations

#[cfg(feature = "serde")]
use alloc::string::{String, ToString};
use alloc::{borrow::ToOwned, vec::Vec};
use core::fmt;
use core::str::FromStr;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize, de, ser};

use crate::{
    Error,
    v2::{
        Metric, MetricType, Score,
        metric::{
            base::{
                AccessComplexity, AccessVector, Authentication, AvailabilityImpact,
                ConfidentialityImpact, IntegrityImpact,
            },
            environmental::{
                AvailabilityRequirement, CollateralDamagePotential, ConfidentialityRequirement,
                IntegrityRequirement, TargetDistribution,
            },
            temporal::{Exploitability, RemediationLevel, ReportConfidence},
        },
    },
};

/// CVSS v2.0 Vector
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct Vector {
    /// Access Vector (AV)
    pub av: Option<AccessVector>,

    /// Access Complexity (AC)
    pub ac: Option<AccessComplexity>,

    /// Authentication (Au)
    pub au: Option<Authentication>,

    /// Confidentiality Impact (C)
    pub c: Option<ConfidentialityImpact>,

    /// Integrity Impact (I)
    pub i: Option<IntegrityImpact>,

    /// Availability Impact (A)
    pub a: Option<AvailabilityImpact>,

    /// Exploitability (E)
    pub e: Option<Exploitability>,

    /// Remediation Level (RL)
    pub rl: Option<RemediationLevel>,

    /// Report Confidence (RC)
    pub rc: Option<ReportConfidence>,

    /// Collateral Damage Potential (CDP)
    pub cdp: Option<CollateralDamagePotential>,

    /// Target Distribution (TD)
    pub td: Option<TargetDistribution>,

    /// Confidentiality Requirement (CR)
    pub cr: Option<ConfidentialityRequirement>,

    /// Integrity Requirement (IR)
    pub ir: Option<IntegrityRequirement>,

    /// Availability Requirement (AR)
    pub ar: Option<AvailabilityRequirement>,
}

impl Vector {
    /// Calculate Base CVSS score: overall value for determining the severity
    /// of a vulnerability, generally referred to as the "CVSS score".
    ///
    /// Described in CVSS v2.0 Specification: Section 3.2.1:
    /// <https://www.first.org/cvss/v2/guide#3-2-1-Base-Equation>
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn score(&self) -> Score {
        Score::new(f64::max(
            self.base_score_internal(self.impact()).value(),
            0.0,
        ))
    }

    /// Internal calculation of Base CVSS that takes impact as parameter.
    ///
    /// This is primarily needed in environmental score calculation where the
    /// impact is adjusted. Rounded to 1 decimal, per the CVSS v2.0 spec's
    /// definition of `BaseScore` (Section 3.2.1).
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub(crate) fn base_score_internal(&self, impact: Score) -> Score {
        let exploitability = self.exploitability();
        let f_impact = if impact.value() == 0.0 { 0.0 } else { 1.176 };

        let score = ((0.6 * impact.value()) + (0.4 * exploitability.value()) - 1.5) * f_impact;
        Score::new(score).roundup()
    }

    /// Calculate Base Exploitability score: sub-score for measuring
    /// ease of exploitation.
    ///
    /// Described in CVSS v2.0 Specification: Section 3.2.1:
    /// <https://www.first.org/cvss/v2/guide#3-2-1-Base-Equation>
    ///
    /// Metrics absent from a hand-constructed vector count as 0.0; vectors
    /// obtained through `FromStr` always carry all six base metrics.
    pub fn exploitability(&self) -> Score {
        let av_score = self.av.map(|av| av.score()).unwrap_or(0.0);
        let ac_score = self.ac.map(|ac| ac.score()).unwrap_or(0.0);
        let au_score = self.au.map(|au| au.score()).unwrap_or(0.0);

        (20.0 * av_score * ac_score * au_score).into()
    }

    /// Calculate Base Impact Score: sub-score for measuring the
    /// consequences of successful exploitation.
    ///
    /// Described in CVSS v2.0 Specification: Section 3.2.1:
    /// <https://www.first.org/cvss/v2/guide#3-2-1-Base-Equation>
    ///
    /// Metrics absent from a hand-constructed vector count as 0.0; vectors
    /// obtained through `FromStr` always carry all six base metrics.
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn impact(&self) -> Score {
        let c_score = self.c.map(|c| c.score()).unwrap_or(0.0);
        let i_score = self.i.map(|i| i.score()).unwrap_or(0.0);
        let a_score = self.a.map(|a| a.score()).unwrap_or(0.0);
        (10.41 * (1.0 - ((1.0 - c_score) * (1.0 - i_score) * (1.0 - a_score)))).into()
    }

    /// Calculate Environmental CVSS score
    ///
    /// Described in CVSS v2.0 Specification: Section 3.2.3:
    /// <https://www.first.org/cvss/v2/guide#3-2-3-Environmental-Equation>
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn environmental_score(&self) -> Score {
        let adjusted_temporal = self.temporal_score_internal(self.adjusted_impact());
        let cdp_score = self.cdp.map(|cdp| cdp.score()).unwrap_or(0.0);
        let td_score = self.td.map(|td| td.score()).unwrap_or(1.0);
        let score =
            (adjusted_temporal.value() + (10.0 - adjusted_temporal.value()) * cdp_score) * td_score;

        Score::new(f64::max(Score::new(score).roundup().value(), 0.0))
    }

    /// Calculate Adjusted Impact: modified impact score based on
    /// environmental requirements.
    pub fn adjusted_impact(&self) -> Score {
        let c_score = self.c.map(|c| c.score()).unwrap_or(0.0);
        let i_score = self.i.map(|i| i.score()).unwrap_or(0.0);
        let a_score = self.a.map(|a| a.score()).unwrap_or(0.0);

        let cr_score = self.cr.map(|cr| cr.score()).unwrap_or(1.0);
        let ir_score = self.ir.map(|ir| ir.score()).unwrap_or(1.0);
        let ar_score = self.ar.map(|ar| ar.score()).unwrap_or(1.0);

        (10.0_f64.min(
            10.41
                * (1.0
                    - (1.0 - c_score * cr_score)
                        * (1.0 - i_score * ir_score)
                        * (1.0 - a_score * ar_score)),
        ))
        .into()
    }

    /// Calculate Temporal CVSS score
    ///
    /// Described in CVSS v2.0 Specification: Section 3.2.2:
    /// <https://www.first.org/cvss/v2/guide#3-2-2-Temporal-Equation>
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn temporal_score(&self) -> Score {
        Score::new(f64::max(
            self.temporal_score_internal(self.impact()).value(),
            0.0,
        ))
    }

    /// Internal calculation of Temporal CVSS that takes impact as parameter.
    ///
    /// This is primarily needed in environmental score calculation where the
    /// impact is adjusted. Both the (Adjusted) `BaseScore` and the returned
    /// Temporal/Adjusted Temporal value are rounded to 1 decimal, per the
    /// CVSS v2.0 spec's definition of `TemporalScore` (Section 3.2.2):
    /// `TemporalScore = round_to_1_decimal(BaseScore*E*RL*RC)`. Confirmed
    /// against FIRST.org's own worked examples in the spec guide (e.g. for
    /// CVE-2003-0818, the guide shows an intermediate Adjusted Temporal of
    /// 8.0, which only matches when this rounding is applied -- the raw
    /// unrounded product is 8.017).
    #[cfg(feature = "std")]
    #[cfg_attr(docsrs, doc(cfg(feature = "std")))]
    pub fn temporal_score_internal(&self, impact: Score) -> Score {
        let base_score = self.base_score_internal(impact).value();
        let e_score = self.e.map(|e| e.score()).unwrap_or(1.0);
        let rl_score = self.rl.map(|rl| rl.score()).unwrap_or(1.0);
        let rc_score = self.rc.map(|rc| rc.score()).unwrap_or(1.0);

        let score = base_score * e_score * rl_score * rc_score;
        Score::new(score).roundup()
    }

    /// Iterate over all defined metrics
    pub fn metrics(&self) -> impl Iterator<Item = (MetricType, &dyn fmt::Debug)> {
        [
            (
                MetricType::AC,
                self.ac.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (
                MetricType::Au,
                self.au.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (
                MetricType::AV,
                self.av.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (MetricType::C, self.c.as_ref().map(|m| m as &dyn fmt::Debug)),
            (MetricType::I, self.i.as_ref().map(|m| m as &dyn fmt::Debug)),
            (MetricType::A, self.a.as_ref().map(|m| m as &dyn fmt::Debug)),
            (
                MetricType::CR,
                self.cr.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (
                MetricType::IR,
                self.ir.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (
                MetricType::AR,
                self.ar.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (
                MetricType::CDP,
                self.cdp.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (
                MetricType::TD,
                self.td.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (MetricType::E, self.e.as_ref().map(|m| m as &dyn fmt::Debug)),
            (
                MetricType::RC,
                self.rc.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
            (
                MetricType::RL,
                self.rl.as_ref().map(|m| m as &dyn fmt::Debug),
            ),
        ]
        .into_iter()
        .filter_map(|(name, metric)| metric.as_ref().map(|&m| (name, m)))
    }

    /// Check for required base metrics presence
    fn check_mandatory_metrics(&self) -> Result<(), Error> {
        fn ensure_present<T>(metric: Option<T>, metric_type: MetricType) -> Result<(), Error> {
            if metric.is_none() {
                return Err(Error::MissingMandatoryMetricV2 { metric_type });
            }
            Ok(())
        }

        ensure_present(self.av.as_ref(), MetricType::AV)?;
        ensure_present(self.ac.as_ref(), MetricType::AC)?;
        ensure_present(self.au.as_ref(), MetricType::Au)?;
        ensure_present(self.c.as_ref(), MetricType::C)?;
        ensure_present(self.i.as_ref(), MetricType::I)?;
        ensure_present(self.a.as_ref(), MetricType::A)?;
        Ok(())
    }
}

macro_rules! write_metrics {
    ($f:expr, $($metric:expr),+) => {
        let mut __first = true;
        $(
            if let Some(metric) = $metric {
                if __first {
                    write!($f, "{}", metric)?;
                    __first = false;
                } else {
                    write!($f, "/{}", metric)?;
                }
            }
        )+
    };
}

impl fmt::Display for Vector {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write_metrics!(
            f, self.av, self.ac, self.au, self.c, self.i, self.a, self.e, self.rl, self.rc,
            self.cdp, self.td, self.cr, self.ir, self.ar
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

        let components = component_vec.iter();
        let mut metrics = Self::default();

        fn get_value<T: FromStr<Err = Error>>(
            metric_type: MetricType,
            current_val: Option<T>,
            new_val: &str,
        ) -> Result<Option<T>, Error> {
            let parsed = T::from_str(new_val)?;
            if current_val.is_some() {
                return Err(Error::DuplicateMetricV2 { metric_type });
            }
            Ok(Some(parsed))
        }

        for &component in components {
            let key = component.0;
            let value = component.1;

            let metric_type = key.parse::<MetricType>()?;
            match metric_type {
                // Base metrics
                MetricType::AV => metrics.av = get_value(metric_type, metrics.av, value)?,
                MetricType::AC => metrics.ac = get_value(metric_type, metrics.ac, value)?,
                MetricType::Au => metrics.au = get_value(metric_type, metrics.au, value)?,
                MetricType::C => metrics.c = get_value(metric_type, metrics.c, value)?,
                MetricType::I => metrics.i = get_value(metric_type, metrics.i, value)?,
                MetricType::A => metrics.a = get_value(metric_type, metrics.a, value)?,

                // Temporal metrics
                MetricType::E => metrics.e = get_value(metric_type, metrics.e, value)?,
                MetricType::RL => metrics.rl = get_value(metric_type, metrics.rl, value)?,
                MetricType::RC => metrics.rc = get_value(metric_type, metrics.rc, value)?,

                // Environmental metrics
                MetricType::CDP => metrics.cdp = get_value(metric_type, metrics.cdp, value)?,
                MetricType::TD => metrics.td = get_value(metric_type, metrics.td, value)?,
                MetricType::CR => metrics.cr = get_value(metric_type, metrics.cr, value)?,
                MetricType::IR => metrics.ir = get_value(metric_type, metrics.ir, value)?,
                MetricType::AR => metrics.ar = get_value(metric_type, metrics.ar, value)?,
            }
        }

        metrics.check_mandatory_metrics()?;

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

#[cfg(all(feature = "std", test))]
mod tests {
    use super::Vector;
    use core::str::FromStr;

    fn approx_eq(a: f64, b: f64) -> bool {
        (a - b).abs() < 0.0001
    }

    /// The reference implementations compute in exact decimal arithmetic and
    /// round ties half up; `9.0 * 0.95` must round to 8.6, not stay at the
    /// `f64` product one ULP below the tie.
    #[test]
    fn temporal_tie_rounds_half_up() {
        let v = "AV:N/AC:L/Au:S/C:C/I:C/A:C/E:F/RL:U/RC:C";
        let cvss = Vector::from_str(v).expect("parse vector");
        assert!(approx_eq(cvss.score().value(), 9.0));
        assert!(
            approx_eq(cvss.temporal_score().value(), 8.6),
            "temporal expected 8.6, got {}",
            cvss.temporal_score().value()
        );
    }

    /// The reference implementation floors the three published scores at
    /// zero, and only those: the adjusted-temporal intermediate inside the
    /// environmental equation stays unclamped, so a negative adjusted base
    /// still lowers the collateral-damage term.
    #[test]
    fn published_scores_clamp_at_zero_but_intermediates_do_not() {
        // Adjusted base is -0.2 here; with CDP:ND the environmental score
        // clamps to 0.0.
        let v = "AV:L/AC:H/Au:M/C:N/I:N/A:P/E:H/RL:ND/TD:H/CR:L/IR:H/AR:L";
        let cvss = Vector::from_str(v).expect("parse vector");
        assert!(
            approx_eq(cvss.environmental_score().value(), 0.0),
            "environmental expected 0.0, got {}",
            cvss.environmental_score().value()
        );

        // Same negative adjusted base, but CDP:MH: the unclamped -0.2 feeds
        // (AT + (10 - AT) * CDP) * TD = (-0.2 + 10.2 * 0.4) * 1.0 = 3.88 → 3.9.
        // A clamped intermediate would give 4.0.
        let v = "AV:L/AC:H/Au:M/C:N/I:P/A:N/E:F/RL:TF/RC:UR/CDP:MH/TD:ND/CR:H/IR:L/AR:L";
        let cvss = Vector::from_str(v).expect("parse vector");
        assert!(
            approx_eq(cvss.environmental_score().value(), 3.9),
            "environmental expected 3.9, got {}",
            cvss.environmental_score().value()
        );
    }

    /// The environmental equation hits the same ties through its collateral
    /// damage and target distribution factors.
    #[test]
    fn environmental_tie_rounds_half_up() {
        let v = "AV:N/AC:M/Au:N/C:C/I:C/A:P/RL:TF/RC:UC/CDP:MH/TD:M/IR:L/AR:H";
        let cvss = Vector::from_str(v).expect("parse vector");
        assert!(
            approx_eq(cvss.environmental_score().value(), 6.2),
            "environmental expected 6.2, got {}",
            cvss.environmental_score().value()
        );
    }

    #[test]
    fn spec_cve_2002_0392() {
        // See https://www.first.org/cvss/v2/guide (Section 3.3.1 worked
        // example). Note NVD's own v2 calculator
        // (https://nvd.nist.gov/vuln-metrics/cvss/v2-calculator) reports an
        // environmental score of 9.1 for this vector, which does not match
        // FIRST.org's own published value of 9.2 for CDP:H/TD:H.
        let v = "AV:N/AC:L/Au:N/C:N/I:N/A:C/E:F/RL:OF/RC:C/CDP:H/TD:H/CR:M/IR:M/AR:H";
        let cvss = Vector::from_str(v).expect("parse vector");

        let base = cvss.score().value();
        let impact = cvss.impact().roundup().value();
        let exploitability = cvss.exploitability().roundup().value();
        let temporal = cvss.temporal_score().value();
        let environmental = cvss.environmental_score().value();
        let adjusted_impact = cvss.adjusted_impact().roundup().value();
        assert!(
            approx_eq(base, 7.8),
            "base score expected 7.8, got {}",
            base
        );
        assert!(
            approx_eq(impact, 6.9),
            "impact expected 6.9, got {}",
            impact
        );
        assert!(
            approx_eq(exploitability, 10.0),
            "exploitability expected 10.0, got {}",
            exploitability
        );
        assert!(
            approx_eq(temporal, 6.4),
            "temporal expected 6.4, got {}",
            temporal
        );
        assert!(
            approx_eq(adjusted_impact, 10.0),
            "adjusted impact expected 10.0, got {}",
            adjusted_impact
        );
        assert!(
            approx_eq(environmental, 9.2),
            "environmental expected 9.2, got {}",
            environmental
        );
    }

    #[test]
    fn spec_cve_2003_0818() {
        // See https://nvd.nist.gov/vuln-metrics/cvss/v2-calculator?vector=(AV:N/AC:L/Au:N/C:C/I:C/A:C/E:F/RL:OF/RC:C/CDP:H/TD:H/CR:M/IR:M/AR:L)
        let v = "AV:N/AC:L/Au:N/C:C/I:C/A:C/E:F/RL:OF/RC:C/CDP:H/TD:H/CR:M/IR:M/AR:L";
        let cvss = Vector::from_str(v).expect("parse vector");

        let base = cvss.score().value();
        let impact = cvss.impact().roundup().value();
        let exploitability = cvss.exploitability().roundup().value();
        let temporal = cvss.temporal_score().value();
        let environmental = cvss.environmental_score().value();
        let adjusted_impact = cvss.adjusted_impact().roundup().value();

        assert!(
            approx_eq(base, 10.0),
            "base score expected 10.0, got {}",
            base
        );
        assert!(
            approx_eq(impact, 10.0),
            "impact expected 10.0, got {}",
            impact
        );
        assert!(
            approx_eq(exploitability, 10.0),
            "exploitability expected 10.0, got {}",
            exploitability
        );
        assert!(
            approx_eq(temporal, 8.3),
            "temporal expected 8.3, got {}",
            temporal
        );
        assert!(
            approx_eq(adjusted_impact, 9.6),
            "adjusted impact expected 9.6, got {}",
            adjusted_impact
        );
        assert!(
            approx_eq(environmental, 9.0),
            "environmental expected 9.0, got {}",
            environmental
        );
    }

    #[test]
    fn spec_cve_2003_0062() {
        // See https://www.first.org/cvss/v2/guide (Section 3.3.3 worked
        // example): the local, high-complexity case, with CDP:H/TD:H for the
        // guide's upper environmental value.
        let v = "AV:L/AC:H/Au:N/C:C/I:C/A:C/E:POC/RL:OF/RC:C/CDP:H/TD:H/CR:M/IR:M/AR:M";
        let cvss = Vector::from_str(v).expect("parse vector");

        let base = cvss.score().value();
        let impact = cvss.impact().roundup().value();
        let exploitability = cvss.exploitability().roundup().value();
        let temporal = cvss.temporal_score().value();
        let environmental = cvss.environmental_score().value();

        assert!(
            approx_eq(base, 6.2),
            "base score expected 6.2, got {}",
            base
        );
        assert!(
            approx_eq(impact, 10.0),
            "impact expected 10.0, got {}",
            impact
        );
        assert!(
            approx_eq(exploitability, 1.9),
            "exploitability expected 1.9, got {}",
            exploitability
        );
        assert!(
            approx_eq(temporal, 4.9),
            "temporal expected 4.9, got {}",
            temporal
        );
        assert!(
            approx_eq(environmental, 7.5),
            "environmental expected 7.5, got {}",
            environmental
        );
    }
}
