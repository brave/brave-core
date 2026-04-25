use crate::v4::scoring::VectorEq;

/// Max severity distances in EQs MacroVectors (+1)
///
/// <https://github.com/FIRSTdotorg/cvss-v4-calculator/blob/c5b0d409ae9f57c44264c6ce5f27d89298e1d32a/max_severity.js>
pub(super) fn max_severity(eq: VectorEq) -> u8 {
    match eq {
        VectorEq::Eq1(0) => 1,
        VectorEq::Eq1(1) => 4,
        VectorEq::Eq1(2) => 5,
        VectorEq::Eq2(0) => 1,
        VectorEq::Eq2(1) => 2,
        VectorEq::Eq3Eq6(0, 0) => 7,
        VectorEq::Eq3Eq6(0, 1) => 6,
        VectorEq::Eq3Eq6(1, 0) => 8,
        VectorEq::Eq3Eq6(1, 1) => 8,
        VectorEq::Eq3Eq6(2, 1) => 10,
        VectorEq::Eq4(0) => 6,
        VectorEq::Eq4(1) => 5,
        VectorEq::Eq4(2) => 4,
        VectorEq::Eq5(0) => 1,
        VectorEq::Eq5(1) => 1,
        VectorEq::Eq5(2) => 1,
        _ => unreachable!("Unexpected vector: {:?}", eq),
    }
}
