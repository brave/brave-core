//! Score computation

use crate::v4::{
    MetricType, Vector,
    metric::{
        MetricLevel,
        base::{
            MergedAttackComplexity, MergedAttackRequirements, MergedAttackVector,
            MergedAvailabilityImpactToTheSubsequentSystem,
            MergedAvailabilityImpactToTheVulnerableSystem,
            MergedConfidentialityImpactToTheSubsequentSystem,
            MergedConfidentialityImpactToTheVulnerableSystem,
            MergedIntegrityImpactToTheSubsequentSystem, MergedIntegrityImpactToTheVulnerableSystem,
            MergedPrivilegesRequired, MergedUserInteraction,
        },
        environmental::{
            MergedAvailabilityRequirements, MergedConfidentialityRequirements,
            MergedIntegrityRequirements,
        },
        supplemental::{
            Automatable, ProviderUrgency, Recovery, Safety, ValueDensity,
            VulnerabilityResponseEffort,
        },
        threat::MergedExploitMaturity,
    },
    scoring::{lookup::lookup_global, max_composed::max_composed, max_severity::max_severity},
};
use alloc::vec::Vec;
use std::{format, vec};

mod lookup;
mod max_composed;
mod max_severity;

#[derive(Hash, Debug)]
pub(crate) enum VectorEq {
    Eq1(u8),
    Eq2(u8),
    Eq3Eq6(u8, u8),
    Eq4(u8),
    Eq5(u8),
}

/// `ScoringVector` contains the data necessary for scoring, as it is transformed
/// before the computations, and the types are subtly different.
///
/// Contains the result of the `m()` function of the reference implementation.
#[derive(Default)]
pub(crate) struct ScoringVector {
    // Base scores overridden by modified scores
    ac: MergedAttackComplexity,
    at: MergedAttackRequirements,
    av: MergedAttackVector,
    pr: MergedPrivilegesRequired,
    sa: MergedAvailabilityImpactToTheSubsequentSystem,
    sc: MergedConfidentialityImpactToTheSubsequentSystem,
    si: MergedIntegrityImpactToTheSubsequentSystem,
    ui: MergedUserInteraction,
    va: MergedAvailabilityImpactToTheVulnerableSystem,
    vc: MergedConfidentialityImpactToTheVulnerableSystem,
    vi: MergedIntegrityImpactToTheVulnerableSystem,

    e: MergedExploitMaturity,

    ar: MergedAvailabilityRequirements,
    cr: MergedConfidentialityRequirements,
    ir: MergedIntegrityRequirements,

    au: Automatable,
    r: Recovery,
    re: VulnerabilityResponseEffort,
    s: Safety,
    u: ProviderUrgency,
    v: ValueDensity,
}

impl From<&Vector> for ScoringVector {
    fn from(v: &Vector) -> Self {
        Self {
            // All of these are unwrapped because they are mandatory
            // and the constructor ensures they are present.
            ac: v.ac.unwrap().merge(v.mac),
            at: v.at.unwrap().merge(v.mat),
            av: v.av.unwrap().merge(v.mav),
            pr: v.pr.unwrap().merge(v.mpr),
            sa: v.sa.unwrap().merge(v.msa),
            sc: v.sc.unwrap().merge(v.msc),
            si: v.si.unwrap().merge(v.msi),
            ui: v.ui.unwrap().merge(v.mui),
            va: v.va.unwrap().merge(v.mva),
            vc: v.vc.unwrap().merge(v.mvc),
            vi: v.vi.unwrap().merge(v.mvi),
            e: v.e.unwrap_or_default().merge(),
            ar: v.ar.unwrap_or_default().merge(),
            cr: v.cr.unwrap_or_default().merge(),
            ir: v.ir.unwrap_or_default().merge(),
            au: v.au.unwrap_or_default(),
            r: v.r.unwrap_or_default(),
            re: v.re.unwrap_or_default(),
            s: v.s.unwrap_or_default(),
            u: v.u.unwrap_or_default(),
            v: v.v.unwrap_or_default(),
        }
    }
}

