#![cfg(all(feature = "v3", feature = "std"))]
/// Base Metrics tests
///
/// NOTE: These CVEs are actually `CVSS:3.0` and have been modified for the
/// purposes of these tests
use core::str::FromStr;

/// CVE-2013-1937
#[test]
fn cve_2013_1937() {
    let cvss_for_cve_2013_1937 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:R/S:C/C:L/I:L/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2013_1937).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2013_1937);
    assert_eq!(base.score().value(), 6.1);
}

/// Missing CVSS:3.1 prefix
#[test]
fn bad_prefix() {
    let cvss_for_cve_2013_1937e = "AV:N/AC:L/PR:N/UI:R/S:C/C:L/I:L/A:N";
    assert!(cvss::v3::Base::from_str(cvss_for_cve_2013_1937e).is_err());
}

/// CVSS:3.0 prefix (parse these as for the purposes of this library they're identical)
#[test]
fn cvss_v3_0_prefix() {
    let cvss_for_cve_2013_1937 = "CVSS:3.0/AV:N/AC:L/PR:N/UI:R/S:C/C:L/I:L/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2013_1937).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2013_1937);
    assert_eq!(base.score().value(), 6.1);
}

/// CVE-2013-0375
#[test]
fn cve_2013_0375() {
    let cvss_for_cve_2013_0375 = "CVSS:3.1/AV:N/AC:L/PR:L/UI:N/S:C/C:L/I:L/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2013_0375).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2013_0375);
    assert_eq!(base.score().value(), 6.4);
}

/// CVE-2014-3566
#[test]
fn cve_2014_3566() {
    let cvss_for_cve_2014_3566 = "CVSS:3.1/AV:N/AC:H/PR:N/UI:R/S:U/C:L/I:N/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2014_3566).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2014_3566);
    assert_eq!(base.score().value(), 3.1);
}

/// CVE-2012-1516
#[test]
fn cve_2012_1516() {
    let cvss_for_cve_2012_1516 = "CVSS:3.1/AV:N/AC:L/PR:L/UI:N/S:C/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2012_1516).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2012_1516);
    assert_eq!(base.score().value(), 9.9);
}

/// CVE-2009-0783
#[test]
fn cve_2009_0783() {
    let cvss_for_cve_2009_0783 = "CVSS:3.1/AV:L/AC:L/PR:H/UI:N/S:U/C:L/I:L/A:L";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2009_0783).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2009_0783);
    assert_eq!(base.score().value(), 4.2);
}

/// CVE-2012-0384
#[test]
fn cve_2012_0384() {
    let cvss_for_cve_2012_0384 = "CVSS:3.1/AV:N/AC:L/PR:L/UI:N/S:U/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2012_0384).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2012_0384);
    assert_eq!(base.score().value(), 8.8);
}

/// CVE-2015-1098
#[test]
fn cve_2015_1098() {
    let cvss_for_cve_2015_1098 = "CVSS:3.1/AV:L/AC:L/PR:N/UI:R/S:U/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2015_1098).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2015_1098);
    assert_eq!(base.score().value(), 7.8);
}

/// CVE-2014-0160
#[test]
fn cve_2014_0160() {
    let cvss_for_cve_2014_0160 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:N/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2014_0160).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2014_0160);
    assert_eq!(base.score().value(), 7.5);
}

/// CVE-2014-6271
#[test]
fn cve_2014_6271() {
    let cvss_for_cve_2014_6271 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2014_6271).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2014_6271);
    assert_eq!(base.score().value(), 9.8);
}

/// CVE-2008-1447
#[test]
fn cve_2008_1447() {
    let cvss_for_cve_2008_1447 = "CVSS:3.1/AV:N/AC:H/PR:N/UI:N/S:C/C:N/I:H/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2008_1447).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2008_1447);
    assert_eq!(base.score().value(), 6.8);
}

/// CVE-2014-2005
#[test]
fn cve_2014_2005() {
    let cvss_for_cve_2014_2005 = "CVSS:3.1/AV:P/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2014_2005).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2014_2005);
    assert_eq!(base.score().value(), 6.8);
}

/// CVE-2010-0467
#[test]
fn cve_2010_0467() {
    let cvss_for_cve_2010_0467 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:C/C:L/I:N/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2010_0467).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2010_0467);
    assert_eq!(base.score().value(), 5.8);
}

/// CVE-2012-1342
#[test]
fn cve_2012_1342() {
    let cvss_for_cve_2012_1342 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:C/C:N/I:L/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2012_1342).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2012_1342);
    assert_eq!(base.score().value(), 5.8);
}

