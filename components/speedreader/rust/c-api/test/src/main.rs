//! The test runner for the C API tests.

extern "C" {
    fn run_tests() -> i32;
}

fn main() {
    unsafe { std::process::exit(run_tests()) }
}
