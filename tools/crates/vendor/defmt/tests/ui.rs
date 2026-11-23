#[test]
fn ui() {
    // Run UI tests with logging enabled
    std::env::set_var("DEFMT_LOG", "trace");

    // only test error messages on the stable channel (nightly may change too often)
    if rustc_version::version_meta()
        .map(|meta| meta.channel == rustc_version::Channel::Stable)
        .unwrap_or(false)
        && std::env::var_os("SKIP_UI_TESTS").is_none()
    {
        let t = trybuild::TestCases::new();
        t.compile_fail("tests/ui/*.rs");

        t.pass("tests/basic_usage.rs");
        t.pass("tests/derive-bounds.rs");
    }
}
