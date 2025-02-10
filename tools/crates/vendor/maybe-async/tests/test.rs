#[test]
fn ui() {
    let t = trybuild::TestCases::new();
    t.pass("tests/ui/01-maybe-async.rs");
    t.pass("tests/ui/02-must-be-async.rs");
    t.pass("tests/ui/03-must-be-sync.rs");
    t.pass("tests/ui/04-unit-test-util.rs");
    t.pass("tests/ui/05-replace-future-generic-type-with-output.rs");
    t.pass("tests/ui/06-sync_impl_async_impl.rs");

    t.compile_fail("tests/ui/test_fail/01-empty-test.rs");
    t.compile_fail("tests/ui/test_fail/02-unknown-path.rs");
    t.compile_fail("tests/ui/test_fail/03-async-gt2.rs");
    t.compile_fail("tests/ui/test_fail/04-bad-sync-cond.rs");
}
