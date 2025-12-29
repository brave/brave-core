//! Regression Tests
//!
//! NOTE: These CVEs are actually `CVSS:4.0` and have been picked from real
//! practical cases not covered by Red Hat

#![cfg(all(feature = "v4", feature = "std"))]
use {cvss::v4::Vector, std::str::FromStr};

/// CVE-2025-47273
#[test]
fn cve_2025_47273() {
    let cvss_for_cve_2025_47273 = concat!(
        "CVSS:4.0/AV:N/AC:L/AT:N/PR:N/UI:N/VC:N/VI:H/VA:N/SC:N/SI:N/SA:N",
        "/E:P/CR:X/IR:X/AR:X",
        "/MAV:X/MAC:X/MAT:X/MPR:X/MUI:X/MVC:X/MVI:X/MVA:X/MSC:X/MSI:X/MSA:X",
        "/S:X/AU:X/R:X/V:X/RE:X/U:X",
    );
    let base = Vector::from_str(cvss_for_cve_2025_47273).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2025_47273);
    assert_eq!(base.score().value(), 7.7);
}