impl ScoringVector {
    /// Read partial vectors from a string, to be used with `max_composed`
    /// output (as it contains `Merge` values, like `SI:S`).
    fn from_max_composed_vector(s: &str) -> Self {
        let s = s.trim_end_matches('/');

        let components: Vec<(&str, &str)> = s
            .split('/')
            .map(|component| {
                let mut parts = component.split(':');
                let id = parts.next().unwrap();
                let value = parts.next().unwrap();
                (id, value)
            })
            .collect();

        let mut metrics = Self {
            ..Default::default()
        };
        for component in components {
            let id = component.0.to_ascii_uppercase();
            let value = component.1.to_ascii_uppercase();

            match id.parse::<MetricType>().unwrap() {
                MetricType::AV => metrics.av = value.parse().unwrap(),
                MetricType::AC => metrics.ac = value.parse().unwrap(),
                MetricType::PR => metrics.pr = value.parse().unwrap(),
                MetricType::UI => metrics.ui = value.parse().unwrap(),
                MetricType::S => metrics.s = value.parse().unwrap(),
                MetricType::AT => metrics.at = value.parse().unwrap(),
                MetricType::SA => metrics.sa = value.parse().unwrap(),
                MetricType::SC => metrics.sc = value.parse().unwrap(),
                MetricType::SI => metrics.si = value.parse().unwrap(),
                MetricType::VA => metrics.va = value.parse().unwrap(),
                MetricType::VC => metrics.vc = value.parse().unwrap(),
                MetricType::VI => metrics.vi = value.parse().unwrap(),
                MetricType::E => metrics.e = value.parse().unwrap(),
                MetricType::AR => metrics.ar = value.parse().unwrap(),
                MetricType::CR => metrics.cr = value.parse().unwrap(),
                MetricType::IR => metrics.ir = value.parse().unwrap(),
                MetricType::AU => metrics.au = value.parse().unwrap(),
                MetricType::R => metrics.r = value.parse().unwrap(),
                MetricType::RE => metrics.re = value.parse().unwrap(),
                MetricType::U => metrics.u = value.parse().unwrap(),
                MetricType::V => metrics.v = value.parse().unwrap(),
                _ => unreachable!("Invalid metric type in max_composed: {}", id),
            }
        }
        metrics
    }

    fn eq1(&self) -> u8 {
        // EQ1: 0-                                    AV:N and PR:N and UI:N
        //      1-    (AV:N or PR:N or UI:N) and not (AV:N and PR:N and UI:N) and not
        // AV:P      2- not(AV:N or PR:N or UI:N) or
        // AV:P
        if self.av == MergedAttackVector::Network
            && self.pr == MergedPrivilegesRequired::None
            && self.ui == MergedUserInteraction::None
        {
            0
        } else if (self.av == MergedAttackVector::Network
            || self.pr == MergedPrivilegesRequired::None
            || self.ui == MergedUserInteraction::None)
            && self.av != MergedAttackVector::Physical
        {
            1
        } else {
            2
        }
    }

    fn eq2(&self) -> u8 {
        // EQ2: 0-    (AC:L and AT:N)
        //      1-(not(AC:L and AT:N))
        if self.ac == MergedAttackComplexity::Low && self.at == MergedAttackRequirements::None {
            0
        } else {
            1
        }
    }

    fn eq3(&self) -> u8 {
        // EQ3: 0-    (VC:H and VI:H)
        //      1-(not(VC:H and VI:H) and (VC:H or VI:H or VA:H))
        //      2-                    not (VC:H or VI:H or VA:H)
        if self.vc == MergedConfidentialityImpactToTheVulnerableSystem::High
            && self.vi == MergedIntegrityImpactToTheVulnerableSystem::High
        {
            0
        } else if self.vc == MergedConfidentialityImpactToTheVulnerableSystem::High
            || self.vi == MergedIntegrityImpactToTheVulnerableSystem::High
            || self.va == MergedAvailabilityImpactToTheVulnerableSystem::High
        {
            1
        } else {
            2
        }
    }

    fn eq4(&self) -> u8 {
        // EQ4: 0-    (MSI:S or MSA:S)
        //      1-not (MSI:S or MSA:S) and     (SC:H or SI:H or SA:H)
        //      2-not (MSI:S or MSA:S) and not (SC:H or SI:H or SA:H)
        if self.si == MergedIntegrityImpactToTheSubsequentSystem::Safety
            || self.sa == MergedAvailabilityImpactToTheSubsequentSystem::Safety
        {
            0
        } else if self.sc == MergedConfidentialityImpactToTheSubsequentSystem::High
            || self.si == MergedIntegrityImpactToTheSubsequentSystem::High
            || self.sa == MergedAvailabilityImpactToTheSubsequentSystem::High
        {
            1
        } else {
            2
        }
    }