/// CVE-2013-6014
#[test]
fn cve_2013_6014() {
    let cvss_for_cve_2013_6014 = "CVSS:3.1/AV:A/AC:L/PR:N/UI:N/S:C/C:H/I:N/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2013_6014).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2013_6014);
    assert_eq!(base.score().value(), 9.3);
}

/// CVE-2014-9253
#[test]
fn cve_2014_9253() {
    let cvss_for_cve_2014_9253 = "CVSS:3.1/AV:N/AC:L/PR:L/UI:R/S:C/C:L/I:L/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2014_9253).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2014_9253);
    assert_eq!(base.score().value(), 5.4);
}

/// CVE-2009-0658
#[test]
fn cve_2009_0658() {
    let cvss_for_cve_2009_0658 = "CVSS:3.1/AV:L/AC:L/PR:N/UI:R/S:U/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2009_0658).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2009_0658);
    assert_eq!(base.score().value(), 7.8);
}

/// CVE-2011-1265
#[test]
fn cve_2011_1265() {
    let cvss_for_cve_2011_1265 = "CVSS:3.1/AV:A/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2011_1265).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2011_1265);
    assert_eq!(base.score().value(), 8.8);
}

/// CVE-2014-2019
#[test]
fn cve_2014_2019() {
    let cvss_for_cve_2014_2019 = "CVSS:3.1/AV:P/AC:L/PR:N/UI:N/S:U/C:N/I:H/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2014_2019).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2014_2019);
    assert_eq!(base.score().value(), 4.6);
}

/// CVE-2015-0970
#[test]
fn cve_2015_0970() {
    let cvss_for_cve_2015_0970 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:R/S:U/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2015_0970).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2015_0970);
    assert_eq!(base.score().value(), 8.8);
}

/// CVE-2014-0224
#[test]
fn cve_2014_0224() {
    let cvss_for_cve_2014_0224 = "CVSS:3.1/AV:N/AC:H/PR:N/UI:N/S:U/C:H/I:H/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2014_0224).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2014_0224);
    assert_eq!(base.score().value(), 7.4);
}

/// CVE-2012-5376
#[test]
fn cve_2012_5376() {
    let cvss_for_cve_2012_5376 = "CVSS:3.1/AV:N/AC:L/PR:N/UI:R/S:C/C:H/I:H/A:H";
    let base = cvss::v3::Base::from_str(cvss_for_cve_2012_5376).unwrap();
    assert_eq!(&base.to_string(), cvss_for_cve_2012_5376);
    assert_eq!(base.score().value(), 9.6);
}

/// No impact scope changed
#[test]
fn no_impact_scope_changed() {
    // https://www.first.org/cvss/calculator/3.1#CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:C/C:N/I:N/A:N => 0.0
    let cvss_for_no_impact_scope_changed = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:C/C:N/I:N/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_no_impact_scope_changed).unwrap();
    assert_eq!(&base.to_string(), cvss_for_no_impact_scope_changed);
    assert_eq!(base.score().value(), 0.0);
}

/// No impact scope unchanged
#[test]
fn no_impact_scope_unchanged() {
    // https://www.first.org/cvss/calculator/3.1#CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N => 0.0
    let cvss_for_no_impact_scope_unchanged = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N";
    let base = cvss::v3::Base::from_str(cvss_for_no_impact_scope_unchanged).unwrap();
    assert_eq!(&base.to_string(), cvss_for_no_impact_scope_unchanged);
    assert_eq!(base.score().value(), 0.0);
}

#[test]
fn low_scope_unchanged() {
    // https://www.first.org/cvss/calculator/3.1#CVSS:3.1/AV:P/AC:H/PR:H/UI:R/S:U/C:N/I:N/A:L => 1.6
    let cvss_for_low_scope_unchanged = "CVSS:3.1/AV:P/AC:H/PR:H/UI:R/S:U/C:N/I:N/A:L";
    let base = cvss::v3::Base::from_str(cvss_for_low_scope_unchanged).unwrap();
    assert_eq!(&base.to_string(), cvss_for_low_scope_unchanged);
    assert_eq!(base.score().value(), 1.6);
}

#[test]
fn low_scope_changed() {
    // https://www.first.org/cvss/calculator/3.1#CVSS:3.1/AV:P/AC:H/PR:H/UI:R/S:C/C:N/I:N/A:L => 1.8
    let cvss_for_low_scope_changed = "CVSS:3.1/AV:P/AC:H/PR:H/UI:R/S:C/C:N/I:N/A:L";
    let base = cvss::v3::Base::from_str(cvss_for_low_scope_changed).unwrap();
    assert_eq!(&base.to_string(), cvss_for_low_scope_changed);
    assert_eq!(base.score().value(), 1.8);
}
