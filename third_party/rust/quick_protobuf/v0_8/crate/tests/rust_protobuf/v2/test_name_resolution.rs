// Within test_name_resolution_p0/p1/p2/p3/p4.rs, there are:
//     - Many different messages, which all share the name `B`
//     - Various messages, each of which include some message `B` as a field.

// This series of tests aims to check whether each message includes the
// *expected* version of `B` (from the expected package, from the expected
// module) based on protobuf name resolution rules. As long as they compile, the
// tests should pass; we don't actually need to do anything with the structs.

#[test]
fn test_p0() {
    let test_package_prefixed_field_names =
        super::shared0::shared1::shared2::first0::first1::first2::TestPackagePrefixedFieldNames {
            b: Some(super::shared0::shared1::shared2::second0::second1::second2::B {}),
        };

    let test_non_unique_package_prefixed_field_names =
        super::shared0::shared1::shared2::first0::first1::first2::TestNonUniquePackagePrefixedFieldNames {
            b: Some(super::shared0::shared1::shared2::second0::second1::second2::B {}),
        };

    let test_absolute_field_names =
        super::shared0::shared1::shared2::first0::first1::first2::TestAbsoluteFieldNames {
            b: Some(super::shared0::shared1::shared2::second0::second1::second2::B {}),
        };

    let should_use_internal_b =
        super::shared0::shared1::shared2::first0::first1::first2::ShouldUseInternalB {
            b: Some(super::shared0::shared1::shared2::first0::first1::first2::mod_ShouldUseInternalB::B {}),
        };

    let should_use_same_package_b =
        super::shared0::shared1::shared2::first0::first1::first2::ShouldUseSamePackageB {
            b: Some(super::shared0::shared1::shared2::first0::first1::first2::B {}),
        };
}

/* Nothing to test in `test_name_resolution_p1.rs` */

#[test]
fn test_p2() {
    let should_use_internal_b = super::test_name_resolution_p2::ShouldUseInternalB {
        b: Some(super::test_name_resolution_p2::mod_ShouldUseInternalB::B {}),
    };

    let should_use_same_package_b = super::test_name_resolution_p2::ShouldUseSamePackageB {
        b: Some(super::test_name_resolution_p2::B {}),
    };
}

#[test]
fn test_p3() {
    let should_use_internal_b = super::test_name_resolution_p3::ShouldUseInternalB {
        b: Some(super::test_name_resolution_p3::mod_ShouldUseInternalB::B {}),
    };

    let should_use_different_package_b =
        super::test_name_resolution_p3::ShouldUseDifferentPackageB {
            b: Some(super::test_name_resolution_p2::B {}),
        };
}

#[test]
fn test_p4() {
    let should_use_same_package_b_despite_import =
        super::test_name_resolution_p4::ShouldUseSamePackageBDespiteImport {
            b: Some(super::test_name_resolution_p4::B {}),
        };
}
