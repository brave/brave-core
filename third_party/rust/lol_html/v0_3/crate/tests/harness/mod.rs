use lazy_static::lazy_static;
use std::sync::Mutex;

lazy_static! {
    pub static ref TEST_CRITICAL_SECTION_MUTEX: Mutex<()> = Mutex::new(());
}

macro_rules! ignore {
    (@info $($args:expr),+) => {
        if std::env::var("IGNORES_VERBOSE").is_ok() {
            println!($($args),+);
        }
    };

    (@total $type:expr, $count:expr) => {
        println!("Ignoring {} {} tests, run with `IGNORES_VERBOSE=1` to get more info.", $count, $type);
    };
}

mod input;

#[macro_use]
pub mod suites;

pub use self::input::Input;

pub trait TestFixture<T> {
    fn test_cases() -> Vec<T>;
    fn run(test: &T);
}

macro_rules! create_test {
    ($name:expr, $should_panic:expr, $body:tt) => {{
        use rustc_test::{TestDesc, TestDescAndFn, TestFn, TestName};

        TestDescAndFn {
            desc: TestDesc {
                name: TestName::DynTestName($name),
                ignore: false,
                should_panic: $should_panic,
                allow_fail: false,
            },
            testfn: TestFn::DynTestFn(Box::new(move || $body)),
        }
    }};
}

macro_rules! test_fixture {
    ($fixture:ident) => {
        use rustc_test::{ShouldPanic, TestDescAndFn};

        pub fn get_tests() -> Vec<TestDescAndFn> {
            $fixture::test_cases()
                .into_iter()
                .map(|t| {
                    create_test!(t.description.to_owned(), ShouldPanic::No, {
                        $fixture::run(&t);
                    })
                })
                .collect()
        }
    };
}

macro_rules! test_modules {
    ($($m:ident),+) => {
        $(mod $m;)+

        use rustc_test::TestDescAndFn;

        pub fn get_tests() -> Vec<TestDescAndFn> {
            let mut tests = Vec::default();

            $(tests.extend($m::get_tests());)+

            tests
        }
    };
}
