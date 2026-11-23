#[test]
#[cfg(any(target_arch = "x86_64", target_arch = "x86"))]
fn compare_with_std() {
    macro_rules! check_feature {
        ($feature:tt) => {
            if cfg!(allow_false_negative) {
                if core_detect::is_x86_feature_detected!($feature) {
                    assert!(std::is_x86_feature_detected!($feature));
                }
            } else {
                assert_eq!(
                    core_detect::is_x86_feature_detected!($feature),
                    std::is_x86_feature_detected!($feature),
                    "core_detect and libstd disagree on {:?}",
                    $feature,
                );
            }
        };
    }
    check_feature!("aes");
    check_feature!("pclmulqdq");
    check_feature!("rdrand");
    check_feature!("rdseed");
    check_feature!("tsc");
    check_feature!("sse");
    check_feature!("sse2");
    check_feature!("sse3");
    check_feature!("ssse3");
    check_feature!("sse4.1");
    check_feature!("sse4.2");
    check_feature!("sse4a");
    check_feature!("sha");
    check_feature!("avx");
    check_feature!("avx2");
    check_feature!("avx512f");
    check_feature!("avx512cd");
    check_feature!("avx512er");
    check_feature!("avx512pf");
    check_feature!("avx512bw");
    check_feature!("avx512dq");
    check_feature!("avx512vl");
    check_feature!("avx512ifma");
    check_feature!("avx512vbmi");
    check_feature!("avx512vpopcntdq");
    check_feature!("fma");
    check_feature!("bmi1");
    check_feature!("bmi2");
    check_feature!("popcnt");
    check_feature!("abm");
    check_feature!("tbm");
    check_feature!("lzcnt");
    check_feature!("xsave");
    check_feature!("xsaveopt");
    check_feature!("xsavec");
    check_feature!("xsaves");
    // Our crate has no issue with these (see `compare_cupidq), but libstd
    // didn't get them until versions after our MSRV.

    // check_feature!("cmpxchg16b");
    // check_feature!("adx");
    // check_feature!("rtm");
}
