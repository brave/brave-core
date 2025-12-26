use crate::v4::scoring::VectorEq;
use alloc::vec::Vec;
use std::vec;

/// Highest severity vectors
///
/// <https://github.com/FIRSTdotorg/cvss-v4-calculator/blob/c5b0d409ae9f57c44264c6ce5f27d89298e1d32a/max_composed.js>
pub(super) fn max_composed(eq: VectorEq) -> Vec<&'static str> {
    match eq {
        VectorEq::Eq1(0) => vec!["AV:N/PR:N/UI:N/"],
        VectorEq::Eq1(1) => vec!["AV:A/PR:N/UI:N/", "AV:N/PR:L/UI:N/", "AV:N/PR:N/UI:P/"],
        VectorEq::Eq1(2) => vec!["AV:P/PR:N/UI:N/", "AV:A/PR:L/UI:P/"],
        VectorEq::Eq2(0) => vec!["AC:L/AT:N/"],
        VectorEq::Eq2(1) => vec!["AC:H/AT:N/", "AC:L/AT:P/"],
        VectorEq::Eq3Eq6(0, 0) => vec!["VC:H/VI:H/VA:H/CR:H/IR:H/AR:H/"],
        VectorEq::Eq3Eq6(0, 1) => vec![
            "VC:H/VI:H/VA:L/CR:M/IR:M/AR:H/",
            "VC:H/VI:H/VA:H/CR:M/IR:M/AR:M/",
        ],
        VectorEq::Eq3Eq6(1, 0) => vec![
            "VC:L/VI:H/VA:H/CR:H/IR:H/AR:H/",
            "VC:H/VI:L/VA:H/CR:H/IR:H/AR:H/",
        ],
        VectorEq::Eq3Eq6(1, 1) => vec![
            "VC:L/VI:H/VA:L/CR:H/IR:M/AR:H/",
            "VC:L/VI:H/VA:H/CR:H/IR:M/AR:M/",
            "VC:H/VI:L/VA:H/CR:M/IR:H/AR:M/",
            "VC:H/VI:L/VA:L/CR:M/IR:H/AR:H/",
            "VC:L/VI:L/VA:H/CR:H/IR:H/AR:M/",
        ],
        VectorEq::Eq3Eq6(2, 1) => vec!["VC:L/VI:L/VA:L/CR:H/IR:H/AR:H/"],
        VectorEq::Eq4(0) => vec!["SC:H/SI:S/SA:S/"],
        VectorEq::Eq4(1) => vec!["SC:H/SI:H/SA:H/"],
        VectorEq::Eq4(2) => vec!["SC:L/SI:L/SA:L/"],
        VectorEq::Eq5(0) => vec!["E:A/"],
        VectorEq::Eq5(1) => vec!["E:P/"],
        VectorEq::Eq5(2) => vec!["E:U/"],
        _ => unreachable!("Unexpected vector: {:?}", eq),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::v4::scoring::ScoringVector;
    use crate::v4::scoring::VectorEq::*;
    use std::format;

    #[test]
    fn all_combination_are_valid_scoring_vectors() {
        for eq1 in 0..3 {
            for eq2 in 0..2 {
                for eq3 in 0..3 {
                    for eq4 in 0..3 {
                        for eq5 in 0..3 {
                            for eq6 in 0..2 {
                                if eq3 == 2 && eq6 == 0 {
                                    // impossible combination
                                    continue;
                                }
                                let eq1_vals = max_composed(Eq1(eq1));
                                let eq2_vals = max_composed(Eq2(eq2));
                                let eq3_eq6_vals = max_composed(Eq3Eq6(eq3, eq6));
                                let eq4_vals = max_composed(Eq4(eq4));
                                let eq5_vals = max_composed(Eq5(eq5));

                                for eq1_val in eq1_vals.iter() {
                                    for eq2_val in eq2_vals.iter() {
                                        for eq3_eq6_val in eq3_eq6_vals.iter() {
                                            for eq4_val in eq4_vals.iter() {
                                                for eq5_val in eq5_vals.iter() {
                                                    let scoring_vector = format!(
                                                        "{}{}{}{}{}",
                                                        eq1_val,
                                                        eq2_val,
                                                        eq3_eq6_val,
                                                        eq4_val,
                                                        eq5_val
                                                    );

                                                    // ensure the conversion does not panic.
                                                    assert!(
                                                        ScoringVector::from_max_composed_vector(
                                                            &scoring_vector
                                                        )
                                                        .score()
                                                            > 0.0
                                                    );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