    fn eq5(&self) -> u8 {
        // EQ5: 0-E:A
        //      1-E:P
        //      2-E:U
        if self.e == MergedExploitMaturity::Attacked {
            0
        } else if self.e == MergedExploitMaturity::ProofOfConcept {
            1
        } else {
            2
        }
    }

    fn eq6(&self) -> u8 {
        // EQ6: 0-    (CR:H and VC:H) or (IR:H and VI:H) or (AR:H and VA:H)
        //      1-not[(CR:H and VC:H) or (IR:H and VI:H) or (AR:H and VA:H)]
        if (self.cr == MergedConfidentialityRequirements::High
            && self.vc == MergedConfidentialityImpactToTheVulnerableSystem::High)
            || (self.ir == MergedIntegrityRequirements::High
                && self.vi == MergedIntegrityImpactToTheVulnerableSystem::High)
            || (self.ar == MergedAvailabilityRequirements::High
                && self.va == MergedAvailabilityImpactToTheVulnerableSystem::High)
        {
            0
        } else {
            1
        }
    }

    fn macro_vector(&self) -> MacroVector {
        MacroVector::new(
            self.eq1(),
            self.eq2(),
            self.eq3(),
            self.eq4(),
            self.eq5(),
            self.eq6(),
        )
    }

    pub(crate) fn score(&self) -> f64 {
        // Exception for no impact on system (shortcut)
        if self.vc == MergedConfidentialityImpactToTheVulnerableSystem::None
            && self.vi == MergedIntegrityImpactToTheVulnerableSystem::None
            && self.va == MergedAvailabilityImpactToTheVulnerableSystem::None
            && self.sc == MergedConfidentialityImpactToTheSubsequentSystem::None
            && self.si == MergedIntegrityImpactToTheSubsequentSystem::None
            && self.sa == MergedAvailabilityImpactToTheSubsequentSystem::None
        {
            return 0.;
        }

        let macro_vector = self.macro_vector();

        let value = lookup_global(&macro_vector).unwrap();

        // 1. For each of the EQs:
        //   a. The maximal scoring difference is determined as the difference
        //      between the current MacroVector and the lower MacroVector.
        //     i. If there is no lower MacroVector the available distance is
        //        set to NaN and then ignored in the further calculations.

        // compute next lower macro, it can also not exist
        // get their score, if the next lower macro score do not exist the result is
        // None
        let score_eq1_next_lower_macro = lookup_global(&macro_vector.incr_eq1());
        let score_eq2_next_lower_macro = lookup_global(&macro_vector.incr_eq2());

        // eq3 and eq6 are related
        let score_eq3eq6_next_lower_macro = if macro_vector.eq3 == 1 && macro_vector.eq6 == 1 {
            // 11 --> 21
            lookup_global(&macro_vector.incr_eq3())
        } else if macro_vector.eq3 == 0 && macro_vector.eq6 == 1 {
            // 01 --> 11
            lookup_global(&macro_vector.incr_eq3())
        } else if macro_vector.eq3 == 1 && macro_vector.eq6 == 0 {
            // 10 --> 11
            lookup_global(&macro_vector.incr_eq6())
        } else if macro_vector.eq3 == 0 && macro_vector.eq6 == 0 {
            // 00 --> 01
            // 00 --> 10
            let score_eq3eq6_next_lower_macro_left = lookup_global(&macro_vector.incr_eq6());
            let score_eq3eq6_next_lower_macro_right = lookup_global(&macro_vector.incr_eq3());

            if score_eq3eq6_next_lower_macro_left > score_eq3eq6_next_lower_macro_right {
                score_eq3eq6_next_lower_macro_left
            } else {
                score_eq3eq6_next_lower_macro_right
            }
        } else {
            // 21 --> 32 (do not exist)
            lookup_global(&macro_vector.incr_eq3())
        };

        let score_eq4_next_lower_macro = lookup_global(&macro_vector.incr_eq4());
        let score_eq5_next_lower_macro = lookup_global(&macro_vector.incr_eq5());

        //   b. The severity distance of the to-be scored vector from a
        //      highest severity vector in the same MacroVector is determined.

        let eq1_maxes = max_composed(VectorEq::Eq1(macro_vector.eq1));
        let eq2_maxes = max_composed(VectorEq::Eq2(macro_vector.eq2));
        let eq3_eq6_maxes = max_composed(VectorEq::Eq3Eq6(macro_vector.eq3, macro_vector.eq6));
        let eq4_maxes = max_composed(VectorEq::Eq4(macro_vector.eq4));
        let eq5_maxes = max_composed(VectorEq::Eq5(macro_vector.eq5));

        // compose them
        let mut max_vectors = vec![];
        for eq1_max in &eq1_maxes {
            for eq2_max in &eq2_maxes {
                for eq3_eq6_max in &eq3_eq6_maxes {
                    for eq4_max in &eq4_maxes {
                        for eq5max in &eq5_maxes {
                            max_vectors
                                .push(format!("{eq1_max}{eq2_max}{eq3_eq6_max}{eq4_max}{eq5max}"))
                        }
                    }
                }
            }
        }

        let mut severity_distance_av = 0.;
        let mut severity_distance_pr = 0.;
        let mut severity_distance_ui = 0.;
        let mut severity_distance_ac = 0.;
        let mut severity_distance_at = 0.;
        let mut severity_distance_vc = 0.;
        let mut severity_distance_vi = 0.;
        let mut severity_distance_va = 0.;
        let mut severity_distance_sc = 0.;
        let mut severity_distance_si = 0.;
        let mut severity_distance_sa = 0.;
        let mut severity_distance_cr = 0.;
        let mut severity_distance_ir = 0.;
        let mut severity_distance_ar = 0.;
        // Find the max vector to use i.e. one in the combination of all the highest
        // that is greater or equal (severity distance) than the to-be scored vector.
        for max_vector in &max_vectors {
            let max_vector = max_vector.trim_end_matches('/');
            let v = ScoringVector::from_max_composed_vector(max_vector);

            severity_distance_av = self.av.level() - v.av.level();
            severity_distance_pr = self.pr.level() - v.pr.level();
            severity_distance_ui = self.ui.level() - v.ui.level();
            severity_distance_ac = self.ac.level() - v.ac.level();
            severity_distance_at = self.at.level() - v.at.level();
            severity_distance_vc = self.vc.level() - v.vc.level();
            severity_distance_vi = self.vi.level() - v.vi.level();
            severity_distance_va = self.va.level() - v.va.level();
            severity_distance_sc = self.sc.level() - v.sc.level();
            severity_distance_si = self.si.level() - v.si.level();
            severity_distance_sa = self.sa.level() - v.sa.level();
            severity_distance_cr = self.cr.level() - v.cr.level();
            severity_distance_ir = self.ir.level() - v.ir.level();
            severity_distance_ar = self.ar.level() - v.ar.level();

            // if any is less than zero this is not the right max
            if severity_distance_av < 0.
                || severity_distance_pr < 0.
                || severity_distance_ui < 0.
                || severity_distance_ac < 0.
                || severity_distance_at < 0.
                || severity_distance_vc < 0.
                || severity_distance_vi < 0.
                || severity_distance_va < 0.
                || severity_distance_sc < 0.
                || severity_distance_si < 0.
                || severity_distance_sa < 0.
                || severity_distance_cr < 0.
                || severity_distance_ir < 0.
                || severity_distance_ar < 0.
            {
                continue;
            } else {
                // if multiple maxes exist to reach it it is enough the first one
                break;
            }
        }

        let current_severity_distance_eq1 =
            severity_distance_av + severity_distance_pr + severity_distance_ui;
        let current_severity_distance_eq2 = severity_distance_ac + severity_distance_at;
        let current_severity_distance_eq3eq6 = severity_distance_vc
            + severity_distance_vi
            + severity_distance_va
            + severity_distance_cr
            + severity_distance_ir
            + severity_distance_ar;
        let current_severity_distance_eq4 =
            severity_distance_sc + severity_distance_si + severity_distance_sa;

        let step = 0.1;

        // if the next lower macro score does not exist the result is None
        let available_distance_eq1 = score_eq1_next_lower_macro.map(|v| value - v);
        let available_distance_eq2 = score_eq2_next_lower_macro.map(|v| value - v);
        let available_distance_eq3eq6 = score_eq3eq6_next_lower_macro.map(|v| value - v);
        let available_distance_eq4 = score_eq4_next_lower_macro.map(|v| value - v);
        let available_distance_eq5 = score_eq5_next_lower_macro.map(|v| value - v);

        // some of them do not exist, we will find them by retrieving the score. If
        // score null then do not exist.
        let mut n_existing_lower = 0;

        // multiply by step because distance is pure
        let max_severity_eq1 = max_severity(VectorEq::Eq1(macro_vector.eq1)) as f64 * step;
        let max_severity_eq2 = max_severity(VectorEq::Eq2(macro_vector.eq2)) as f64 * step;
        let max_severity_eq3eq6 =
            max_severity(VectorEq::Eq3Eq6(macro_vector.eq3, macro_vector.eq6)) as f64 * step;
        let max_severity_eq4 = max_severity(VectorEq::Eq4(macro_vector.eq4)) as f64 * step;

        //   c. The proportion of the distance is determined by dividing
        //      the severity distance of the to-be-scored vector by the depth
        //      of the MacroVector.
        //   d. The maximal scoring difference is multiplied by the proportion of
        //      distance.
        let normalized_severity_eq1 = if let Some(a) = available_distance_eq1 {
            n_existing_lower += 1;
            let percent_to_next_eq1_severity = (current_severity_distance_eq1) / max_severity_eq1;
            a * percent_to_next_eq1_severity
        } else {
            0.
        };

        let normalized_severity_eq2 = if let Some(a) = available_distance_eq2 {
            n_existing_lower += 1;
            let percent_to_next_eq2_severity = (current_severity_distance_eq2) / max_severity_eq2;
            a * percent_to_next_eq2_severity
        } else {
            0.
        };

        let normalized_severity_eq3eq6 = if let Some(a) = available_distance_eq3eq6 {
            n_existing_lower += 1;
            let percent_to_next_eq3eq6_severity =
                (current_severity_distance_eq3eq6) / max_severity_eq3eq6;
            a * percent_to_next_eq3eq6_severity
        } else {
            0.
        };

        let normalized_severity_eq4 = if let Some(a) = available_distance_eq4 {
            n_existing_lower += 1;
            let percent_to_next_eq4_severity = (current_severity_distance_eq4) / max_severity_eq4;
            a * percent_to_next_eq4_severity
        } else {
            0.
        };

        let normalized_severity_eq5 = if let Some(a) = available_distance_eq5 {
            // for eq5 is always 0 the percentage
            n_existing_lower += 1;
            let percent_to_next_eq5_severity = 0.;
            a * percent_to_next_eq5_severity
        } else {
            0.
        };

        // 2. The mean of the above computed proportional distances is computed.
        let mean_distance = if n_existing_lower == 0 {
            0.
        } else {
            // sometimes we need to go up but there is nothing there, or down but there is
            // nothing there so it's a change of 0.
            (normalized_severity_eq1
                + normalized_severity_eq2
                + normalized_severity_eq3eq6
                + normalized_severity_eq4
                + normalized_severity_eq5)
                / n_existing_lower as f64
        };

        // 3. The score of the vector is the score of the MacroVector (i.e. the score of
        //    the highest severity vector) minus the mean distance so computed. This
        //    score is rounded to one decimal place.
        value - mean_distance
    }
}

