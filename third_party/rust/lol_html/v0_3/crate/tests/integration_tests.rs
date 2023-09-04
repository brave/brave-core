use cfg_if::cfg_if;

cfg_if! {
    if #[cfg(feature="integration_test")] {
        #[macro_use]
        extern crate serde_derive;

        #[macro_use]
        mod harness;

        mod fixtures;

        use self::fixtures::get_tests;
        use rustc_test::test_main;

        fn main() {
            let args: Vec<_> = ::std::env::args().collect();

            test_main(&args, get_tests());
        }
    } else {
        fn main() {
            println!(concat![
                "Integration tests will not run. ",
                "To run integration tests either run `./scripts/test.sh` ",
                "or pass `--features=integration_test` flag to `cargo test`."
            ]);
        }
    }
}