#[derive(Clone, Hash, PartialEq, Eq, Debug)]
struct MacroVector {
    eq1: u8,
    eq2: u8,
    eq3: u8,
    eq4: u8,
    eq5: u8,
    eq6: u8,
}

impl MacroVector {
    fn new(eq1: u8, eq2: u8, eq3: u8, eq4: u8, eq5: u8, eq6: u8) -> Self {
        MacroVector {
            eq1,
            eq2,
            eq3,
            eq4,
            eq5,
            eq6,
        }
    }

    fn as_tuple(&self) -> (u8, u8, u8, u8, u8, u8) {
        (self.eq1, self.eq2, self.eq3, self.eq4, self.eq5, self.eq6)
    }

    fn incr_eq1(&self) -> Self {
        Self {
            eq1: self.eq1 + 1,
            ..*self
        }
    }

    fn incr_eq2(&self) -> Self {
        Self {
            eq2: self.eq2 + 1,
            ..*self
        }
    }

    fn incr_eq3(&self) -> Self {
        Self {
            eq3: self.eq3 + 1,
            ..*self
        }
    }

    fn incr_eq4(&self) -> Self {
        Self {
            eq4: self.eq4 + 1,
            ..*self
        }
    }

    fn incr_eq5(&self) -> Self {
        Self {
            eq5: self.eq5 + 1,
            ..*self
        }
    }

    fn incr_eq6(&self) -> Self {
        Self {
            eq6: self.eq6 + 1,
            ..*self
        }
    }
}
